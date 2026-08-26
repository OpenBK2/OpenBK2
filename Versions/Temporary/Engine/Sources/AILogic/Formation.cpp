#include "stdafx.h"

#include "Artillery.h"
#include "Soldier.h"
#include "Formation.h"
#include "Commands.h"
#include "Units.h"
#include "FormationStates.h"
#include "Guns.h"
#include "GroupLogic.h"
#include "Building.h"
#include "Entrenchment.h"
#include "Technics.h"
#include "NewUpdater.h"
#include "Diplomacy.h"
#include "AckManager.h"
#include "UnitCreation.h"
#include "Statistics.h"
#include "General.h"

#include "AILogicInternal.h"
#include "TimeCounter.h"

#include "Common_RTS_AI/CommonPathFinder.h"
#include "Common_RTS_AI/InFormationPath.h"
#include "Common_RTS_AI/StandartPath2.h"
#include "Common_RTS_AI/StaticPathInternal.h"
#include "Common_RTS_AI/PathFinder.h"
#include "Common_RTS_AI/StaticMapHeights.h"

#include "System/Commands.h"

#include "AILogic_export.h"

#include <cstdint>

#include <fmt/format.h>

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1108D498, CFormation );

enum ECatchArtilleryType
{
	EAT_GUN					= 0x00000001,
	EAT_HOWITZER		= 0x00000002,
	EAT_HEAVY_GUN		= 0x00000004,
	EAT_AA_GUN			= 0x00000008,
	EAT_ROCKET			= 0x00000010,
	EAT_SUPER				= 0x00000020,
	EAT_MORTAR			= 0x00000040,
	EAT_HEAVY_MG		= 0x00000080,
};

extern CSupremeBeing theSupremeBeing;
extern CAckManager theAckManager;
extern NTimer::STime curTime;
extern CUnits units;
extern CGroupLogic theGroupLogic;
extern CEventUpdater updater;
extern CDiplomacy theDipl;
extern CUnitCreation theUnitCreation;
extern CStatistics theStatistics;
extern CDifficultyLevel theDifficultyLevel;

extern NAI::CTimeCounter timeCounter;
//
static bool bDrawPathMarkers = false;

CAIUnit* CFormation::CCarryedMortar::CreateMortar( class CFormation *pOwner )
{
	NI_ASSERT( bHasMortar, "formation doesn't have mortar");
	CVec3 vPos( (*pOwner)[0]->GetCenter() );
	
	pOwner->SetCenter( vPos );
	const int nUniqueID = ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID;
	const int nID = theUnitCreation.AddNewUnit( nUniqueID, pStats, fHP / pStats->fMaxHP, vPos.x, vPos.y, vPos.z,
		pOwner->GetDirection(), theDipl.GetNeutralPlayer(), false, true, (*pOwner)[0]->GetReinforcementType() );

	CAIUnit *pMortar = checked_cast<CAIUnit*>( CLinkObject::GetObjectByUniqueId( nID ) );
	/*
	const int nPassClass = pMortar->GetStats()->nAIPassabilityClass;
	const int nBoundTileRadius = pMortar->GetBoundTileRadius();
	pMortar->UnlockTiles();
	bool bCanPlace = true;
	if ( !GetTerrain()->CanUnitGo( nBoundTileRadius, AICellsTiles::GetTile( CVec2(vPos.x, vPos.y) ), EAIClasses(nPassClass) ) )
	{
		bCanPlace = false;
		// find first unlocked place and move mortar here
		RecordRandomCall();
		const uint16_t wRandStart = NRandom::Random( 65535 );
		for ( int nDistance = 0; nDistance < 50 && !bCanPlace; nDistance += 2 )
		{
			for ( int nAngle = 0; nAngle < 8 && !bCanPlace; ++nAngle )
			{
				const CVec3 vNewPos( vPos + CVec3(GetVectorByDirection( nAngle * 65535 / 8 * nDistance ) * nDistance * SConsts::TILE_SIZE, 0 ) );
				if ( GetTerrain()->CanUnitGo( nBoundTileRadius, AICellsTiles::GetTile( vNewPos.x, vNewPos.y ), EAIClasses(nPassClass) ) )
				{
					vPos = GetHeights()->Get3DPoint( CVec2( vNewPos.x, vNewPos.y ) );
					bCanPlace = true;
				}
			}
		}
	}
	pMortar->SetCenter( vPos, true );
	*/
	bHasMortar = false;
	CAIUnit * pArt = checked_cast<CAIUnit *>( GetObjectByUniqueIdSafe( nID ) );
	for ( int i = 0; i < pArt->GetNCommonGuns(); ++i )
		pArt->GetGun( i )->SetNAmmo( ammo[i] );
	pArt->CheckAmmoStatus();
	updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, pArt, -1 );
	updater.AddUpdate( 0, ACTION_NOTIFY_SERVED_ARTILLERY, pOwner, pArt->GetUniqueId() );
	return pArt;
}

