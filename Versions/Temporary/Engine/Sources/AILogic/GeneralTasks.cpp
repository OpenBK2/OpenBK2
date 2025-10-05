#include "stdafx.h"

#include "GeneralTasks.h"

#include "GeneralHelper.h"
#include "GeneralInternal.h"
#include "Formation.h"
#include "GroupLogic.h"
#include "Soldier.h"
#include "GeneralConsts.h"
#include "StaticObjects.h"
#include "TempBuffer.h"
#include "UnitsIterators2.h"
#include "UnitCreation.h"
#include "DebugTools/DebugInfoManager.h"
#include "UnitStates.h"
#include "B2AI.h"

#include <algorithm>

extern CSupremeBeing theSupremeBeing;
extern CStaticObjects theStaticObjects;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CGroupLogic theGroupLogic;
extern CUnitCreation theUnitCreation;
const float GetWeightOfUnit( const SUnitBaseRPGStats* pStats );


REGISTER_SAVELOAD_CLASS( 0x1508D489, CGeneralSwarmWaitForReady );
REGISTER_SAVELOAD_CLASS( 0x1508D48A, CGeneralTaskToSwarmToPoint );
REGISTER_SAVELOAD_CLASS( 0x1508D499, CGeneralTaskRecaptureStorage );
REGISTER_SAVELOAD_CLASS( 0x1508D4B1, CGeneralTaskToHoldReinforcement );
REGISTER_SAVELOAD_CLASS( 0x1508D4B2, CGeneralTaskToDefendPatch );


//*******************************************************************
//*											CGeneralTaskToDefendPatch										*
//*******************************************************************

CGeneralTaskToDefendPatch::CGeneralTaskToDefendPatch() 
: nCurReinforcePoint( 0 ), pOwner( 0 ), bFinished( false ), bResistanceRemoved( false )
{  
}

void CGeneralTaskToDefendPatch::InitTanks( CCommonUnit *pUnit )
{
	const CVec2 vRP ( patchInfo.reinforcePoints.empty() ? VNULL2: patchInfo.reinforcePoints[nCurReinforcePoint].vCenter );
	const WORD wRD = ( patchInfo.reinforcePoints.empty() ? 0 : patchInfo.reinforcePoints[nCurReinforcePoint].GetDir() );
	const CVec2 vTransReinforcePoint ( vRP.y, -vRP.x );
	const CVec2 vReinforcePoint( patchInfo.vCenter + ( vTransReinforcePoint ^ GetVectorByDirection(patchInfo.GetDir()) ) );
	const CVec2 vLookPoint( vReinforcePoint + GetVectorByDirection( wRD + patchInfo.GetDir() ) * 100 );

	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_SWARM_TO, vReinforcePoint ), pUnit, false );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_ROTATE_TO, vLookPoint ), pUnit, true );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF), pUnit, true );
	++nCurReinforcePoint;
	if ( !patchInfo.reinforcePoints.empty() )
		nCurReinforcePoint %= patchInfo.reinforcePoints.size();
}

void CGeneralTaskToDefendPatch::InitInfantryInTrenches( class CCommonUnit *pUnit )
{
	const CVec2 vSpyGlassPoint( patchInfo.vCenter + GetVectorByDirection( patchInfo.GetDir() ) * 1000 );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_USE_SPYGLASS, vSpyGlassPoint), pUnit, true );
}

void CGeneralTaskToDefendPatch::Init( const NDb::SAIGeneralParcel &_patchInfo, CGeneral *_pOwner )
{
	pOwner = _pOwner;
	patchInfo = _patchInfo;
	fSeverity = fEnemyForce = fFriendlyForce = fMaxSeverity = fFriendlyMobileForce = 0;
	bFinished = false;
	bWaitForFinish = false;
	timeLastUpdate = 0;
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "show_general_patches", false ) )
	{
		CSegment segm;
		segm.p1 = patchInfo.vCenter + CVec2( -20, -20 );
		segm.p2 = patchInfo.vCenter + CVec2( 20, 20 );
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
		segm.p1 = patchInfo.vCenter + CVec2( -20, 20 );
		segm.p2 = patchInfo.vCenter + CVec2( 20, -20 );
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );


		for ( int i = 0; i < patchInfo.reinforcePoints.size(); ++i )
		{
			const CVec2 vRP ( patchInfo.reinforcePoints.empty() ? VNULL2: patchInfo.reinforcePoints[i].vCenter );
			const WORD wRD = ( patchInfo.reinforcePoints.empty() ? 0 : patchInfo.reinforcePoints[i].GetDir() );
			const CVec2 vTransReinforcePoint ( vRP.y, -vRP.x );
			const CVec2 vP( patchInfo.vCenter + ( vTransReinforcePoint ^ GetVectorByDirection(patchInfo.GetDir()) ) );
			const CVec2 vLookPoint( vP + GetVectorByDirection( wRD + patchInfo.GetDir() ) * 100 );

			segm.p1 = vP + CVec2( -10, -10 );
			segm.p2 = vP + CVec2( 10, 10 );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
			segm.p1 = vP + CVec2( -10, 10 );
			segm.p2 = vP + CVec2( 10, -10 );
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
			segm.p1 = vP;
			segm.p2 = vLookPoint;
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );

		}
	}
