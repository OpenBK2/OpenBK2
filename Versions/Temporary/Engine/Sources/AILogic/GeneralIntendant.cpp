#include "StdAfx.h"

#include "GeneralIntendant.h"

#include "STaticObjects.h"
#include "Building.h"
#include "AIUnit.h"
#include "GroupLogic.h"
#include "Diplomacy.h"
#include "GeneralConsts.h"
#include "GeneralHelper.h"
#include "General.h"
#include "UnitStates.h"
#include "..\Common_RTS_AI\AIMap.h"
#include "TimeCounter.h"
#include "Units.h"
#include "UnitsIterators2.h"
#include "Formation.h"
#include "Soldier.h"
#include "Artillery.h"
#include "../System/Commands.h"
#include "GlobalWarFog.h"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NAI::CTimeCounter timeCounter;
extern CDiplomacy theDipl;
extern CGroupLogic theGroupLogic;
extern CStaticObjects theStatObjs;
extern CSupremeBeing theSupremeBeing;
extern NTimer::STime curTime;
extern CGlobalWarFog theWarFog;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//BASIC_REGISTER_CLASS( CGeneralIntendant );
static const unsigned int INTENDANT_TASKS_PER_SEGMENT = 5;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x1508D495, CGeneralTaskCheckCellDanger );
REGISTER_SAVELOAD_CLASS( 0x1508D496, CGeneralTaskToResupplyCell );
REGISTER_SAVELOAD_CLASS_NM( 0x1508D497, CWaitForChangePlayer, CGeneralTaskToDefendStorage );
REGISTER_SAVELOAD_CLASS( 0x1508D498, CResupplyCellInfo );
REGISTER_SAVELOAD_CLASS( 0x1508D49B, CGeneralTaskToDefendStorage );
REGISTER_SAVELOAD_CLASS( 0x1508D49C, CGeneralIntendant );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*								CGeneralTaskToDefendStorage*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CGeneralTaskToDefendStorage::CWaitForChangePlayer::CWaitForChangePlayer( CBuilding * pStorage, CGeneralTaskToDefendStorage * pMainTask, const int nParty )
	: pStorage( pStorage ), pMainTask( pMainTask ), nParty( nParty )
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGeneralTaskToDefendStorage::CWaitForChangePlayer::IsTimeToRun() const
{
	return theDipl.GetNParty( pStorage->GetPlayer() ) == nParty ;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralTaskToDefendStorage::CWaitForChangePlayer::Run()
{
	pMainTask->Recaptured();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*								CGeneralTaskToDefendStorage*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CGeneralTaskToDefendStorage::CGeneralTaskToDefendStorage ( CBuilding * pStorage, const int nParty ) 
: pStorage( pStorage ),
  fSeverity( 0 ),
	nParty( nParty ),
	//eState( TS_OPERATE ),
	wRequestID( 0 )
{  
	eState = EDI_FRIEND == theDipl.GetDiplStatusForParties( theDipl.GetNParty(pStorage->GetPlayer()), nParty ) ?
					 TS_OPERATE : TS_RECAPTURE ;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralTaskToDefendStorage::AskForWorker( ICommander *pManager, const float fMaxSeverity, const bool bInit )
{
	if ( !bInit && fSeverity < fMaxSeverity )
	{
		switch( eState )
		{
		case TS_RECAPTURE:
			if ( !wRequestID )
				wRequestID = pManager->RequestForSupport( CVec2(pStorage->GetCenter().x, pStorage->GetCenter().y), FT_RECAPTURE_STORAGE );
			break;
		case TS_REPAIR:
			if ( !IsValidObj(pRepairTransport) )
				pManager->EnumWorkers( FT_TRUCK_REPAIR_BUILDING, this );
			break;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGeneralTaskToDefendStorage::EnumWorker( class CCommonUnit *pUnit, const enum EForceType eType )
{
	switch( eType )
	{
	case FT_TRUCK_REPAIR_BUILDING:
		pRepairTransport = pUnit;
		theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_REPEAR_OBJECT, pStorage->GetUniqueId()), pRepairTransport );
		fSeverity = 0;

		return false;
	default:
		NI_ASSERT(false, StrFmt( "didn't asked worker of type %i", eType ) );

	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CGeneralTaskToDefendStorage::NeedNBest( const enum EForceType eType ) const 
{ 
	return (eType == FT_TRUCK_REPAIR_BUILDING ? 1 : 0);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CGeneralTaskToDefendStorage::EvaluateWorkerRating( CCommonUnit * pUnit, const enum EForceType eType ) const 
{ 
	switch( eType )
	{
	case FT_TRUCK_REPAIR_BUILDING:
		{
			// у юнитов есть функция GetCenter, которая возвращает 3D координаты юнита
			const float fDist = fabs2( pStorage->GetCenter() - CVec3( pUnit->GetCenterPlain(),0 ) );
			if ( fDist == 0.0f )
				return 1;
			return 1.0f / fDist; 
		}

		break;
	}
	//
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGeneralTaskToDefendStorage::EvaluateWorker( CCommonUnit * pUnit, const enum EForceType eType ) const
{
	switch( eType )
	{
	case FT_TRUCK_REPAIR_BUILDING:
		return pUnit->CanCommandBeExecutedByStats( ACTION_COMMAND_REPEAR_OBJECT );
		break;
	}
	//
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralTaskToDefendStorage::ReleaseWorker( ICommander *pManager, const float fMinSeverity )
{
	if ( wRequestID && eState == TS_FINISH_RECAPTURE )
	{
		pManager->CancelRequest( wRequestID, FT_RECAPTURE_STORAGE );
		wRequestID = 0;
	}

	if ( IsValidObj( pRepairTransport ) && 
			( eState == TS_FINISHED || pStorage->GetHitPoints() != 0 ) )
	{
		pManager->Give( pRepairTransport );
		pRepairTransport = 0;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralTaskToDefendStorage::CancelTask( ICommander *pManager )
{
	if ( pRepairTransport )
		pManager->Give( pRepairTransport );
	pRepairTransport = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralTaskToDefendStorage::Recaptured()
{
	eState = TS_FINISH_RECAPTURE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralTaskToDefendStorage::Segment()
{
	switch( eState )
	{
	case TS_FINISHED:

		break;
	case TS_OPERATE:
		if ( theDipl.GetNParty(pStorage->GetPlayer()) != nParty )
			eState = TS_START_RECAPTURE;
		else if ( pStorage->GetHitPoints()	== 0 ) 
			eState = TS_START_REPAIR;

		break;
	case TS_START_RECAPTURE:
		// storage was captured. 
		// after some time we will ask for support ( to recapture storage )
		if ( NRandom::Random( 0.0f, 1.0f ) < SGeneralConsts::RECAPTURE_STORAGE_PROBALITY )
		{
			eState = TS_RECAPTURE;
			fSeverity = -1;
			theSupremeBeing.RegisterDelayedTask( new CWaitForChangePlayer( pStorage, this, nParty ) );
		}

		break;
	case TS_RECAPTURE:
		if ( theDipl.GetNParty(pStorage->GetPlayer()) == nParty )
		{
			eState = TS_FINISH_RECAPTURE;
		}

		break;
	case TS_FINISH_RECAPTURE:
		if ( !wRequestID )
		{
			fSeverity = 0;
			eState = TS_OPERATE;
		}
		break;
	case TS_START_REPAIR:
		if ( NRandom::Random( 0.0f, 1.0f ) < SGeneralConsts::REPAIR_STORAGE_PROBABILITY ) 
		{
			eState = TS_REPAIR;
			fSeverity = -1;
		}

		break;
	case TS_REPAIR:
		if ( pStorage->GetHitPoints() != 0 )
		{
			fSeverity = pStorage->GetHitPoints() / pStorage->GetStats()->fMaxHP;
			eState = TS_OPERATE;
		}
		
		if ( pRepairTransport && !IsValidObj( pRepairTransport ) )
			pRepairTransport = 0;
		
		break;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*								CGeneralIntendant::SEnumStorages*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct SEnumStorages : public CStaticObjects::IEnumStoragesPredicate 
{
	Storages storages;
	virtual bool OnlyConnected() const { return false ; }
	virtual bool AddStorage( class CBuilding * pStorage, const float fPathLenght )
	{
		const SBuildingRPGStats * pStats = checked_cast<const SBuildingRPGStats*>( pStorage->GetStats() );
		if ( TYPE_TEMP_RU_STORAGE == pStats->etype )
		{
			storages.push_back( pStorage );
		}
		return true;
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*								CGeneralTaskToResupplyCell*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CGeneralTaskToResupplyCell::CGeneralTaskToResupplyCell( CResupplyCellInfo * pCell, const int nParty, const enum EResupplyType _eResupplyType, class CGeneralIntendant *_pCells ) 
: pCell( pCell ), fSeverity( 1.0f ), eResupplyType( _eResupplyType ), bFinished( false ), nParty( nParty ),
	timeNextCheck ( curTime ), pCells( _pCells )
{ 
	pCell->MarkUnderSupply( eResupplyType );
	vResupplyCenter = pCell->CalcResupplyPos( eResupplyType );
	theSupremeBeing.RegisterDelayedTask( new CGeneralTaskCheckCellDanger( this, pCell, eResupplyType, _pCells ) );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralTaskToResupplyCell::AskForWorker( ICommander *pManager, const float fMaxSeverity, const bool bInit ) 
{ 
	if ( !bInit &&
			 pCell->GetNNeeded( eResupplyType ) != 0 &&
			 fSeverity <= fMaxSeverity && 
			 !IsValidObj( pResupplyTransport) )
		pManager->EnumWorkers( FT_TRUCK_RESUPPLY, this );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralTaskToResupplyCell::ReleaseWorker( ICommander *pManager, const float fMinSeverity ) 
{ 
	if ( bFinished )
	{
		pCell->MarkUnderSupply( eResupplyType, false );
		if ( IsValidObj( pResupplyTransport ) )
		{
			pManager->Give( pResupplyTransport );
			pResupplyTransport = 0;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralTaskToResupplyCell::CancelTask( ICommander *pManager ) 
{ 
	bFinished = true;
	ReleaseWorker( pManager, 0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralTaskToResupplyCell::Segment() 
{ 
	//transport was killed.
	if ( pResupplyTransport && !IsValidObj( pResupplyTransport ) )
	{
		pCells->MarkCellsDangerous( pCell->GetCenter() );		
	}
	//all was resupplied
	if ( pCell->GetNNeeded( eResupplyType ) == 0 || pCell->IsDangerous() )
		bFinished = true;

	// truck don't have resources and cannot find storage
	if ( IsValidObj( pResupplyTransport ) && 
				EUSN_REST == pResupplyTransport->GetState()->GetName() )
	{
		bFinished = true;
	}

	if ( fSeverity == 1 && curTime >= timeNextCheck )
		fSeverity = -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGeneralTaskToResupplyCell::EnumWorker( class CCommonUnit *pUnit, const enum EForceType _eType ) 
{ 
	NI_ASSERT( !IsValidObj( pResupplyTransport ), "2 transport at 1 task" );
	pResupplyTransport = pUnit;

	pCell->IssueCommand( pUnit, eResupplyType, vResupplyCenter );
	fSeverity = 0;
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGeneralTaskToResupplyCell::EvaluateWorker( CCommonUnit * pUnit, const enum EForceType _eType ) const
{ 
	return pCell->IsUnitSuitable( pUnit, eResupplyType );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CGeneralTaskToResupplyCell::NeedNBest( const enum EForceType _eType ) const
{ 
	return 1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CGeneralTaskToResupplyCell::EvaluateWorkerRating( CCommonUnit * pUnit, const enum EForceType _eType ) const
{ 
	return 1000.0f / fabs2( pUnit->GetCenterPlain() - vResupplyCenter );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*								CResupplyCellInfo*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CResupplyCellInfo::CResupplyCellInfo()
: cMarkedUnderSupply( 0 ), fCount( 0.0f ), timeLastDanger( 0 )
{
	resupplyCount.resize( _ERT_COUNT, 0.0f );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResupplyCellInfo::AddUnitResupply( CCommonUnit *pUnit, const enum EResupplyType eType )
{
	if ( pUnit->IsRefValid() && pUnit->IsAlive() )
	{
		const int nID = pUnit->GetUniqueId();

		// if this unit doesn't exist, create it.
		if ( resupplyInfo.find( nID ) == resupplyInfo.end() )
			resupplyInfo.insert( pair<int,BYTE>( nID,0) );

		if ( !(resupplyInfo[nID] & (1<<eType)) )
		{
			resupplyInfo[nID] |= (1<<eType);
			const float fPrice = pUnit->GetPriceMax();
			resupplyCount[eType] += fPrice;
			fCount += fPrice;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResupplyCellInfo::RemoveUnitResupply( class CCommonUnit *pUnit, const enum EResupplyType eType )
{
	const int nID = pUnit->GetUniqueId();
	CResupplyInfo::iterator curResupply = resupplyInfo.find( nID );
	if ( resupplyInfo.end() != curResupply )
	{
		const BYTE cRes = curResupply->second;
		if ( cRes & (1<<eType) )
			RemoveUnitResupplyInternal( pUnit, eType );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResupplyCellInfo::RemoveUnitResupplyInternal( class CCommonUnit *pUnit, const enum EResupplyType eType )
{
	const float fPrice = pUnit->GetPriceMax();
	resupplyCount[eType] -= fPrice;
	fCount -= fPrice;

	const int nID = pUnit->GetUniqueId();
	NI_ASSERT( resupplyInfo.end() != resupplyInfo.find( nID ), "unit unregistered" );
	CResupplyInfo::iterator curInfo = resupplyInfo.find( nID );
	curInfo->second &= ~(1<<eType);
	
	if ( 0 == curInfo->second )
		resupplyInfo.erase( nID );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResupplyCellInfo::SetDanger( const NTimer::STime timeDanger ) 
{ 
	timeLastDanger = timeDanger; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CResupplyCellInfo::IsDangerous() const 
{ 
	return timeLastDanger && timeLastDanger > curTime; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResupplyCellInfo::AddUnit( class CCommonUnit *pUnit, const BYTE cRes )
{
	NI_ASSERT( 0 !=  cRes, "unit doesn't need resupply" );
	NI_ASSERT( cRes < (1<<_ERT_COUNT), StrFmt( "wrong resupply mask %d", cRes ) ) ;
	
//	const int nID = pUnit->GetUniqueId();
//	const float fPrice = pUnit->GetPriceMax();
	for ( int i = 0; i < _ERT_COUNT; ++i )
	{
		if ( cRes & (1<<i) )
			AddUnitResupply( pUnit, static_cast<EResupplyType>(i) );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BYTE CResupplyCellInfo::RemoveUnit( class CCommonUnit *pUnit )
{
//	const float fPrice = pUnit->GetPriceMax();
	const int nID = pUnit->GetUniqueId();

	CResupplyInfo::iterator curResupply = resupplyInfo.find( nID );
	
	if ( resupplyInfo.end() != curResupply )
	{
		const BYTE cRes = curResupply->second;
		for ( int i = 0; i < _ERT_COUNT && resupplyInfo.find( nID ) != resupplyInfo.end(); ++i )
		{
			if ( cRes & (1<<i) )
				RemoveUnitResupplyInternal( pUnit, static_cast<EResupplyType>(i) );
		}
		return cRes;
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CResupplyCellInfo::GetNNeeded( const BYTE cTypeMask ) const
{
	float fLocalCount = 0;
	for ( int i = 0; i < _ERT_COUNT; ++i )
	{
		if ( (cTypeMask & (1<<i)) && !(cMarkedUnderSupply & (1<<i)) )
			fLocalCount += resupplyCount[i];
	}
	return fLocalCount;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CResupplyCellInfo::IsEmpty() const
{
	return fCount == 0.0f;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CResupplyCellInfo::IsUnitRegistered( CCommonUnit * pUnit ) const
{
	CResupplyInfo::const_iterator it = resupplyInfo.find( pUnit->GetUniqueId() );
	return it != resupplyInfo.end();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResupplyCellInfo::MarkUnderSupply( const enum EResupplyType eType, const bool bSupply )
{
	if ( bSupply )
		cMarkedUnderSupply |= (1<<eType); 
	else
		cMarkedUnderSupply &= ~(1<<eType);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CResupplyCellInfo::IsUnitSuitable( const class CCommonUnit * pUnit, const enum EResupplyType eType )
{
	switch( eType )
	{
	case ERT_REPAIR:
		return pUnit->CanCommandBeExecutedByStats( ACTION_COMMAND_REPAIR );
	case ERT_RESUPPLY:
		return pUnit->CanCommandBeExecutedByStats( ACTION_COMMAND_RESUPPLY );
	case ERT_HUMAN_RESUPPLY:
		return pUnit->CanCommandBeExecutedByStats( ACTION_COMMAND_RESUPPLY_HR );
	case ERT_MORALE:
		return pUnit->CanCommandBeExecutedByStats( ACTION_COMMAND_RESUPPLY_MORALE );
	case ERT_MEDICINE:
		return pUnit->CanCommandBeExecutedByStats( ACTION_COMMAND_HEAL_INFANTRY );
	default:
		NI_ASSERT( false, StrFmt( "unknown resupply type asked %d", eType ) );
		return false;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CResupplyCellInfo::IssueCommand( class CCommonUnit * pUnit, const enum EResupplyType eType, const CVec2 &vResupplyCenter )
{
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_TO_NOT_PRESIZE, vResupplyCenter, SConsts::GENERAL_CELL_SIZE * 2 ), pUnit, false );
	switch( eType )
	{
	case ERT_REPAIR:
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_REPAIR, vResupplyCenter), pUnit, true );
		break;
	case ERT_RESUPPLY:
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_RESUPPLY, vResupplyCenter), pUnit, true );
		break;
	case ERT_HUMAN_RESUPPLY:
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_RESUPPLY_HR, vResupplyCenter), pUnit, true );
		break;
	case ERT_MEDICINE:
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_MOVE_TO, vResupplyCenter), pUnit, true );
		break;
	case ERT_MORALE:
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_MOVE_TO, vResupplyCenter), pUnit, true );
		break;
	default:
		NI_ASSERT( false, "unknown resupply type asked" );
	}
	
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CVec2 CResupplyCellInfo::CalcResupplyPos( const enum EResupplyType eType ) const
{
	return AICellsTiles::GetCenterOfGeneralCell( vCell );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*								CFreeArtilleryHolder                              *
//*******************************************************************
void CFreeArtilleryHolder::ResetIterator()
{
	if ( artilleries.empty() )
		itCurrent = artilleries.end();
	else
	{
		itCurrent = artilleries.begin();
		timeFromReset = curTime;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFreeArtilleryHolder::AddArtillery( CArtillery *pArtillery )
{
	const int nUnitID = pArtillery->GetUniqueID();
	if ( IsExists( nUnitID ) )
		return;
	const int nCurrentID = !IsValidCurrent() ? -1 : itCurrent->second.pArtillery->GetUniqueID();
	artilleries[nUnitID] = SArtilleryInfo( pArtillery );
	if ( nCurrentID == -1 ) 
		ResetIterator();
	else
		itCurrent = artilleries.find( nCurrentID );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFreeArtilleryHolder::RemoveArtillery( CArtillery *pArtillery )
{
	const int nUnitID = pArtillery->GetUniqueID();
	if ( !IsExists( nUnitID ) )
		return;
	const int nCurrentID = !IsValidCurrent() ? -1 : itCurrent->second.pArtillery->GetUniqueID();
	TArtilleries::iterator posDelete = artilleries.find( nUnitID );
	artilleries.erase( posDelete );
	//DebugTrace( "Remove artillery %d from holder", nUnitID );
	if ( nCurrentID == -1 || artilleries.empty() )
		itCurrent = artilleries.end();
	else if ( nCurrentID == nUnitID )  
		ResetIterator();
	else
		itCurrent = artilleries.find( nCurrentID );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CFreeArtilleryHolder::IsValidCurrent() const
{
	if ( itCurrent == artilleries.end() )
		return false;
	
	return IsValid( itCurrent->second.pArtillery ) && !(itCurrent->second.pArtillery->HasServeCrew());
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CFreeArtilleryHolder::IsFreeCurrent() const
{
	if ( itCurrent == artilleries.end() )
		return false;

	if ( !IsValid( itCurrent->second.pCatchingFormation ) )
		return true;

	if ( itCurrent->second.pCatchingFormation->GetState()->GetName() == EUSN_GUN_CAPTURE )
		return false;

	itCurrent->second.pCatchingFormation = 0;
	return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFreeArtilleryHolder::Segment()
{
	if ( artilleries.empty() )
		return;
	timeFromReset += SAIConsts::AI_SEGMENT_DURATION;
	if ( itCurrent == artilleries.end() )
	{
		if ( curTime - timeFromReset > 1000 )
			ResetIterator();
	}
	else
	{
		++itCurrent;
		while ( itCurrent != artilleries.end() )
		{
			if ( !IsValidCurrent() )
				artilleries.erase( itCurrent++ );
			else if ( !IsFreeCurrent() )
				++itCurrent;
			else 
				return;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CArtillery *CFreeArtilleryHolder::Get() const
{
	return itCurrent == artilleries.end() ? 0 : itCurrent->second.pArtillery;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFreeArtilleryHolder::TryCatch( CFormation *pFormation )
{
	if ( itCurrent != artilleries.end() )
		itCurrent->second.pCatchingFormation = pFormation;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFreeArtilleryHolder::OnSerialize( IBinSaver &f )
{
	if ( f.IsReading() )
		ResetIterator();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*								CGeneralIntendant*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CGeneralIntendant::CGeneralIntendant( const int nParty, CCommander *pGeneral)
: nParty( nParty ), pGeneral( pGeneral ), bInitedByParcel( false )
{
	cells.SetSizes( GetAIMap()->GetSizeX() * SConsts::TILE_SIZE / SConsts::GENERAL_CELL_SIZE + 1,
									GetAIMap()->GetSizeY() * SConsts::TILE_SIZE / SConsts::GENERAL_CELL_SIZE + 1 );
	// Switched sizes (X and Y) in an attempt to evade assert - Stas
	//Init();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::Give( CCommonUnit *pWorker )
{
	NI_ASSERT( dynamic_cast<CAIUnit*>( pWorker ) != 0, "not mech unit" );
	resupplyTrucks.push_back( checked_cast<CAIUnit*>( pWorker ) );

	// send worker to nearest reinforcement position.
	int nBest = 0;
	float fBestDist = 0;
	const CVec2 vWorkerPos ( pWorker->GetCenterPlain() );

	for ( int i = 0; i < vPositions.size(); ++i )
	{
		const float fCurDist = fabs2( vWorkerPos - vPositions[i].first );
		if ( nBest == i || fCurDist < fBestDist )
		{
			fBestDist = fCurDist;
			nBest = i;
		}
	}
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_MOVE_TO, vPositions[nBest].first), pWorker, false );
	const CVec2 vLookPoint( vPositions[nBest].first + GetVectorByDirection( vPositions[nBest].second ) );
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_ROTATE_TO, vLookPoint), pWorker, true );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::AddReiforcePositions( const struct NDb::SAIGeneralParcel &patchInfo )
{
	bInitedByParcel = true;
	const int nFormer = vPositions.size();
	
	vPositions.resize( vPositions.size() + patchInfo.reinforcePoints.size() );
	
	for ( int i = 0; i < patchInfo.reinforcePoints.size(); ++i )
	{
		const CVec2 vTransReinforcePoint ( patchInfo.reinforcePoints[i].vCenter.y, -patchInfo.reinforcePoints[i].vCenter.x );
		const CVec2 vReinforcePoint( patchInfo.vCenter + ( vTransReinforcePoint ^ GetVectorByDirection(patchInfo.GetDir()) ) );

		vPositions[nFormer+i].first = vReinforcePoint;
		vPositions[nFormer+i].second = patchInfo.GetDir() + patchInfo.reinforcePoints[i].GetDir();
	}
	if ( vPositions.empty() )
		vPositions.push_back( CPosition( patchInfo.vCenter, patchInfo.GetDir() ) );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::AddReiforcePosition( const CVec2 & vPos, const WORD wDirection )
{
	if ( !bInitedByParcel )
		vPositions.push_back( CPosition( vPos, wDirection ) );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::Init()
{
	SEnumStorages en;
	theStatObjs.EnumStoragesForParty( nParty, &en );
	for ( Storages::iterator storIter = en.storages.begin(); storIter != en.storages.end(); ++storIter )
	{
		CGeneralTaskToDefendStorage * pTask = new CGeneralTaskToDefendStorage( *storIter, nParty );
		tasks.push_back( pTask );
	}
	
	theStatObjs.EnumStoragesForParty( 1 - nParty, &en );
	for ( Storages::iterator storIter = en.storages.begin(); storIter != en.storages.end(); ++storIter )
	{
		CGeneralTaskToDefendStorage * pTask = new CGeneralTaskToDefendStorage( *storIter, nParty );
		tasks.push_back( pTask );
	}

	for ( int x = 0; x < cells.GetSizeX(); ++x )
	{
		for ( int y = 0; y < cells.GetSizeY(); ++y )
		{
			cells[y][x] = new CResupplyCellInfo;
			cells[y][x]->Init( SVector( x, y ) );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::EnumWorkers( const EForceType eType, IWorkerEnumerator *pEnumerator )
{
	EnumWorkersInternal( eType, pEnumerator, &resupplyTrucks );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CGeneralIntendant::RequestForSupport( const CVec2 &vSupportCenter, enum EForceType eType )
{
	return pGeneral->RequestForSupport( vSupportCenter, eType );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::CancelRequest( int nRequestID, enum EForceType eType )
{
	pGeneral->CancelRequest( nRequestID, eType );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::UnitDead( class CCommonUnit * pUnit )
{
	if ( !IsUnitRegistered( pUnit ) )	return;

	ResupplyCells::iterator pFormerCell = GetCell( pUnit->GetCenterPlain() );
	pFormerCell->second->RemoveUnit( pUnit );
	if ( pFormerCell->second->IsEmpty() )
		cellsWithRequests.erase( pFormerCell );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::UnitChangedPosition( class CCommonUnit *pUnit, const CVec2 &vNewPos )
{
	if ( !IsUnitRegistered( pUnit ) ) return;

	ResupplyCells::iterator pFormerCell = GetCell( pUnit->GetCenterPlain() );
	ResupplyCells::iterator pNewCell = GetCell( vNewPos );
	NI_ASSERT( pFormerCell != pNewCell, "cannot move to the same cell" );
	
	pNewCell->second->AddUnit( pUnit, pFormerCell->second->RemoveUnit( pUnit ) );
	if ( pFormerCell->second->IsEmpty() )
		cellsWithRequests.erase( pFormerCell );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::SetArtilleryVisible( const CAIUnit *pArtillery, const bool bVisible )
{
	/*
	const int nID = pArtillery->GetUniqueId();
	CFreeArtillery::iterator it = freeArtillery.find( nID );
	if ( it != freeArtillery.end() )
		it->second->SetVisible( pArtillery, bVisible );
	*/
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::UnitAskedForResupply( class CCommonUnit * pUnit, const enum EResupplyType eType, const bool bSet )
{
	if ( eType == ERT_HUMAN_RESUPPLY && !pUnit->IsFormation() )
	{
		CDynamicCast<CArtillery> pArtillery = pUnit;
		if ( pArtillery )
		{
			if ( bSet )
				freeArtilleryHolder.AddArtillery( pArtillery );
			else
				freeArtilleryHolder.RemoveArtillery( pArtillery );

			return;
		}
	}


	if ( bSet )
	{
		// remember artillery for human resupply
		if ( eType == ERT_HUMAN_RESUPPLY && !pUnit->IsFormation() )
		{
			CAIUnit * pAIUnit = checked_cast<CAIUnit*>( pUnit );
			NI_ASSERT( pAIUnit->GetStats()->IsArtillery(), "NOT SQUIAD AND NOT ARTILLERY ASKED FOR HUMAH RESUPPLY" );
			if ( pAIUnit->IsVisible( nParty ) )
			{
				ResupplyCells::iterator pCell = GetCell( pAIUnit->GetCenterPlain() );

				pCell->second->AddUnitResupply( pUnit, eType );	
			}
		}
		else
		{
			ResupplyCells::iterator pCell = GetCell( pUnit->GetCenterPlain() );
			pCell->second->AddUnitResupply( pUnit, eType );
		}
	}
	else 
	{
		if ( IsUnitRegistered( pUnit ) )
		{
			ResupplyCells::iterator pFormerCell = GetCell( pUnit->GetCenterPlain() );
			pFormerCell->second->RemoveUnitResupply( pUnit, eType );
			if ( pFormerCell->second->IsEmpty() )
				cellsWithRequests.erase( pFormerCell );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CGeneralIntendant::IsUnitRegistered( CCommonUnit * pUnit ) const
{
	const SVector vCell( AICellsTiles::GetGeneralCell( pUnit->GetCenterPlain() ) );
	ResupplyCells::const_iterator it = cellsWithRequests.find( vCell );
	return cellsWithRequests.end() != it && it->second->IsUnitRegistered( pUnit );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CGeneralIntendant::ResupplyCells::iterator CGeneralIntendant::GetCell( const CVec2 &vPos )
{
	// if the cell with unit exists, them return int
	// if not, create new and return

	SVector vCell( AICellsTiles::GetGeneralCell( vPos ) );
	vCell.x = Clamp( vCell.x, 0, cells.GetSizeX()-1 );
	vCell.y = Clamp( vCell.y, 0, cells.GetSizeY()-1 );
	if ( !cells[vCell.y][vCell.x] )
	{
		cells[vCell.y][vCell.x] = new CResupplyCellInfo;
		cells[vCell.y][vCell.x]->Init( vCell );
	}
	cellsWithRequests[vCell] = cells[vCell.y][vCell.x];
	return cellsWithRequests.find( vCell );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::SearchCrewForArtillery()
{
	freeArtilleryHolder.Segment();
	CPtr<CArtillery> pArtillery = freeArtilleryHolder.Get();
	if ( !IsValid( pArtillery ) || pArtillery->HasServeCrew() )	
		return;
	const CVec2 vArtCenter( pArtillery->GetCenterPlain() );

	CPtr<CFormation> pBestFormation = 0;
	float fBestDist2 = 0.0f;
	for ( CUnitsIter<0,0> iter( nParty, EDI_FRIEND, vArtCenter, theWarFog.GetMaxRadius() * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE ); !iter.IsFinished(); iter.Iterate() )
	{
		CDynamicCast<CSoldier> pSoldier = *iter;
		if ( pSoldier )
		{
			const float fSightRadius2 = sqr( pSoldier->GetSightRadius() );
			if ( fabs2( vArtCenter - pSoldier->GetCenterPlain() ) < fSightRadius2 )
			{
				CPtr<CFormation> pThisFormation = pSoldier->GetFormation();
				if ( pThisFormation && pThisFormation->CanCatchArtillery( pArtillery ) )
				{
					const float fThisDist2 = fabs2( pThisFormation->GetCenterPlain() - vArtCenter );
					if ( pBestFormation == 0 || fThisDist2 < fBestDist2 )
					{
						fBestDist2 = fThisDist2;
						pBestFormation = pThisFormation;
					}
				}
			}
		}
	}
	if ( pBestFormation )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_CATCH_ARTILLERY, pArtillery->GetUniqueID(), 1.0f ), pBestFormation, false );
		freeArtilleryHolder.TryCatch( pBestFormation );
		//DebugTrace( "formation %d try take artillery %d (dist2: %2.3f)", pBestFormation->GetUniqueID(), pArtillery->GetUniqueID(), fBestDist2 );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::Segment()
{
	CCommander::Segment();
	SGeneralHelper::RemoveDead( &resupplyTrucks );

	const BYTE cResupplyAll = ((1<<_ERT_COUNT)-1);
	BYTE cResupplyPossible = 0;

	if ( !cellsWithRequests.empty() )
	{
		// check what resupply actions are possible
		for ( CommonUnits::const_iterator it = resupplyTrucks.begin();
						it != resupplyTrucks.end() && cResupplyAll != cResupplyPossible; ++it )
		{
			CCommonUnit * pUnit = *it;
			for ( int i = 0; i < _ERT_COUNT; ++i )
			{
				if ( CResupplyCellInfo::IsUnitSuitable( pUnit, static_cast<EResupplyType>(i) ) )
					cResupplyPossible |= (1<<i);
			}
		}
	}

	if ( !cellsWithRequests.empty() && cResupplyPossible ) 
	{
		// prioritize tasks to resupply
		vector< CPtr<CResupplyCellInfo> > cellsToSort;
		cellsToSort.resize( cellsWithRequests.size() );
		int i = 0;
		for ( ResupplyCells::iterator it = cellsWithRequests.begin(); it != cellsWithRequests.end(); ++it )
			cellsToSort[i++] = it->second;
		CResupplyCellInfo::SSortByResupplyMaskPredicate pr( cResupplyPossible );
		sort( cellsToSort.begin(), cellsToSort.end(), pr );
		// done, cellsToSort now contains prioritized tasks

		// check if some arrays need some kind of resupply.
		int nTasksCreated = 0; //INTENDANT_TASKS_PER_SEGMENT
		for ( int nCell = 0; nCell < cellsToSort.size() && nTasksCreated <INTENDANT_TASKS_PER_SEGMENT; ++nCell )
		{
			for ( int i = 0; i < _ERT_COUNT && nTasksCreated <INTENDANT_TASKS_PER_SEGMENT; ++i )
			{
				CResupplyCellInfo * pCell = cellsToSort[nCell];
				if ( !pCell->IsDangerous() &&
							(cResupplyPossible&(1<<i)) && 
						 !pCell->IsMarkedUnderSupply( static_cast<EResupplyType>(i) ) &&
						 pCell->GetNNeeded( static_cast<EResupplyType>(i) ) > 0.0f )
				{
					++nTasksCreated;
					IGeneralTask * pTask = new CGeneralTaskToResupplyCell( pCell, nParty, static_cast<EResupplyType>(i), this );
					tasks.push_back( pTask );
				}
			}
		}
	}
	SearchCrewForArtillery();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGeneralIntendant::MarkCellsDangerous( const SVector &vCell )
{
	const int nRadius = SGeneralConsts::INTENDANT_DANGEROUS_CELL_RADIUS / ( SConsts::GENERAL_CELL_SIZE ) + 1;
	const NTimer::STime timeDanger = curTime + 
								1000 * (SGeneralConsts::RESUPPLY_CELL_AFTER_TRANSPORT_DEATH +
									NRandom::Random( SGeneralConsts::RESUPPLY_CELL_AFTER_TRANSPORT_DEATH_RAND ) );


	// mark as dangerous cells in given radius from given cell
	const int nStartX = Clamp( vCell.x - nRadius, 0, cells.GetSizeX() );
	const int nStartY = Clamp( vCell.y - nRadius, 0, cells.GetSizeY() );
	const int nFinishX = Clamp( vCell.x + nRadius, 0, cells.GetSizeX() );
	const int nFinishY = Clamp( vCell.y + nRadius, 0, cells.GetSizeY() );

	for ( int x = nStartX; x < nFinishX; ++x )
	{
		for ( int y = nStartY; y < nFinishY; ++y )
		{
			cells[y][x]->SetDanger( timeDanger );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