void CFormation::CCarryedMortar::Init( const class CAIUnit *pArt )
{
	NI_ASSERT( pArt->GetStats()->etype == RPG_TYPE_ART_MORTAR || pArt->GetStats()->etype == RPG_TYPE_ART_HEAVY_MG, "foramtion attempted to take not mortar");
	bHasMortar = true;
	fHP = pArt->GetHitPoints();
	pStats = pArt->GetStats();
	ammo.clear();
	for ( int i = 0; i < pArt->GetNCommonGuns(); ++i )
		ammo.push_back( pArt->GetGun( i )->GetNAmmo() );
}

//*******************************************************************
//*													CFormation															*
//*******************************************************************

//BASIC_REGISTER_CLASS( CFormation );

CFormation::CFormation()
: nInUnitsID(-1), pObjInside( 0 ), bWithMoraleOfficer( false ), bUsedCharge( false ), pCharge( 0 ), 
fSpeedCoeff( 1.0f ), nBoundTileRadius( 0 ), timeLastCatchArt( 0 ), dwCatchArtFlag( 0 ),
fPass( 0.0f ), timeToCamouflage( 0 ), cPlayer( 0 ), bWaiting( false ), bDisabled( false ),
eInsideType( EOIO_NONE ), fMaxFireRange( 0.0f ), nVirtualUnits( 0 ), bCanBeResupplied( false ),
bBoredInMoveFormationSent( false ), lastBoredInMoveFormationCheck( 0 ), fMaxSpeed( 0.0f ), vAABBHalfSize( VNULL2 )
{
}

void CFormation::AddSoldier( class CSoldier *pUnit )
{
	soldiers.push_back( pUnit );
	SetAlive( Size() != 0 || VirtualUnitsSize() != 0 );
	pGroupSmoothPath->AddUnit( pUnit, -1 );
	UpdateStats( pUnit, Size()-1 );

	const NDb::SUnitStatsModifier *pFormerMod = pStats->formations[GetCurrentGeometry()].pStatsModifiers;
	pUnit->ApplyStatsModifier( pFormerMod, true );
}

void CFormation::UpdateStats( class CSoldier *pUnit, const int nPos )
{
	if ( pUnit->GetStats()->etype == RPG_TYPE_OFFICER ||
		pUnit->GetStats()->etype == RPG_TYPE_ENGINEER ||
		pUnit->GetStats()->etype == RPG_TYPE_SNIPER )
		bWithMoraleOfficer = true;

	if ( pUnit->GetMemFormation() != this )
		pUnit->SetFormation( this );

	if ( pUnit->GetStats()->fSpeed < GetMaxPossibleSpeed() )
		SetMaxSpeed( pUnit->GetStats()->fSpeed );
	if ( pUnit->GetBoundTileRadius() > GetBoundTileRadius() )
		SetBoundTileRadius( pUnit->GetBoundTileRadius() );
	if ( pUnit->GetAABBHalfSize().x > GetAABBHalfSize().x )
		SetAABBHalfSize( pUnit->GetAABBHalfSize() );

	if ( pUnit->GetPassability() < fPass )
		fPass = pUnit->GetPassability();

	if ( pUnit->GetTimeToCamouflage() > timeToCamouflage )
		timeToCamouflage = pUnit->GetTimeToCamouflage();
	if ( pUnit->GetMaxFireRange() > fMaxFireRange )
		fMaxFireRange = pUnit->GetMaxFireRange();

	for ( int i = 0; i < availCommands.GetSize(); ++i )
	{
		if ( pUnit->GetStats()->HasCommand( i ) )
			availCommands.SetData( i );
	}

	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
		guns.push_back( SGunInfo( nPos, i ) );

	if ( cPlayer == 255 )
		cPlayer = pUnit->GetPlayer();
	else if ( cPlayer != pUnit->GetPlayer() )
		pUnit->ChangePlayer( cPlayer );

	theSupremeBeing.UnitAskedForResupply( this, ERT_HUMAN_RESUPPLY, Size() != pStats->members.size() );
}

void CFormation::AddNewUnitToSlot( CSoldier *pUnit, const bool bSendToWorld )
{
	AddSoldier( pUnit );
//	pGroupSmoothPath->AlignGeometriesToCenter();

	if ( bSendToWorld )
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_FORMATION, pUnit, -1 );
}

const int CFormation::GetUnitSlotInStats( const uint8_t cSlot ) const
{
	return 0;
	/*
	NI_ASSERT( cSlot < Size(), "Wrong unit position" ); 
	NI_ASSERT( units[cSlot].nSlotInStats != -1, "Non initialized stats-slot of unit" );
	return units[cSlot].nSlotInStats;
	*/
}

void CFormation::InitGeometries()
{
	for ( int i = 0; i < GetStats()->formations.size(); ++i )
	{
		std::vector<SGeometryCellInfo> cells;
		for ( int j = 0; j < GetStats()->formations[i].order.size(); ++j )
			cells.push_back( SGeometryCellInfo( pStats->formations[i].order[j].vPos, -1 ) );
		pGroupSmoothPath->AddGeometry( cells );
	}
}

ISmoothPath *CFormation::CreateSmoothPath()
{
	ISmoothPath *result = new CGroupSmoothPath();
	result->Init( this, CreatePathByDirection( GetCenterPlain(), CVec2( 1, 1 ), GetCenterPlain(), GetAIMap() ), !IsInfantry(), true, GetAIMap() );
	pGroupSmoothPath = checked_cast<CGroupSmoothPath*>(result);
	return result;
}