#endif	
	pOwner->AddResistance( patchInfo.vCenter, patchInfo.fRadius );
}

ETaskName CGeneralTaskToDefendPatch::GetName() const 
{ 
	return ETN_DEFEND_PATCH; 
}

void CGeneralTaskToDefendPatch::AskForWorker( ICommander *pManager, const float _fMaxSeverity, const bool bInit )
{
	if ( bInit )
	{
		//cannot ask for reinforcement. it is initalization
		pManager->EnumWorkers( FT_INFANTRY_IN_TRENCHES, this );
		pManager->EnumWorkers( FT_STATIONARY_MECH_UNITS, this );
		pManager->EnumWorkers( FT_FREE_INFANTRY, this );
	}
	else 
	{
		fMaxSeverity = _fMaxSeverity;

		//ask for reinforcement
		if ( _fMaxSeverity > fSeverity && fSeverity <= 0 )
			pManager->EnumWorkers( FT_MOBILE_TANKS, this );
	}
}

void CGeneralTaskToDefendPatch::ReleaseWorker( ICommander *pManager, const float _fMinSeverity )
{
	if ( !bFinished )
	{
		// отдать часть подкрепления
		// возможно стоит выбрать самых ненужных юнитов - the resting tanks
		for ( CommonUnits::iterator it = tanksMobile.begin(); it != tanksMobile.end() &&fSeverity > _fMinSeverity && fEnemyForce == 0; )
		{
			CCommonUnit *pTank = *it;
			if ( !pTank->IsRefValid() || !pTank->IsAlive() )
			{
				it = tanksMobile.erase( it );
			}
			else if ( pTank->GetState()->IsRestState() && pTank->GetCurCmd() == 0 )
			{
				it = tanksMobile.erase( it );
				const float fFormerMobileForce = fFriendlyMobileForce;

				CalcSeverity( false, true );
				if ( fSeverity >= _fMinSeverity ) // юнит можно отдать без ущерба для ситуации
					pManager->Give( pTank );
				else
				{
					tanksMobile.push_front( pTank );
					fFriendlyMobileForce = fFormerMobileForce;
					CalcSeverity( false, false );
					break;
				}
			}
			else
				++it;
		}
	}
	else // отдать все резервы
	{
		CancelTask( pManager );
	}
}

float CGeneralTaskToDefendPatch::GetSeverity() const 
{ 
	return fSeverity;	
}

bool CGeneralTaskToDefendPatch::IsFinished() const 
{ 
	return bFinished; 
} 

void CGeneralTaskToDefendPatch::CalcSeverity( const bool bEnemyUpdated, const bool bFriendlyUpdated )
{
	if ( bEnemyUpdated )
	{
		fEnemyForce = 0;
		theSupremeBeing.GetEnemyConatiner( pOwner->GetParty() )->GiveEnemies( this );
	}
	
	if ( bFriendlyUpdated )
	{
		SGeneralHelper::SSeverityCountPredicate pr1;
		fFriendlyForce = 0;
		pr1 = for_each( infantryInTrenches.begin(), infantryInTrenches.end(), pr1 );
		pr1 = for_each( infantryFree.begin(), infantryFree.end(), pr1 );
		pr1 = for_each( stationaryUnits.begin(), stationaryUnits.end(), pr1 );
		fFriendlyForce += pr1.fCount;

		SGeneralHelper::SSeverityCountPredicate pr2;
		pr2 = for_each( tanksMobile.begin(), tanksMobile.end(), pr2 );
		fFriendlyMobileForce = pr2.fCount;
	}
	// Add importance
	fSeverity = fFriendlyForce + fFriendlyMobileForce  - fabs( patchInfo.fImportance ) - SGeneralConsts::PLAYER_FORCE_COEFFICIENT * fEnemyForce;
}