void CFormation::Init( const SSquadRPGStats *_pStats, const CVec2 &center, const int z, const uint16_t dir, ICollisionsCollector *pCollisionsCollector )
{
	fMaxSpeed = 666.0;
	SetUniqueIdForUnits( ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID );

	bCanBeResupplied = true;
	pStats = _pStats;
	cPlayer = 0xff;
	bWaiting = false;
	availCommands.SetSize( 128 );
	availCommands.FillZero();
	timeToCamouflage = 0;
	fPass = 1;
	bDisabled = false;
	availCommands.SetData( ACTION_COMMAND_DISBAND_FORMATION );
	availCommands.SetData( ACTION_COMMAND_ROTATE_TO );
	vAABBHalfSize = VNULL2;
	for ( int i = 0; i < availCommands.GetSize(); ++i )
	{
		if ( pStats->HasCommand( i ) )
			availCommands.SetData( i );
	}

	eInsideType = EOIO_NONE;
	pObjInside = 0;
	fMaxFireRange = 0.0f;
	nVirtualUnits = 0;
	SetAlive( Size() != 0 || VirtualUnitsSize() != 0 );
	bBoredInMoveFormationSent = false;
	lastBoredInMoveFormationCheck = 0;
	bWithMoraleOfficer = false;

	CCommonUnit::Init( CVec3( center, z ), dir, pCollisionsCollector );
	Mem2UniqueIdObjs();

	::units.AddFormation( this );
	SetPrice( GetStats()->fReinforcementPrice );

	InitGeometries();

	theGroupLogic.RegisterSegments( this, dynamic_cast<CAILogic*>(Singleton<IAILogic>())->IsFirstTime(), true );
}

void CFormation::Segment()
{
	if ( !bDisabled )
	{
		CQueueUnit::Segment();

		//if ( !IsValidObj( pCharge ) )
		//DetonateCharge();

		if ( pGroupSmoothPath !=0 && !pGroupSmoothPath->IsFinished() )
		{
			pGroupSmoothPath->Segment( SConsts::AI_SEGMENT_DURATION );

			if ( Size()>1 )
			{
				fMaxSpeed = (*this)[0]->GetMaxSpeedHere();
				for ( int i = 1; i < Size(); ++i )  
					fMaxSpeed = (std::min)( fMaxSpeed, (*this)[i]->GetMaxSpeedHere() );
			}
		}

		if ( timeLastCatchArt > 0 )
		{
			if ( timeLastCatchArt < SAIConsts::AI_SEGMENT_DURATION )
				timeLastCatchArt = 0;
			else
				timeLastCatchArt -= SAIConsts::AI_SEGMENT_DURATION;
		}
	}
}

IStatesFactory* CFormation::GetStatesFactory() const
{
	return CFormationStatesFactory::Instance();
}

const bool CFormation::IsIdle() const
{
	if ( pGroupSmoothPath ==0 )
		return true;
	else if ( pGroupSmoothPath->IsFinished() )
	{
		for ( int i = 0; i < Size(); ++i )
		{
			if ( !(*this)[i]->IsIdle() )
				return false;
		}
		return true;
	}
	return false;
}

void CFormation::BalanceCenter()
{
	if ( Size() > 0 ) 
	{
		CVec2 vNewCenter( VNULL2 );
		for ( int i = 0; i < Size();	++i )
			vNewCenter += (*this)[i]->GetCenterPlain();

		vNewCenter /= Size();
		SetCenter( CVec3( vNewCenter, GetCenter().z ) );
	}
}

void CFormation::Stop()
{
	pGroupSmoothPath->Init( this, CreatePathByDirection( GetCenterPlain(), CVec2( 1, 1 ), GetCenterPlain(), GetAIMap() ), !IsInfantry(), true, GetAIMap() );
}

void CFormation::StopTurning()
{
}

bool CFormation::CanCommandBeExecuted( class CAICommand *pCommand )
{
	const int &nCmd = pCommand->ToUnitCmd().nCmdType;
	// проверять на возможность исполнения только user команды
	if ( nCmd < 1000 )
	{
		NI_ASSERT( nCmd >= 0 && nCmd < availCommands.GetSize(), fmt::format( "Wrong command ( {} )\n", nCmd ) );
		if ( !availCommands.GetData( nCmd ) )
			return false;
	}

	if ( nCmd == ACTION_COMMAND_FORM_FORMATION )
	{
		// нельзя собрать, если не single formation
		if ( Size() > 1 )
			return false;

		// нельзя собрать, если раньше не была разобрана
		CFormation *pOldFormation = (*this)[0]->GetMemFormation();
		if ( pOldFormation == 0 )
			return false;

		// нельзя собрать, если кто-то в транспорте
		for ( int i = 0; i < pOldFormation->Size(); ++i )
		{
			if ( (*pOldFormation)[i]->IsInTransport() )
				return false;
		}
	}

	if ( nCmd == ACTION_COMMAND_DISBAND_FORMATION )
	{
		if ( Size() <= 1 )
			return false;
	}

	return true;
}

bool CFormation::CanCommandBeExecutedByStats( int nCmd ) const
{
	// проверять на возможность исполнения только user команды
	if ( nCmd < 1000 )
	{
		NI_ASSERT( nCmd >= 0 && nCmd < availCommands.GetSize(), fmt::format( "Wrong command ( {} )", nCmd ) );
		if ( !availCommands.GetData( nCmd ) )
			return false;
	}

	return true;
}

bool CFormation::CanCommandBeExecutedByStats( class CAICommand *pCommand )
{
	return CanCommandBeExecutedByStats( pCommand->ToUnitCmd().nCmdType );
}

bool CFormation::IsEveryUnitInTransport() const
{
	for ( int i = 0; i < Size(); ++i )
		if ( !(*this)[i]->IsInTransport() )
			return false;
	return true;

}

bool CFormation::IsMemberResting( CSoldier *pSoldier ) const
{
	IUnitState *pState = pSoldier->GetState();
	return
		pState && pState->IsRestState() && pSoldier->IsEmptyCmdQueue();
}

bool CFormation::IsAnyUnitResting() const
{
	for ( int i = 0; i < Size(); ++i )
		if ( IsMemberResting( (*this)[i] ) )
			return true;
	return false;
}

bool CFormation::IsEveryUnitResting() const
{
	for ( int i = 0; i < Size(); ++i )
		if ( !IsMemberResting( (*this)[i] ) )
			return false;
	return true;

}

CBasicGun* CFormation::GetGun( const int n ) const
{
	return (*this)[guns[n].nUnit]->GetGun( guns[n].nUnitGun );
}

CBasicGun* CFormation::ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime )
{
	return 0;
}

const float CFormation::GetSightRadius() const
{
	float fResult = 0;
	for ( int i = 0; i < Size(); ++i )
		fResult = (std::max)( fResult, (*this)[i]->GetSightRadius() );

	return fResult;
}

void CFormation::Disable() 
{ 
	bDisabled = true;
	theSupremeBeing.UnitAskedForResupply( this, ERT_HUMAN_RESUPPLY, false );
}

void CFormation::Enable() 
{ 
	bDisabled = false; 
	if ( Size() != pStats->members.size() )
		theSupremeBeing.UnitAskedForResupply( this, ERT_HUMAN_RESUPPLY, true );
}

void CFormation::WasHitNearUnit()
{
	if ( pStats->formations[GetCurrentGeometry()].changesByEvent[0] != -1 && 
		pStats->formations[GetCurrentGeometry()].changesByEvent[0] != GetCurrentGeometry() )
		ChangeGeometry( pStats->formations[GetCurrentGeometry()].changesByEvent[0] );
}

void CFormation::SetSelectable( bool bSelectable, bool bSendToWorld )
{
	CCommonUnit::SetSelectable( bSelectable, bSendToWorld );
	for ( int i = 0; i < Size(); ++i )
		(*this)[i]->SetSelectable( bSelectable, bSendToWorld );
}

void CFormation::ChangePlayer( const uint8_t _cPlayer )
{
	if ( cPlayer != _cPlayer )
	{
		cPlayer = _cPlayer;
		updater.AddUpdate( 0, ACTION_NOTIFY_UPDATE_DIPLOMACY, this, -1 );
		CFormation::SetSelectable( cPlayer == theDipl.GetMyNumber(), true );

		for ( int i = 0; i < Size(); ++i )
		{
			if ( (*this)[i]->GetPlayer() != cPlayer )
				(*this)[i]->ChangePlayer( cPlayer );
		}
	}
}

const bool CFormation::IsVisible( const uint8_t party ) const
{
	for ( int i = 0; i < Size(); ++i )
	{
		if ( (*this)[i]->IsVisible( party ) )
			return true;
	}

	return false;
}

const bool CFormation::CanShootToPlanes() const
{
	for ( int i = 0; i < Size(); ++i )
	{
		if ( (*this)[i]->CanShootToPlanes() )
			return true;
	}

	return false;
}

/*void CFormation::SetAmbush()
{
	for ( int i = 0; i < Size(); ++i )
		units[i].pUnit->SetAmbush();
}*/

/*void CFormation::RemoveAmbush()
{
	for ( int i = 0; i < Size(); ++i )
		units[i].pUnit->RemoveAmbush();
}*/

void CFormation::SetCamoulfage()
{
	for ( int i = 0; i < Size(); ++i )
		(*this)[i]->SetCamoulfage();
}

void CFormation::RemoveCamouflage( ECamouflageRemoveReason eReason )
{
	for ( int i = 0; i < Size(); ++i )
		(*this)[i]->RemoveCamouflage( eReason );
}

const NTimer::STime CFormation::GetTimeToCamouflage() const
{
	return timeToCamouflage;
}

void CFormation::UpdateArea( const EActionNotify eAction )
{
	for ( int i = 0; i < Size(); ++i )
		(*this)[i]->UpdateArea( eAction );
}