void CGeneralTaskToDefendPatch::CancelTask( ICommander *pManager )
{
	theSupremeBeing.GetEnemyConatiner( pOwner->GetParty() )->RemoveResistance( patchInfo.vCenter );

	for ( CommonUnits::iterator it = infantryInTrenches.begin(); infantryInTrenches.end() != it; ++it )
		pManager->Give( *it );
	for ( CommonUnits::iterator it = infantryFree.begin(); it != infantryFree.end(); ++it )
		pManager->Give( *it );
	for ( CommonUnits::iterator it = tanksMobile.begin(); tanksMobile.end() != it; ++it )
		pManager->Give( *it );
	for ( CommonUnits::iterator it = newTanks.begin(); newTanks.end() != it; ++it )
		pManager->Give( *it );
	for ( CommonUnits::iterator it = stationaryUnits.begin(); stationaryUnits.end() != it; ++it )
		pManager->Give( *it );

	bFinished = true;
}

void CGeneralTaskToDefendPatch::Segment()
{
	const int x = patchInfo.vCenter.x;
	const int y = patchInfo.vCenter.y;

	if ( bFinished ) 
	{
		return;
	}

	bool bNeedRecalc = false;

	// swarm new tanks all if needed amount gained
	if ( !newTanks.empty() )
	{
		SGeneralHelper::SSeverityCountPredicate pr1;
		pr1 = for_each( newTanks.begin(), newTanks.end(), pr1 );
		bool bNeedMore = pr1.fCount < patchInfo.nMinUnitsToReinforce;
		if ( !bNeedMore )
		{
			for ( CommonUnits::iterator it = newTanks.begin(); it != newTanks.end(); ++it )
				CGeneralTaskToDefendPatch::InitTanks( *it );
			tanksMobile.splice( tanksMobile.end(), newTanks );
			bNeedRecalc = true;
		}
	}

	//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 2, StrFmt( "(%d,%d), SEVERITY = %f", x, y, GetSeverity() ) );
	//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 3, StrFmt( "(%d,%d), FOE = %f", x, y, fEnemyForce ) );
	//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 4, StrFmt( "(%d,%d), FRIEND = %f / %f", x, y, fFriendlyForce, fFriendlyMobileForce ) );
	
	// find and remove dead units
	
	bNeedRecalc |= SGeneralHelper::RemoveNotCurrentPlayer( &infantryFree, pOwner->GetParty() );
	bNeedRecalc |= SGeneralHelper::RemoveNotCurrentPlayer( &infantryInTrenches, pOwner->GetParty() );
	bNeedRecalc |= SGeneralHelper::RemoveNotCurrentPlayer( &tanksMobile, pOwner->GetParty() );
	bNeedRecalc |= SGeneralHelper::RemoveNotCurrentPlayer( &stationaryUnits, pOwner->GetParty() );

	// calculate severity
	CalcSeverity( true, bNeedRecalc );
	
	if ( fFriendlyForce == 0.0f && !bResistanceRemoved ) // all stationary defenders are killed
	{
		bResistanceRemoved = true;
		theSupremeBeing.GetEnemyConatiner( pOwner->GetParty() )->RemoveResistance( patchInfo.vCenter );
	}
	if ( fFriendlyForce != 0.0f && bResistanceRemoved  )
	{
		bResistanceRemoved = false;
		theSupremeBeing.GetEnemyConatiner( pOwner->GetParty() )->AddResistance( patchInfo.vCenter, patchInfo.fRadius );
	}
}

int CGeneralTaskToDefendPatch::GetWorkerCount()
{
	return tanksMobile.size() + newTanks.size();
}

bool CGeneralTaskToDefendPatch::EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType )
{
		// give orders to the worker
	switch( eType )
	{
	case FT_FREE_INFANTRY:
		if ( SGeneralHelper::IsUnitNearParcel( pUnit, patchInfo ) )
		{
			NI_ASSERT( dynamic_cast<CFormation*>(pUnit) != 0, "not infantry passed" );
			infantryInTrenches.push_back( checked_cast<CFormation*>(pUnit) );
			InitInfantryInTrenches( pUnit );
			SGeneralHelper::SSeverityCountPredicate pr1;
			pr1( pUnit );
			fFriendlyForce += pr1.fCount;
			CalcSeverity( false, false );		
		}

		return true;
	case FT_INFANTRY_IN_TRENCHES:
		if ( SGeneralHelper::IsUnitNearParcel( pUnit, patchInfo ) )
		{
			NI_ASSERT( dynamic_cast<CFormation*>(pUnit) != 0, "not infantry passed" );
			infantryInTrenches.push_back( checked_cast<CFormation*>(pUnit) );
			InitInfantryInTrenches( pUnit );
			SGeneralHelper::SSeverityCountPredicate pr1;
			pr1( pUnit );
			fFriendlyForce += pr1.fCount;
		}			
		return true;
	case FT_STATIONARY_MECH_UNITS:
		if ( SGeneralHelper::IsUnitNearParcel( pUnit, patchInfo ) )
		{
			NI_ASSERT( dynamic_cast<CAIUnit*>(pUnit) != 0, "not mechUnit passed" );
			stationaryUnits.push_back( checked_cast<CAIUnit*>(pUnit) );
			SGeneralHelper::SSeverityCountPredicate pr1;
			pr1( pUnit );
			fFriendlyForce += pr1.fCount;
		}
		
		return true;
	case FT_MOBILE_TANKS:
		if ( patchInfo.nMinUnitsToReinforce > 0 )
		{
			NI_ASSERT( dynamic_cast<CAIUnit*>(pUnit) != 0, "not tank passed" );
			newTanks.push_back( checked_cast<CAIUnit*>(pUnit) );
			SGeneralHelper::SSeverityCountPredicate pr1;
			pr1 = for_each( newTanks.begin(), newTanks.end(), pr1 );
			bool bNeedMore = pr1.fCount < patchInfo.nMinUnitsToReinforce;
			
			if ( !bNeedMore )
				Segment();
			return bNeedMore;
		}
		else
		{
			NI_ASSERT( dynamic_cast<CAIUnit*>(pUnit) != 0, "not tank passed" );
			{
				tanksMobile.push_back( checked_cast<CAIUnit*>(pUnit) );
				InitTanks( pUnit );
				SGeneralHelper::SSeverityCountPredicate pr1;
				pr1( pUnit );
				fFriendlyMobileForce += pr1.fCount;
				CalcSeverity( false, false );		
			}
			return fSeverity < fMaxSeverity && fSeverity < 0;
			break;
		}
		break;
	}
	return false;
}

bool CGeneralTaskToDefendPatch::EvaluateWorker( CCommonUnit *pUnit, const enum EForceType eType ) const
{
	switch( eType )
	{
	case FT_INFANTRY_IN_TRENCHES:
	case FT_FREE_INFANTRY:
	case FT_STATIONARY_MECH_UNITS:
		return SGeneralHelper::IsUnitInParcel( pUnit, patchInfo );

	case FT_MOBILE_TANKS:
		// гаубичные орудия не посылаем
		if ( pUnit->GetFirstArtilleryGun() != 0 )
			return false;
		else if ( enemyForces.empty() )	// no enemies, any unit will be fine
		{
			return true;
		}
		// проверить, пробивает ли предложенный юнит кого-то из врагов
		else for ( CommonUnits::const_iterator it = enemyForces.begin(); it != enemyForces.end(); ++it )
		{
			NI_ASSERT( dynamic_cast_ptr<CAIUnit*>(*it) != 0, "wrong tank" );
			CAIUnit * pEnemy = static_cast_ptr<CAIUnit*>( *it );
			if ( 0 != pUnit->GetKillSpeed( pEnemy ) ) // можем пробить
				return true;
		}
		return false; 
	}
	return false;
}

bool CGeneralTaskToDefendPatch::EnumEnemy( class CAIUnit *pEnemy )
{

	if ( SGeneralHelper::IsUnitNearParcel( pEnemy, patchInfo ) )
	{
		// scan for other enemies inside resistance point (yes, look under war fog)
		for ( CUnitsIter<0,2> iter( pEnemy->GetParty(), EDI_FRIEND, patchInfo.vCenter, patchInfo.fRadius ); !iter.IsFinished(); iter.Iterate() )
		{
			if ( SGeneralHelper::IsUnitNearParcel( pEnemy, patchInfo ) )
			{
				SGeneralHelper::SSeverityCountPredicate pr;
				CAIUnit * pNearby = *iter;
				pr( pNearby );
				fEnemyForce += pr.fCount;
				enemyForces.push_back( pNearby );
			}
		}
		return false;
	}
	return true;
}

//*******************************************************************
//*											CGeneralTaskToHoldReinforcement							*
//*******************************************************************

CGeneralTaskToHoldReinforcement::CGeneralTaskToHoldReinforcement()
: fSeverity( 0 ), nCurReinforcePoint( 0 )
{
}

void CGeneralTaskToHoldReinforcement::Init( const NDb::SAIGeneralParcel &_patchInfo )
{
	patchInfo = _patchInfo;
//	NI_ASSERT( !patchInfo.reinforcePoints.empty(), "no places to hold reinforcements, adding VNULL2" );
	if ( patchInfo.reinforcePoints.empty() )
	{
		NDb::SReinforcePoint pt;
		pt.fDirection = 0.0f;
		pt.vCenter = VNULL2;
		patchInfo.reinforcePoints.push_back( pt );
	}
	fSeverity = 0;
}