void CFormation::PrepareToDelete()
{
	GetState()->TryInterruptState( 0 );

	theGroupLogic.DelUnitFromGroup( this );
	theGroupLogic.DelUnitFromSpecialGroup( this );
	DelCmdQueue( GetUniqueId() );
	
	dynamic_cast<CAILogic*>(Singleton<IAILogic>())->ToGarbage( this );
	::units.DelFormation( this );

	updater.AddUpdate( 0, ACTION_NOTIFY_DISSAPEAR_OBJ, this, -1 );

	theSupremeBeing.UnitDied( this );
	theGroupLogic.UnregisterSegments( this );
}

void CFormation::Disappear()
{
	// т.к. последний юнит формации вызовет эту функцию, то общее удаление формации выполняется только тогда
	if ( Size() != 0 )
	{
		while ( Size() != 0 )
			(*this)[0]->Disappear();
	}
	else
		PrepareToDelete();
}

void CFormation::Die( const bool fromExplosion, const float fDamage )
{
	// т.к. последний юнит формации вызовет эту функцию, то общее удаление формации выполняется только тогда	
	if ( Size() != 0 )
	{
		while ( Size() != 0 )
			(*this)[0]->Disappear();
	}
	else
	{
		PrepareToDelete();
		theStatistics.UnitDead( this );
	}
}

const bool CFormation::SendAlongPath( IStaticPath *pStaticPath, const CVec2 &vShift, bool bSmoothTurn )
{
	{
		const CCommonStaticPath *pPath = dynamic_cast<const CCommonStaticPath *>(pStaticPath);

#ifndef _FINALRELEASE
		if ( bDrawPathMarkers ) 
		{
			if ( pPath )
			{
				std::vector<SVector> tiles;
				tiles.reserve( pPath->GetLength() );
				for ( int i = 0; i < pPath->GetLength(); ++i )
					tiles.push_back( pPath->GetTile( i ) );
				DebugInfoManager()->CreateMarker( NDebugInfo::OBJECT_ID_FORGET, tiles, NDebugInfo::RED );
			}
		}
#endif
	}

	if ( IsInTankPit() )
	{
		theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), this );
		return true;
	}
	else
	{
		for ( int i = 0; i < Size(); ++i )
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), (*this)[i], false );

		bool bResult = false;
		if ( pStaticPath )
		{
			const CVec2 vFinisnPoint = GetAIMap()->IsPointInside( pStaticPath->GetFinishPoint() + vShift ) ?
				pStaticPath->GetFinishPoint() + vShift : pStaticPath->GetFinishPoint();

//			if ( CCommonUnit::SendAlongPath( pStaticPath, vShift, bSmoothTurn ) )
			if ( pGroupSmoothPath->Init( this, new CStandartPath2( this, pStaticPath, GetCenterPlain(), vFinisnPoint, GetAIMap() ), Size() > 1, true, GetAIMap() ) )
			{
				for ( int i = 0; i < Size(); ++i )
				{
					CPtr<CInFormationPath> pNewPath = new CInFormationPath();
					pNewPath->Init( (*this)[i], pGroupSmoothPath, true, true, GetAIMap() );
					(*this)[i]->SendAlongSmoothPath( pNewPath );
				}
				return true;
			}
		}

		return false;
	}
}

const bool CFormation::SendAlongPath( IPath *pPath )
{
	for ( int i = 0; i < Size(); ++i )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), (*this)[i], false );
	CCommonUnit::SendAlongPath( pPath );

	for ( int i = 0; i < Size(); ++i )
		(*this)[i]->SendAlongPath( 0, VNULL2, true );

	return true;
}

void CFormation::SetGeometryPropertiesToSoldier( CSoldier *pSoldier, const bool bChangeWarFog )
{
	if ( pStats->formations[GetCurrentGeometry()].nLieFlag == 1 )
		pSoldier->StandUp();
	else if ( pStats->formations[GetCurrentGeometry()].nLieFlag == 2 )
		pSoldier->LieDown();

	if ( bChangeWarFog )
		pSoldier->WarFogChanged();
}

bool CFormation::IsAllowedLieDown() const
{
	return pStats->formations[GetCurrentGeometry()].nLieFlag != 1;
}

bool CFormation::IsAllowedStandUp() const
{
	return pStats->formations[GetCurrentGeometry()].nLieFlag != 2;
}

struct SEdge
{
	CFormation *pFormation;
	CAICommand *pCmd;
	float fDist;

	SEdge() : pFormation( 0 ), pCmd( 0 ), fDist( 0.0f ) { }

	operator float() const { return fDist; }
};

void CFormation::UnitCommand( CAICommand *pCommand, bool bPlaceInQueue, bool bOnlyThisUnitCommand )
{
	if ( !bPlaceInQueue && Size() == 1 && pCommand->ToUnitCmd().nCmdType < 1000 )
		(*this)[0]->SetWait2FormFlag( false );

	FreezeByState( false );

	if ( pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_PARADE )
		CPtr<IUnitState> pParadeState = CFormationParadeState::Instance( this, pCommand->ToUnitCmd().fNumber );
	else
	{
		if ( !bOnlyThisUnitCommand )
		{
			SetBehaviourMoving( SBehaviour::EMRoaming );

			for ( int i = 0; i < Size(); ++i )
			{
				CSoldier *pSoldier = (*this)[i];
				pSoldier->SetBehaviourMoving( SBehaviour::EMRoaming );
				pSoldier->UnlockTiles();
			}
		}

		CCommonUnit::UnitCommand( pCommand, bPlaceInQueue, bOnlyThisUnitCommand );
	}
}