void CGeneralTaskToHoldReinforcement::AskForWorker( ICommander *pManager, const float fMaxSeverity, const bool bInit )
{
	if ( !bInit && fMaxSeverity >= 0.0f )
	{
		// in non-combat situation
		// забрать под свое комманлование все свободные танки.
		pManager->EnumWorkers( FT_MOBILE_TANKS, this );
	}
}

void CGeneralTaskToHoldReinforcement::ReleaseWorker( ICommander *pManager, const float fMinSeverity )
{
	// отдать все танки генералу, пусть выберет лучший для своей цели
	while ( fMinSeverity < 0 && !tanksFree.empty() )
	{
		CCommonUnit *pTank = *tanksFree.begin();
		tanksFree.pop_front();
		//don't erase position, unit may be given back to reinforcement.
		//unitsPositions.erase( pUnit );

		if ( pTank->IsRefValid() && pTank->IsAlive() )
			pManager->Give( pTank );
	}
}

void CGeneralTaskToHoldReinforcement::CancelTask( ICommander *pManager )
{
	for ( CommonUnits::iterator it = tanksFree.begin(); tanksFree.end() != it; ++it )
		pManager->Give( *it );
}

int CGeneralTaskToHoldReinforcement::GetWorkerCount()
{
	return tanksFree.size();
}

void CGeneralTaskToHoldReinforcement::Segment()
{
	if ( SGeneralHelper::RemoveDead( &tanksFree ) )
	{
		std::list< CPtr<CCommonUnit> > removed;
		for ( UnitsPositions::iterator it = unitsPositions.begin(); it != unitsPositions.end(); ++it )
		{
			CCommonUnit *pUnit = GetObjectByUniqueIdSafe<CCommonUnit>( it->first );
			if ( !pUnit || !pUnit->IsRefValid() || !pUnit->IsAlive() )
				removed.push_back( pUnit );
		}
	}
	SGeneralHelper::SSeverityCountPredicate pr1;
	pr1 = for_each( tanksFree.begin(), tanksFree.end(), pr1 );
	fSeverity = pr1.fCount;
}

bool CGeneralTaskToHoldReinforcement::EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType )
{
	// послать танк на место сбора подкрепления
	NI_ASSERT( FT_MOBILE_TANKS == eType, "not tank reinforcement" );
	
	tanksFree.push_back( checked_cast<CAIUnit*>(pUnit) );
	if ( EPATCH_UNKNOWN == patchInfo.eType )
	{
		if ( unitsPositions.find( pUnit->GetUniqueId() ) == unitsPositions.end() )
			unitsPositions[pUnit->GetUniqueId()] = pUnit->GetCenterPlain();
		else
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_SWARM_TO, unitsPositions[pUnit->GetUniqueId()] ), pUnit, false );
	}
	else
	{
		const CVec2 vRP ( patchInfo.reinforcePoints.empty() ? VNULL2: patchInfo.reinforcePoints[nCurReinforcePoint].vCenter );
		const WORD wRD = ( patchInfo.reinforcePoints.empty() ? 0 : patchInfo.reinforcePoints[nCurReinforcePoint].GetDir() );
		const CVec2 vTransReinforcePoint ( vRP.y, -vRP.x );
		const CVec2 vReinforcePoint( patchInfo.vCenter + ( vTransReinforcePoint ^ GetVectorByDirection(patchInfo.GetDir()) ) );
		const CVec2 vLookPoint( vReinforcePoint + GetVectorByDirection( wRD + patchInfo.GetDir() ) * 100 );

		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_SWARM_TO, vReinforcePoint ), pUnit, false );
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_ROTATE_TO, vLookPoint), pUnit, true );
		
		++nCurReinforcePoint;
		nCurReinforcePoint %= patchInfo.reinforcePoints.size();
	}
	return false; // take 1 worker at a time
}

float CGeneralTaskToHoldReinforcement::GetSeverity() const 
{ 
	//CRAP{ for test
	return /*fSeverity*/0.0f; 
	//CRAP}
}

bool CGeneralTaskToHoldReinforcement::EvaluateWorker( CCommonUnit *pUnit, const enum EForceType eType ) const
{
	if ( FT_MOBILE_TANKS == eType && pUnit->GetFirstArtilleryGun() == 0 )
	{
		return true;
	}
	return false;
}

//*******************************************************************
//*											CGeneralTaskRecaptureStorage								*
//*******************************************************************