void CFormation::DeleteSoldier( class CSoldier *pUnit )
{
	if ( pGroupSmoothPath->DeleteUnit( pUnit ) )  
	{
		for ( std::vector< CPtr<CSoldier> >::iterator it = soldiers.begin(); it != soldiers.end(); ++it )
		{
			if ( *it == pUnit )
			{
				pUnit->RestoreSmoothPath();
				soldiers.erase( it );
				SetAlive( Size() != 0 || VirtualUnitsSize() != 0 );
				const NDb::SUnitStatsModifier *pFormerMod = pStats->formations[GetCurrentGeometry()].pStatsModifiers;
				pUnit->ApplyStatsModifier( pFormerMod, true );
				break;
			}
		}
		availCommands.FillZero();
		availCommands.SetData( ACTION_COMMAND_DISBAND_FORMATION );
		AddAvailCmd( ACTION_COMMAND_FOLLOW );
		AddAvailCmd( ACTION_COMMAND_ROTATE_TO );

		fMaxFireRange = 0.0f;

		bWithMoraleOfficer = false;
		for ( int i = 0; i < Size(); ++i )
			UpdateStats( (*this)[i], i );

		if ( Size() > 0 )
			MoveGeometries2Center();
	}
}

void CFormation::SetFree() 
{ 
	eInsideType = EOIO_NONE; 
	pObjInside = 0;
}

void CFormation::SetInBuilding( class CBuilding *pBuilding ) 
{ 
	eInsideType = EOIO_BUILDING; 
	pObjInside = pBuilding;
}

void CFormation::SetInTransport(  class CMilitaryCar *pUnit ) 
{ 
	eInsideType = EOIO_TRANSPORT; 
	pObjInside = pUnit;
}

void CFormation::SetInEntrenchment( class CEntrenchment *pEntrenchment ) 
{ 
	eInsideType = EOIO_ENTRENCHMENT; 
	pObjInside = pEntrenchment;
}

CBuilding* CFormation::GetBuilding() const
{
	NI_ASSERT( IsInBuilding(), "Soldier isn't in a building" );
	return dynamic_cast_ptr<CBuilding*>( pObjInside );
}

CEntrenchment* CFormation::GetEntrenchment() const
{
	//NI_ASSERT( IsInEntrenchment(), "Soldier isn't in entrenchment" );
	return dynamic_cast_ptr<CEntrenchment*>( pObjInside );
}

CMilitaryCar* CFormation::GetTransportUnit() const
{
	NI_ASSERT( IsInTransport(), "Soldier isn't in a transport" );
	return dynamic_cast_ptr<CMilitaryCar*>( pObjInside );
}

void CFormation::GetNewUnitInfo( SNewUnitInfo *pNewUnitInfo )
{
	pNewUnitInfo->dir = GetDirection();
	pNewUnitInfo->center = GetCenterPlain();
	pNewUnitInfo->z = GetCenter().z;

	pNewUnitInfo->pStats = GetStats();
	pNewUnitInfo->eDipl = theDipl.GetDiplStatus( theDipl.GetMyNumber(), GetPlayer() );
	pNewUnitInfo->nFrameIndex = -1; //GetScenarioUnit() ? GetScenarioUnit()->GetScenarioID() : -1;
	pNewUnitInfo->nObjUniqueID = GetUniqueID();
	pNewUnitInfo->fHitPoints = 1.0f;
	pNewUnitInfo->fFuel = 0.0f;
	pNewUnitInfo->fResize = 1.0f;
	pNewUnitInfo->nPlayer = GetPlayer();
}

void CFormation::SendAcknowledgement( CAICommand *pCommand, EUnitAckType ack, bool bForce )
{
	if ( Size() > 0 && ( bForce || pCommand && !pCommand->IsFromAI() ) )
		(*this)[0]->SendAcknowledgement( pCommand, ack, theDipl.GetMyNumber() == GetPlayer() );
}

void CFormation::SendAcknowledgement( EUnitAckType ack, bool bForce )
{
	SendAcknowledgement( GetCurCmd(), ack, bForce );
}

const float CFormation::GetSightMultiplier() const
{
	return pStats->formations[GetCurrentGeometry()].fVisibleBonus;
}

EUnitAckType CFormation::GetGunsRejectReason() const
{
	EUnitAckType eBestReason = EUnitAckType( ACK_NONE );

	for ( int i = 0; i < Size(); ++i )
	{
		EUnitAckType eReason = (*this)[i]->GetGunsRejectReason();
		if ( eReason != ACK_NONE && int(eReason) < int(eBestReason) )
			eBestReason = eReason;
	}

	return ( eBestReason == ACK_NONE ) ? ACK_NEGATIVE : eBestReason;
}