CGeneralTaskRecaptureStorage::CGeneralTaskRecaptureStorage( const CVec2 & vReinforcePoint )
: vReinforcePoint( vReinforcePoint ), 
	fSeverity( -SGeneralConsts::RECAPTURE_ARTILLERY_TANKS_NUMBER ),
	bFinished( false )
{
}

void CGeneralTaskRecaptureStorage::AskForWorker( ICommander *pManager, const float fMaxSeverity, const bool bInit )
{
	if ( !bInit && fMaxSeverity > fSeverity )
		pManager->EnumWorkers( FT_MOBILE_TANKS, this );
}

void CGeneralTaskRecaptureStorage::ReleaseWorker( ICommander *pManager, const float fMinSeverity )
{
	if ( bFinished )
	{
		while( !tanksFree.empty() )
		{
			pManager->Give( *tanksFree.begin() );
			tanksFree.pop_front();
		}
	}
}

bool CGeneralTaskRecaptureStorage::IsFinished() const 
{
	return bFinished;
}

void CGeneralTaskRecaptureStorage::CancelTask( ICommander *pManager )
{
	bFinished = true;
	ReleaseWorker( pManager, 0 );
}

void CGeneralTaskRecaptureStorage::Segment()
{
	SGeneralHelper::RemoveDead( &tanksFree );
	
	if ( 0 == fSeverity && tanksFree.empty() )
		bFinished = true;
}

bool CGeneralTaskRecaptureStorage::EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType )
{
	if ( FT_MOBILE_TANKS == eType )
	{
		tanksFree.push_back( pUnit );
		theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_SWARM_TO, vReinforcePoint), pUnit );
		return ++fSeverity == 0;
	}
	NI_ASSERT( false, "wrong type of worker given" );
	return false;
}

bool CGeneralTaskRecaptureStorage::EvaluateWorker( CCommonUnit * pUnit, const enum EForceType eType ) const
{
	return fabs2( pUnit->GetCenterPlain() - vReinforcePoint ) < sqr(SGeneralConsts::RECAPTURE_STORAGE_MAX_DISTANCE);
}

//*******************************************************************
//*											CGeneralTaskToSwarmToPoint									*
//*******************************************************************

CGeneralTaskToSwarmToPoint::CGeneralTaskToSwarmToPoint( CGeneral *_pOwner )
: nAdditionalIterations( 0 ), fSeverity( 0 ), bFinished( false ), 
	bReleaseWorkers( false ), timeNextCheck( 0 ), 
	eState( ESS_REST ), pOwner( _pOwner ), bResistanesBusyByUs( false )
{
	ClearResistanceToAcceptNewTask();
}

CGeneralTaskToSwarmToPoint::CGeneralTaskToSwarmToPoint() : pOwner( 0 ), bResistanesBusyByUs( false )
{
}

bool CGeneralTaskToSwarmToPoint::IsTimeToRun() const
{
	// wait for some time, not forever.
	if ( curTime > timeNextCheck )
		return true;

	for ( CTanks::const_iterator it = swarmingTanks.begin(); it != swarmingTanks.end(); ++it )
	{
		CCommonUnit *pUnit = *it;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() && !pUnit->IsIdle() )
			return false;
	}
	return true;
}

int CGeneralTaskToSwarmToPoint::GetWorkerCount() 
{ 
	return swarmingTanks.size(); 
}

void CGeneralTaskToSwarmToPoint::Run()
{
	if ( !swarmingTanks.empty() )
	{
		const int nGroup = theGroupLogic.GenerateGroupNumber();
		CObjectBase **arUnits = GetLocalTempBuffer<CObjectBase*>( swarmingTanks.size() );
		for ( int i = 0; i < swarmingTanks.size(); ++i )
			arUnits[i] = swarmingTanks[i];
		theGroupLogic.RegisterGroup( arUnits, swarmingTanks.size(), nGroup );
		SAIUnitCmd cmd(ACTION_COMMAND_SWARM_TO, curResistanceToAttack.GetResistanceCellCenter());
		cmd.bFromAI = false;
		theGroupLogic.GroupCommand( cmd, nGroup, false );

		//for ( int i = 0; i < swarmingTanks.size(); ++i )
			//theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_SWARM_TO, curResistanceToAttack.GetResistanceCellCenter()), swarmingTanks[i], false );

		//CRAP{for testing
		{
//			const int x = curResistanceToAttack.GetResistanceCellCenter().x;
//			const int y = curResistanceToAttack.GetResistanceCellCenter().y;
			//Singleton<IStatistics>()->UpdateEntry( "General: (ATTACK):", StrFmt( "Attacking, (%d,%d)", x, y ) );
		}
		//CRAP}
		pOwner->SetCellInUse( curResistanceToAttack.GetCellNumber(), true );
		bResistanesBusyByUs = true;
		theGroupLogic.UnregisterGroup( nGroup );
		Singleton<IAILogic>()->SetNeedNewGroupNumber();
	}
	timeNextCheck = curTime + 1000* (SGeneralConsts::TIME_SWARM_DURATION + NRandom::Random( SGeneralConsts::TIME_SWARM_DURATION_RANDOM ));
	eState = ESS_ATTACKING;


}

void CGeneralTaskToSwarmToPoint::AskForWorker( ICommander *pManager, const float _fMaxSeverity, const bool bInit )
{
	if ( !bInit && eState == ESS_REST && curResistanceToAttack.IsInitted() && fSeverity < 0 && swarmingTanks.empty() )
	{
		fMaxSeverity = _fMaxSeverity;
		pManager->EnumWorkers( FT_SWARMING_TANKS, this ); // resieved some tanks.
		SendToGroupPoint();
	}
}

void CGeneralTaskToSwarmToPoint::SendToGroupPoint()
{
	if ( !swarmingTanks.empty() )
	{
		for ( int i = 0; i < swarmingTanks.size(); ++i )
		{
			if ( swarmingTanks[i]->IsRefValid() && swarmingTanks[i]->IsAlive() )
			{
				vPrepearCenter = swarmingTanks[i]->GetCenterPlain();
				break;
			}
		}

		for ( int i = 0; i < swarmingTanks.size(); ++i )
		{
			if ( swarmingTanks[i]->IsRefValid() && swarmingTanks[i]->IsAlive() )
				theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_TO_NOT_PRESIZE,vPrepearCenter, 32*SConsts::TILE_SIZE), swarmingTanks[i], true );
		}
		
		eState = ESS_PREPEARING;
		theSupremeBeing.RegisterDelayedTask( new CGeneralSwarmWaitForReady(this) );
		timeNextCheck = curTime + 1000*(SGeneralConsts::TIME_TO_WAIT_SWARM_READY + NRandom::Random( SGeneralConsts::TIME_TO_WAIT_SWARM_READY_RANDOM ));

		//CRAP{for testing
		{
//			const int x = vPrepearCenter.x;
//			const int y = vPrepearCenter.y;
			//Singleton<IStatistics>()->UpdateEntry( "General: (ATTACK):", StrFmt( "Prepearing, (%d,%d)", x, y ) );
		}
		//CRAP}

	}
}

void CGeneralTaskToSwarmToPoint::ReleaseWorker( ICommander *pManager, const float fMinSeverity )
{
	if ( (!curResistanceToAttack.IsInitted() || bReleaseWorkers ) && !swarmingTanks.empty() )
	{
		for ( int i = 0; i < swarmingTanks.size(); ++i )
			pManager->Give( swarmingTanks[i] );
		swarmingTanks.clear();
	}
}

float CGeneralTaskToSwarmToPoint::GetSeverity() const 
{ 
	return fSeverity; 
}

bool CGeneralTaskToSwarmToPoint::IsFinished() const
{
	return bFinished;
}

void CGeneralTaskToSwarmToPoint::CancelTask( ICommander *pManager )
{
	bFinished = true;
	ReleaseWorker( pManager, 0 );
}