const int CFormation::GetVirtualUnitSlotInStats( const int nVirtualUnit ) const
{
	NI_ASSERT( nVirtualUnit < VirtualUnitsSize(), fmt::format( "Wrong number of virtual unit ({}), size of virtual units ({})", nVirtualUnit, VirtualUnitsSize() ) );
	return virtualUnits[nVirtualUnit].nSlotInStats;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////v
void CFormation::AddVirtualUnit( CSoldier *pSoldier, const int nSlotInStats )
{
	if ( virtualUnits.size() <= nVirtualUnits )
		virtualUnits.resize( nVirtualUnits + 2 );

	virtualUnits[nVirtualUnits].pSoldier = pSoldier;
	virtualUnits[nVirtualUnits].nSlotInStats = nSlotInStats;

	++nVirtualUnits;
	SetAlive( Size() != 0 || VirtualUnitsSize() != 0 );

	pSoldier->SetVirtualFormation( this );
}

void CFormation::DelVirtualUnit( CSoldier *pSoldier )
{
	int i = 0;
	while ( i < VirtualUnitsSize() && virtualUnits[i].pSoldier != pSoldier )
		++i;

	NI_ASSERT( i < VirtualUnitsSize(), "Virtual unit not found" );

	for ( int j = i; j < nVirtualUnits - 1; ++j )
		virtualUnits[j] = virtualUnits[j+1];

	virtualUnits[nVirtualUnits-1].pSoldier = 0;
	--nVirtualUnits;
	SetAlive( Size() != 0 || VirtualUnitsSize() != 0 );

	pSoldier->SetVirtualFormation( 0 );
}

void CFormation::MakeVirtualUnitReal( CSoldier *pSoldier )
{
	int i = 0;
	while ( i < VirtualUnitsSize() && virtualUnits[i].pSoldier != pSoldier )
		++i;

	NI_ASSERT( i < VirtualUnitsSize(), "Virtual unit not found" );

	AddNewUnitToSlot( virtualUnits[i].pSoldier, true );
	DelVirtualUnit( pSoldier );

	SetGeometryPropertiesToSoldier( pSoldier, true );

	theSupremeBeing.UnitAskedForResupply( this, ERT_HUMAN_RESUPPLY, Size() != pStats->members.size() );
}

void CFormation::UnRegisterAsBored( const enum EUnitAckType eBoredType )
{
	for ( int i = 0; i < Size(); ++i )
	{
		(*this)[i]->UnRegisterAsBored( eBoredType );
	}
}

void CFormation::RegisterAsBored( const enum EUnitAckType eBoredType )
{
	for ( int i = 0; i < Size(); ++i )
	{
		(*this)[i]->RegisterAsBored( eBoredType );
	}
}

void CFormation::SetCarryedMortar( class CAIUnit *pMortar )
{
	mortar.Init( pMortar );
}

bool CFormation::HasMortar() const
{
	return mortar.HasMortar();
}

CAIUnit* CFormation::InstallCarryedMortar()
{
	return mortar.CreateMortar( this );
}

void CFormation::ResetTargetScan()
{
	for ( int i = 0; i < Size(); ++i )
		(*this)[i]->ResetTargetScan();
}

uint8_t CFormation::AnalyzeTargetScan(	CAIUnit *pCurTarget, const bool bDamageUpdated, bool bScanForObstacles, CObjectBase *pCheckBuilding )
{
	uint8_t cResult = 0;
	for ( int i = 0; i < Size(); ++i )
	{
		if ( IsMemberResting( (*this)[i] ) )
		{
			cResult |= (*this)[i]->AnalyzeTargetScan( pCurTarget, bDamageUpdated, pCheckBuilding );
		}
		else
		{
			cResult |= 1;
		}
	}

	return cResult;
}

void CFormation::LookForTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, CBasicGun **pGun )
{
	*pBestTarget = 0;
	*pGun = 0;
	for ( int i = 0; i < Size(); ++i )
	{
		(*this)[i]->LookForTarget( pCurTarget, bDamageUpdated, pBestTarget, pGun );
		if ( *pBestTarget != 0 )
			return;
	}
}

float CFormation::GetPriceMax() const
{
	float fPrice = 0;
	const int nOrderSize = pStats->members.size();
	for ( int i = 0; i < nOrderSize; ++i )
	{
		fPrice += pStats->members[i]->fPrice;
	}
	return fPrice;
}

const float CFormation::GetTargetScanRadius()
{
	float fRadius = 0.0f;
	for ( int i = 0; i < Size(); ++i )
	{
		const float fSoldierScanRadius = (*this)[i]->GetTargetScanRadius();
		if ( fSoldierScanRadius > fRadius )
			fRadius = fSoldierScanRadius;
	}

	return fRadius;
}

void CFormation::FreezeByState( const bool bFreeze )
{
	if ( !bFreeze )
	{
		for ( int i = 0; i < Size(); ++i )
			(*this)[i]->FreezeByState( bFreeze );
	}

	CCommonUnit::FreezeByState( bFreeze );
}

void CFormation::NotifyAbilityRun( class CAICommand * pCommand )
{
	if ( pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_FIRST_AID )
	{
		ProduceEventByAction( NDb::ABILITY_FIRST_AID, NDb::ESpecialAbilityParam(int(pCommand->ToUnitCmd().fNumber)), pCommand );
	}
	else
	{
		for ( int i = 0; i < Size(); ++i )
			(*this)[i]->NotifyAbilityRun( pCommand );
	}
}