void CGeneralTaskToSwarmToPoint::Segment()
{
	SGeneralHelper::SDeadPredicate pr;
	swarmingTanks.erase( remove_if( swarmingTanks.begin(), swarmingTanks.end(), pr ), swarmingTanks.end() );

	switch( eState )
	{
	case ESS_REST:
		if ( timeNextCheck < curTime )
		{
			timeNextCheck = curTime + 3000;		//
			//fSeverity = - 1;
			fSeverity = 0;
			ClearResistanceToAcceptNewTask();
			theSupremeBeing.GetEnemyConatiner( pOwner->GetParty() )->GiveResistances( this );

			// wait for some time
			bReleaseWorkers = !curResistanceToAttack.IsInitted();
			if ( fSeverity != 0.0f )
			{
				eState = ESS_WAIT_FOR_WORKERS;
				timeNextCheck = curTime;
			}
		}

		break;
	case ESS_WAIT_FOR_WORKERS:
		if ( !swarmingTanks.empty() )
		{
			SendToGroupPoint();
			nAdditionalIterations = NRandom::Random( 0, SGeneralConsts::SWARM_ADDITIONAL_ITERATIONS );
			bReleaseWorkers = false;
		}
		else if ( timeNextCheck < curTime )
		{
			eState = ESS_REST;
		}
		break;
	case ESS_PREPEARING:
		// check if all are dead
		for ( int i = 0; i < swarmingTanks.size(); ++i )
		{
			if ( IsValidObj( swarmingTanks[i] ) )
			{
				break;
			}
		}
		eState = ESS_REST;
	
		break;
	case ESS_ATTACKING:
		if ( timeNextCheck < curTime )
		{
			int nAlive = 0;
			int nIdle = 0;
			for ( int i = 0; i < swarmingTanks.size(); ++i )
			{
				if ( IsValidObj( swarmingTanks[i] ) )
				{
					++nAlive;
					if ( swarmingTanks[i]->GetState()->IsRestState() || !swarmingTanks[i]->CanMove() )
						++nIdle;
				}
			}
			if ( timeNextCheck < curTime || nIdle == nAlive || 0 == nAlive )
			{
				if ( ( 0 == nAlive || nIdle == nAlive ) && nAdditionalIterations <= 0 ) // don't want to swarm anymore
				{
					eState = ESS_REST;

					if ( bResistanesBusyByUs )
					{
						pOwner->SetCellInUse( curResistanceToAttack.GetCellNumber(), false );
						bResistanesBusyByUs = false;
					}

					curResistanceToAttack.Clear();

					bReleaseWorkers = true;
				}
				else  // will swarm another time
				{
					--nAdditionalIterations;

					if ( bResistanesBusyByUs )
					{
						pOwner->SetCellInUse( curResistanceToAttack.GetCellNumber(), false );
						bResistanesBusyByUs = false;
					}

					ClearResistanceToAcceptNewTask();
					theSupremeBeing.GetEnemyConatiner( pOwner->GetParty() )->GiveResistances( this );
					if ( curResistanceToAttack.IsInitted() )
						SendToGroupPoint();
				}
			}
		}

		break;
	}
}

void CGeneralTaskToSwarmToPoint::ClearResistanceToAcceptNewTask()
{
	if ( bResistanesBusyByUs )
	{
		pOwner->SetCellInUse( curResistanceToAttack.GetCellNumber(), false );
		bResistanesBusyByUs = false;
	}
	
	curResistanceToAttack.Clear();

	vTanksPosition = CVec2( -1,-1 );
	fCurDistance = -1;
	int nAlive = 0;
	for ( int i = 0; i < swarmingTanks.size(); ++i )
	{
		if ( IsValidObj( swarmingTanks[i] ) )
		{
			vTanksPosition += swarmingTanks[i]->GetCenterPlain();
			++nAlive;
		}
	}
	if ( nAlive )
		vTanksPosition /= nAlive;
}

bool CGeneralTaskToSwarmToPoint::EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType )
{
	NI_ASSERT( curResistanceToAttack.GetWeight() != -1, "wrong weight" );
	swarmingTanks.push_back( pUnit );
	fSeverity += pUnit->GetPriceMax();
	return fSeverity < fMaxSeverity;
}

float CGeneralTaskToSwarmToPoint::EvaluateWorkerRating( CCommonUnit * pUnit, const enum EForceType eType ) const
{
	const CVec2 vFinishPoint( curResistanceToAttack.GetResistanceCellCenter() );
	return 1.0f / ( fabs2( vFinishPoint - pUnit->GetCenterPlain() ) + 1 );
}

bool CGeneralTaskToSwarmToPoint::EnumResistances( const SResistance &resistance )
{
	if ( resistance.GetWeight() > SGeneralConsts::MIN_WEIGHT_TO_SEND_SWARM )
	{
		if ( vTanksPosition.x == -1 || vTanksPosition.y == -1 )
		{
			fSeverity = - resistance.GetWeight() * SGeneralConsts::SWARM_WEIGHT_COEFFICIENT;
			curResistanceToAttack = resistance;

			return false;
		}
		//choose nearest resistance
		const float fDistance = fabs2( vTanksPosition - resistance.GetResistanceCellCenter() );
		if ( fCurDistance == -1 || fDistance < fCurDistance )
		{
			fSeverity = - resistance.GetWeight() * SGeneralConsts::SWARM_WEIGHT_COEFFICIENT;
			curResistanceToAttack = resistance;
			fCurDistance = fDistance;
		}
	}
	return true;
}

bool CGeneralTaskToSwarmToPoint::EvaluateWorker( CCommonUnit *pUnit, const enum EForceType eType ) const
{
	if ( pUnit->GetFirstArtilleryGun() != 0 )
		return false;
	
	return true;
}