const SRect & CFormation::GetUnitRect() const
{
	static SRect unitRect;
	const float fLength = GetAABBHalfSize().y * SConsts::COEFF_FOR_LOCK;
	const float fWidth = GetAABBHalfSize().x * ( SConsts::COEFF_FOR_LOCK + 0.01f );

	unitRect.InitRect( GetCenterPlain(), GetDirectionVector(), fLength, fWidth );

	return unitRect;
}

const SUnitProfile & CFormation::GetUnitProfile() const
{
	static SUnitProfile unitProfile;
	unitProfile = SUnitProfile( GetUnitRect() );
	return unitProfile;
}

bool CFormation::IsInTankPit() const
{
	for ( int i = 0; i < Size(); ++i )
	{
		if ( (*this)[i]->IsInTankPit() )
			return true;
	}
	return false;
}

void CFormation::ChangeGeometry( const int nGeometry )
{
	// remove soldier's bonus from former formation layout
	{
		const NDb::SUnitStatsModifier *pFormerMod = pStats->formations[GetCurrentGeometry()].pStatsModifiers;
		for ( int i = 0; i < Size(); ++i )
			(*this)[i]->ApplyStatsModifier( pFormerMod, false );
	}
	pGroupSmoothPath->ChangeGeometry( nGeometry ); 

	for ( int i = 0; i < Size(); ++i )
		SetGeometryPropertiesToSoldier( (*this)[i], true );

	// apply new bonus to all soldiers in formation
	{
		const NDb::SUnitStatsModifier *pFormerMod = pStats->formations[GetCurrentGeometry()].pStatsModifiers;
		for ( int i = 0; i < Size(); ++i )
			(*this)[i]->ApplyStatsModifier( pFormerMod, true );
	}
}

const int CFormation::GetGeometriesCount() const
{ 
	return pGroupSmoothPath->GetGeometriesCount();
}

const int CFormation::GetCurrentGeometry() const
{ 
	return pGroupSmoothPath->GetCurrentGeometry();
}

const CVec2 CFormation::GetUnitCoord( const CBasePathUnit *pSoldier ) const
{
	return pGroupSmoothPath->GetValidUnitCenter( pSoldier );
}

const CVec2 CFormation::GetUnitDir( const CBasePathUnit *pSoldier ) const
{
	return pGroupSmoothPath->GetUnitDirection( pSoldier );
}

const float CFormation::GetMaxProjection() const
{ 
	return pGroupSmoothPath->GetMaxProjection(); 
}

const float CFormation::GetRadius() const
{ 
	return pGroupSmoothPath->GetRadius(); 
}

void CFormation::MoveGeometries2Center()
{ 
	pGroupSmoothPath->AlignGeometriesToCenter();
}

void CFormation::StopFormationCenter()
{
	pGroupSmoothPath->Stop();
}

const NDb::SUnitSpecialAblityDesc * CFormation::GetUnitAbilityDesc( const NDb::EUnitSpecialAbility eType )
{
	for ( int i = 0; i < Size(); ++i )
	{
		if ( const NDb::SUnitSpecialAblityDesc *pSA = (*this)[i]->GetUnitAbilityDesc( eType ) )
			return pSA;
	}
	return 0;
}

const bool CFormation::CanCatchArtillery( const CArtillery *pArtillery ) const
{
	if ( !GetState() || !(GetState()->IsRestState()) )
		return false;

	uint32_t dwType = 0;
	switch( pArtillery->GetStats()->etype )
	{
	case NDb::RPG_TYPE_ART_GUN: dwType = EAT_GUN; break;
	case NDb::RPG_TYPE_ART_HOWITZER: dwType = EAT_HOWITZER; break;
	case NDb::RPG_TYPE_ART_HEAVY_GUN: dwType = EAT_HEAVY_GUN; break;
	case NDb::RPG_TYPE_ART_AAGUN: dwType = EAT_AA_GUN; break;
	case NDb::RPG_TYPE_ART_ROCKET: dwType = EAT_ROCKET; break;
	case NDb::RPG_TYPE_ART_SUPER: dwType = EAT_SUPER; break;
	case NDb::RPG_TYPE_ART_MORTAR: dwType = EAT_MORTAR; break;
	case NDb::RPG_TYPE_ART_HEAVY_MG: dwType = EAT_HEAVY_MG; break;
	}

	return ( dwType & dwCatchArtFlag ) != 0;
}

void CFormation::ResetCatchArtTimer()
{
	timeLastCatchArt = SConsts::FORMATION_CHECKFREEART_PERIOD;
}

void CFormation::QuickLoadToMechUnit( CAITransportUnit *pTransport )
{
	for ( int i = 0; i < Size(); ++i )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pTransport->GetUniqueID() ), (*this)[i], false );
		(*this)[i]->SetInTransport( pTransport );
		pTransport->AddPassenger( (*this)[i] );
	}
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pTransport->GetUniqueID() ), this, false );
}

START_REGISTER(FormationVars)
REGISTER_VAR_EX( "draw_path_marker", NGlobal::VarBoolHandler, &bDrawPathMarkers, 0, STORAGE_NONE );
FINISH_REGISTER

