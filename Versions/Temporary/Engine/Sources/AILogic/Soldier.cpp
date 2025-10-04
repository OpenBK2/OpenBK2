#include "stdafx.h"

#include "System/Time.h"
#include "Misc/bresenham.h"
#include "Soldier.h"
#include "SoldierStates.h"
#include "InBuildingStates.h"
#include "InTransportStates.h"
#include "NewUpdater.h"
#include "Entrenchment.h"
#include "Building.h"
#include "UnitsIterators2.h"
#include "HitsStore.h"
#include "Technics.h"
#include "UnitGuns.h"
#include "Formation.h"
#include "Shell.h"
#include "FormationStates.h"
#include "Commands.h"
#include "ShootEstimatorInternal.h"
#include "General.h"
#include "DifficultyLevel.h"
#include "Artillery.h"
#include "StaticObjectsIters.h"
#include "ExecutorContainer.h"
// for profiling
#include "TimeCounter.h"

#include "GroupLogic.h"
#include "Statistics.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "Graveyard.h"
#include "AILogicInternal.h"
#include "DBAIConsts.h"
#include "Shell.h"
#include "GlobalWarFog.h"

REGISTER_SAVELOAD_CLASS( 0x1108D4DC, CSniper);
REGISTER_SAVELOAD_CLASS( 0x1108D442, CInfantry );

extern CExecutorContainer theExecutorContainer;
extern CSupremeBeing theSupremeBeing;
extern CDiplomacy theDipl;
extern CEventUpdater updater;
extern NTimer::STime curTime;
extern CUnits units;
extern CHitsStore theHitsStore;
extern CGlobalWarFog theWarFog;
extern CStaticObjects theStatObjs;
extern CDifficultyLevel theDifficultyLevel;

extern NAI::CTimeCounter timeCounter;

extern CGroupLogic theGroupLogic;
extern CStatistics theStatistics;
extern CGraveyard theGraveyard;
extern CShellsStore theShellsStore;

//*******************************************************************
//*													CInfantry																*
//*******************************************************************

BASIC_REGISTER_CLASS( CSoldier );


CSoldier::~CSoldier()
{

}

void CSoldier::AdjustSpeed()
{
	SetSpeed( GetMaxSpeedHere() );
}

void CSoldier::AllowLieDown( bool _bAllowLieDown )
{
	bAllowLieDown = _bAllowLieDown;
	
	if ( !bAllowLieDown )
		StandUp();
}

void CSoldier::InitGuns()
{
	SetAngles( 0, 65535 );
	pGuns = new CInfantryGuns;
	pGuns->Init( this );

	SetShootEstimator( new CSoldierShootEstimator( this ) );
}

const DWORD CSoldier::GetNormale( const CVec2 &vCenter ) const
{
	return IsLying() ? GetHeights()->GetNormal( vCenter ) : GetHeights()->GetNormal( -1, -1 );
}

const DWORD CSoldier::GetNormale() const
{
	return IsLying() ? GetHeights()->GetNormal( GetCenterPlain() ) : GetHeights()->GetNormal( -1, -1 );
}

IStatesFactory* CSoldier::GetStatesFactory() const
{
	if ( IsInBuilding() ) 
		return CInBuildingStatesFactory::Instance();
	/*else if ( IsInEntrenchment() )
	return CInEntrenchmentStatesFactory::Instance();*/
	else if ( IsInTransport() )
		return CInTransportStatesFactory::Instance();
	else
		return CSoldierStatesFactory::Instance();
}

CBuilding* CSoldier::GetBuilding() const
{
	NI_ASSERT( IsInBuilding(), "Soldier isn't in a building" );
	return checked_cast<CBuilding*>( pObjInside );
}

CEntrenchment* CSoldier::GetEntrenchment() const
{
	//NI_ASSERT( IsInEntrenchment(), "Soldier isn't in entrenchment" );
	return checked_cast<CEntrenchment*>( pObjInside );
}

CMilitaryCar* CSoldier::GetTransportUnit() const
{
	NI_ASSERT( IsInTransport(), "Soldier isn't in a transport" );
	return checked_cast<CMilitaryCar*>( pObjInside );
}

void CSoldier::WarFogChanged()
{
	IUnitState *pState = GetFormation()->GetState();

	if ( ( !pState || pState->GetName() != EUSN_GUN_CREW_STATE ) && GetParty() < 2 )
	{
		SWarFogUnitInfo fogInfo;
		GetFogInfo( &fogInfo );
		theWarFog.UpdateUnit( GetUniqueId(), fogInfo );
	}
}

void CSoldier::SetInBuilding( CBuilding *pBuilding )
{
	pObjInside = pBuilding;
	SetMasterOfStreetsBonus( EOIO_BUILDING );
	eInsideType = EOIO_BUILDING;
	//SetVisionAngle( 0 );

	SAIEntranceUpdate *pUpdate = new SAIEntranceUpdate;
	pUpdate->info.bEnter = true;
	pUpdate->info.nInfantryUniqueID = GetUniqueId();
	pUpdate->info.nTargetUniqueID = pBuilding->GetUniqueId();
	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, this, -1 );
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_ENTRANCE_STATE, this, -1 );
	//WarFogChanged();

	slotInfo.nSlot = -1;

	pBuilding->AddInsider( this );
	SetSelectable( false, true );
}

void CSoldier::SetInEntrenchment( CEntrenchment *pEntrenchment )
{
	Stop();	

	pObjInside = pEntrenchment;
	SetMasterOfStreetsBonus( EOIO_ENTRENCHMENT );
	eInsideType = EOIO_ENTRENCHMENT;

	// Add bonus
	ApplyStatsModifier( pEntrenchment->GetUnitBonus(), true );

	//	SetVisionAngle( 0 );
	SAIEntranceUpdate *pUpdate = new SAIEntranceUpdate;
	pUpdate->info.bEnter = true;
	pUpdate->info.nInfantryUniqueID = GetUniqueId();
	pUpdate->info.nTargetUniqueID = pEntrenchment->GetUniqueId();
	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, this, -1 );
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_ENTRANCE_STATE, this, -1 );

	pEntrenchment->AddInsider( this );
}

void CSoldier::SetInTransport( class CMilitaryCar *pUnit )
{
	pObjInside = pUnit;
	SetMasterOfStreetsBonus( EOIO_TRANSPORT );
	eInsideType = EOIO_TRANSPORT;
	// на броне
	if ( pUnit->GetStats()->IsArmor() || pUnit->GetStats()->IsSPG() )
		SetToFirePlace();
	else
	{
		SetVisionAngle( 0 );
		SetToSolidPlace();
	}

	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, this, -1 );
	SAIEntranceUpdate *pUpdate = new SAIEntranceUpdate;
	pUpdate->info.bEnter = true;
	pUpdate->info.nInfantryUniqueID = GetUniqueId();
	pUpdate->info.nTargetUniqueID = pUnit->GetUniqueId();
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_ENTRANCE_STATE, this, -1 );
	WarFogChanged();
	SetSelectable( false, true );
}

void CSoldier::SetFree()
{
	SetVisionAngle( SConsts::STANDART_VIS_ANGLE );
	//SetSightMultiplier( 1 );
	SetAngles( 0, 65535 );

	if ( bInFirePlace )
		bInFirePlace = false;
	else if ( bInSolidPlace )
	{
		units.AddUnitToCell( this, true );
		bInSolidPlace = false;
	}

	if ( eInsideType == EOIO_TRANSPORT && GetPlayer() == theDipl.GetMyNumber() )
		SetSelectable( true, true );

	if ( eInsideType == EOIO_BUILDING || eInsideType ==  EOIO_ENTRENCHMENT || eInsideType == EOIO_TRANSPORT)
	{
		SAIEntranceUpdate *pUpdate = new SAIEntranceUpdate;
		pUpdate->info.bEnter = false;
		pUpdate->info.nInfantryUniqueID = GetUniqueId();
		switch (eInsideType) 
		{
		case EOIO_BUILDING:
			pUpdate->info.nTargetUniqueID = GetBuilding()->GetUniqueId();
			break;
		case EOIO_ENTRENCHMENT:
			{
				pUpdate->info.nTargetUniqueID = GetEntrenchment()->GetUniqueId();
				// Remove bonus
				CEntrenchment *pEntrenchment = dynamic_cast<CEntrenchment*>( pObjInside );
				if ( pEntrenchment )
					ApplyStatsModifier( pEntrenchment->GetUnitBonus(), false );
			}
			break;
		case EOIO_TRANSPORT:
			pUpdate->info.nTargetUniqueID = GetTransportUnit()->GetUniqueId();
			break;
		default:
			pUpdate->info.nTargetUniqueID = -1;
		}
		EObjectInsideOf eTmp = eInsideType;
		eInsideType = EOIO_NONE;
		CalcVisibility( true );
		UpdateVisibilityForced();
		eInsideType = eTmp;
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_ENTRANCE_STATE, this, -1 );
		eInsideType = EOIO_NONE;
		UpdateVisibilityForced();
		SetSelectable( GetPlayer() == theDipl.GetMyNumber(), true );
	}

	SetMasterOfStreetsBonus( EOIO_NONE );

	eInsideType = EOIO_NONE;
	slotInfo.nSlot = -1;
	WarFogChanged();

	pFormation->SetGeometryPropertiesToSoldier( this, false );
}

void CSoldier::SetMasterOfStreetsBonus( EObjectInsideOf eNewInsideType )
{
	if ( const NDb::SUnitSpecialAblityDesc *pSA = GetUnitAbilityDesc( NDb::ABILITY_MASTER_OF_STREETS ) )
	{
		if ( !pSA->pStatsBonus )
			return;

		if ( eInsideType == EOIO_NONE && eNewInsideType != EOIO_NONE )
			ApplyStatsModifier( pSA->pStatsBonus, true );
		else if ( eInsideType != EOIO_NONE && eNewInsideType == EOIO_NONE )
			ApplyStatsModifier( pSA->pStatsBonus, false );
	}
}

void CSoldier::SetNSlot( const int nSlot )
{
	slotInfo.nSlot = nSlot;
	//DebugTrace( "Set slot %d for unit %d", nSlot, GetUniqueID() );
}

void CSoldier::MoveToEntrenchFireplace( const CVec3 &coord, const int _nSlot )
{
	NI_ASSERT( IsInEntrenchment(), "Wrong unit state" );

	SetNSlot( _nSlot );

	// CRAP{ чтобы не падала
	if ( IsInEntrenchment() )
	{
		// CRAP}
		CEntrenchment *pEntrenchment = GetEntrenchment();

		SRect rect;
		pEntrenchment->GetBoundRect( &rect );
		SetDirectionVec( -rect.dirPerp );

		SetToFirePlace();
	}

	SetCenter( coord, false );
	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, this, -1 );
	CalcVisibility( true );
	if ( IsInEntrenchment() && IsVisibleByPlayer() )
	{
		CEntrenchment *pEntrenchment = GetEntrenchment();
		SAIEntranceUpdate *pUpdate = new SAIEntranceUpdate;
		pUpdate->info.bEnter = true;
		pUpdate->info.nInfantryUniqueID = GetUniqueId();
		pUpdate->info.nTargetUniqueID = pEntrenchment->GetUniqueId();
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_IDLE_TRENCH, this, -1 );
	}

	SetVisionAngle( SConsts::STANDART_VIS_ANGLE );
	WarFogChanged();
}

void CSoldier::GetShotInfo( SAINotifyInfantryShot *pShotInfo ) const
{
	NI_ASSERT( IsInTransport() == false, "Can't shoot from inside of transport unit" );

	pShotInfo->typeID = GetShootAction();

	if ( IsInBuilding() )
	{
		pShotInfo->nObjUniqueID = GetBuilding()->GetUniqueId();
		pShotInfo->nSlot = slotInfo.nSlot;
	}
	else
	{
		pShotInfo->nObjUniqueID = GetUniqueId();
		pShotInfo->nSlot = -1;
	}
}

void CSoldier::GetThrowInfo( struct SAINotifyInfantryShot *pThrowInfo ) const
{
	NI_ASSERT( IsInTransport() == false, "Can't throw a grenade from inside of transport unit" );

	pThrowInfo->typeID = GetThrowAction();	

	if ( IsInBuilding() )
	{
		pThrowInfo->nObjUniqueID = GetBuilding()->GetUniqueId();
		pThrowInfo->nSlot = slotInfo.nSlot;
	}
	else
	{
		pThrowInfo->nObjUniqueID = GetUniqueId();
		pThrowInfo->nSlot = -1;
	}
}

const EActionNotify CSoldier::GetAimAction() const
{
	NI_ASSERT( IsInTransport() == false, "Can't aim from inside of transport unit" );

	if ( IsInBuilding() )
		return ACTION_NOTIFY_NONE;
	else if ( IsInEntrenchment() || IsInTankPit() && !IsVirtualTankPit() )
		return ACTION_NOTIFY_AIM_TRENCH;
	else if ( bLying || IsVirtualTankPit() )
		return ACTION_NOTIFY_AIM_LYING;
	else
		return ACTION_NOTIFY_AIM;
}

const EActionNotify CSoldier::GetShootAction() const
{
	NI_ASSERT( IsInTransport() == false, "Can't shoot from inside of transport unit" );

	if ( IsInBuilding() )
		return ACTION_NOTIFY_SHOOT_BUILDING;
	else if ( IsInEntrenchment() || IsInTankPit() && !IsVirtualTankPit() )
		return ACTION_NOTIFY_SHOOT_TRENCH;
	else if ( IsLying() || IsVirtualTankPit() )
		return ACTION_NOTIFY_SHOOT_LYING;
	else
		return ACTION_NOTIFY_INFANTRY_SHOOT;
}

const EActionNotify CSoldier::GetThrowAction() const
{
	NI_ASSERT( IsInTransport() == false, "Can't throw a grenade from inside of transport unit" );

	if ( IsInBuilding() )
		return ACTION_NOTIFY_THROW_BUILDING;
	else if ( IsInEntrenchment() || IsInTankPit() && !IsVirtualTankPit() )
		return ACTION_NOTIFY_THROW_TRENCH;
	else if ( IsLying() || IsVirtualTankPit() )
		return ACTION_NOTIFY_THROW_LYING;
	else
		return ACTION_NOTIFY_THROW;
}

bool CSoldier::InVisCone( const CVec2 &point ) const
{
	return 
		GetVisionAngle() >= 32768 || 
		DirsDifference( GetDirectionByVector( point - GetCenterPlain() ),GetFrontDirection() ) < GetVisionAngle();
}

void CSoldier::Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *_pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector )
{
	bBeingHealed = false;
	eInsideType = EOIO_NONE;
	pObjInside = 0;
	bInFirePlace = false;
	bInSolidPlace = false;
	fOwnSightRadius = -1.0f;

	pStats = checked_cast<const SInfantryRPGStats*>( _pStats );
	SetRememberedStats( pStats );
	lastHit = lastCheck = lastMineCheck = curTime;
	bLying = false;
	bAllowLieDown = true;
	lastDirUpdate = 0;
	bWait2Form = false;
	CAIUnit::Init( center, z, fHP, dir, player, pCollisionsCollector );
	nextSegmTime = 0;
	timeBWSegments = 0;
	nextPathSegmTime = 0;
	nextLogicSegmTime = 0;
	wMinAngle = 0;
	wMaxAngle = 65535;
	numBlastingCharges.first = 0;
	numBlastingCharges.second = 0;
	numControlledCharges.first = 0;
	numControlledCharges.second = 0;
	numLandMines.first = 0;
	numLandMines.second = 0;
	for ( vector< CDBPtr< NDb::SUnitSpecialAblityDesc > >::const_iterator it = pStats->GetActions()->specialAbilities.begin(); it != pStats->GetActions()->specialAbilities.end(); ++it )
	{
		const NDb::SUnitSpecialAblityDesc *pSA = *it;
		int nCharges; 
		if ( pSA )
			nCharges = pSA->fParameter;
		else
			nCharges = 3;
		switch ( (*it)->eName )
		{
			case NDb::ABILITY_PLACE_CHARGE:
				numBlastingCharges.first = nCharges;
				numBlastingCharges.second = nCharges;
				break;
			case NDb::ABILITY_PLACE_CONTROLLED_CHARGE:
				numControlledCharges.first = nCharges;
				numControlledCharges.second = nCharges;
				break;
			case NDb::ABILITY_LAND_MINE:
				numLandMines.first = nCharges;
				numLandMines.second = nCharges;
				break;
			default:
				break;
		}
	}
}

void CSoldier::DetonateCharges()
{
	for ( list< CPtr<CMineStaticObject> >::iterator it = detonatableCharges.begin(); it != detonatableCharges.end(); ++it )
	{
		CPtr<CMineStaticObject> pMine = *it;
		if ( IsValidObj( pMine ) )
			pMine->Die( 1.0f );
	}
	detonatableCharges.clear();
	SExecutorEventParam par( EID_ABILITY_DISABLE, 0, GetUniqueId() );
	CExecutorEventSpecialAbility event( par, NDb::ABILITY_DETONATE );
	pTheExecutorsContainer->RaiseEvent( event );
}

void CSoldier::UsedCharge( NDb::EUnitSpecialAbility eType, CMineStaticObject *pMine )
{
	pair<int,int> *pNumbers = 0;
	switch ( eType )
	{
	case NDb::ABILITY_PLACE_CHARGE:
		pNumbers = &numBlastingCharges;
		break;
	case NDb::ABILITY_PLACE_CONTROLLED_CHARGE:
		{
			pNumbers = &numControlledCharges;
			detonatableCharges.push_back( pMine );
			SExecutorEventParam par( EID_ABILITY_ENABLE, 0, GetUniqueId() );
			CExecutorEventSpecialAbility event( par, NDb::ABILITY_DETONATE );
			pTheExecutorsContainer->RaiseEvent( event );
		}
		break;
	case NDb::ABILITY_LAND_MINE:
		pNumbers = &numLandMines;
		break;
	default:
		NI_ASSERT( false, "Wrong ability passed" );
		break;
	}
	NI_ASSERT( pNumbers->first > 0, "No more charges left!" );
	--(pNumbers->first);
	if ( pNumbers->first == 0 )
	{
		SExecutorEventParam par( EID_ABILITY_DISABLE, 0, GetUniqueId() );
		CExecutorEventSpecialAbility event( par, eType );
		pTheExecutorsContainer->RaiseEvent( event );		
	}
}

bool CSoldier::HasChargesToDetonate() const
{
	return !detonatableCharges.empty();
}

int CSoldier::GetChargesLeft( NDb::EUnitSpecialAbility eType ) const
{
	switch ( eType )
	{
		case NDb::ABILITY_PLACE_CHARGE:
			return numBlastingCharges.first;
			break;
		case NDb::ABILITY_PLACE_CONTROLLED_CHARGE:
			return numControlledCharges.first;
			break;
		case NDb::ABILITY_LAND_MINE:
			return numLandMines.first;
			break;
		default:
			NI_ASSERT( false, "Wrong ability passed" );
			return 0;
			break;
	}
}

const EActionNotify CSoldier::GetDieAction() const
{
	if ( IsInBuilding() )
		return ACTION_NOTIFY_DIE_BUILDING;
	else if ( IsInEntrenchment() || IsInTankPit() && !IsVirtualTankPit() )
		return ACTION_NOTIFY_DIE_TRENCH;
	else if ( IsInTransport() )
		return ACTION_NOTIFY_DIE_TRANSPORT;
	else if ( bLying || IsVirtualTankPit() )
		return ACTION_NOTIFY_DIE_LYING;
	else
		return ACTION_NOTIFY_DIE;
}

const EActionNotify CSoldier::GetIdleAction() const
{
	if ( IsInEntrenchment()  )
		return ACTION_NOTIFY_IDLE_TRENCH;
	else// if ( IsFree() )
	{
		if ( bLying || IsInTankPit() || IsVirtualTankPit() )
			return ACTION_NOTIFY_IDLE_LYING;
		else
			return ACTION_NOTIFY_IDLE;
	}
//	else
//		return ACTION_NOTIFY_IDLE;
}

const EActionNotify CSoldier::GetMovingAction() const
{
	if ( EUSN_PARTROOP == GetState()->GetName() )
	{
		return ACTION_NOTIFY_FALLING;
	}
	else if ( bLying )
		return ACTION_NOTIFY_CRAWL;
	else
		return ACTION_NOTIFY_MOVE;
}

bool CSoldier::IsInSolidPlace() const
{
	return bInSolidPlace;
}

bool CSoldier::IsInFirePlace() const
{
	return bInFirePlace;
}

void CSoldier::SetToFirePlace()
{
	bLying = false;
	if ( !IsInFirePlace() )
	{
		if ( IsInSolidPlace() )
		{
			units.AddUnitToCell( this, true );
			bInSolidPlace = false;
		}

		bInFirePlace = true;
	}

	if ( eInsideType == EOIO_NONE )
		eInsideType = EOIO_UNKNOWN;

	WarFogChanged();
}

void CSoldier::SetToSolidPlace()
{
	bLying = false;
	if ( !IsInSolidPlace() )
	{
		units.DelUnitFromCell( this, true );

		bInFirePlace = false;
		bInSolidPlace = true;
		CalcVisibility( true );
	}

	if ( eInsideType == EOIO_NONE )
		eInsideType = EOIO_UNKNOWN;
	
	WarFogChanged();
}

void CSoldier::GetEntranceStateInfo( struct SAINotifyEntranceState *pInfo ) const
{
	// CRAP{ something was wrong with 'pObjInside' - need additional fixing
	if ( pObjInside )
	{
		pInfo->nTargetUniqueID = checked_cast<CUpdatableObj*>(pObjInside)->GetUniqueId();
		pInfo->bEnter = !IsFree();
		pInfo->nInfantryUniqueID = GetUniqueId();
	}
	else
	{
		pInfo->nTargetUniqueID = 0;
		pInfo->nInfantryUniqueID = 0;
	}
	// CRAP}
}

const float CSoldier::GetCover() const
{
	return GetStatsModifier()->cover.Get( 1.0f );
}

void CSoldier::UpdateLyingPosition()
{
	if ( bLying == true )
		updater.AddUpdate( 0, ACTION_NOTIFY_STAYING_TO_LYING, this, -1 );
	else
		updater.AddUpdate( 0, ACTION_NOTIFY_LYING_TO_STAYING, this, -1 );
	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, this, -1 );
}

void CSoldier::LieDownForce()
{
	bLying = true;
	UpdateLyingPosition();
}

void CSoldier::LieDown()
{
	if ( !bLying && bAllowLieDown && IsFree() && !bLying && ( !IsValidObj( pFormation ) || pFormation->IsAllowedLieDown() ) )
	{
		LieDownForce();
	}
}

void CSoldier::StandUp()
{
	if ( bLying && bAllowLieDown && ( !IsValidObj( pFormation ) || pFormation->IsAllowedStandUp() ) )
	{
		bLying = false;
		UpdateLyingPosition();
	}
}

void CSoldier::Segment()
{
	if ( curTime >= nextLogicSegmTime )
		CAIUnit::Segment();

	pGuns->Segment();

	if ( IsInBuilding() && GetSlot() != -1 )
	{
		CBuilding *pBuilding = checked_cast<CBuilding*>(pObjInside);
		for ( int i = 0; i < pBuilding->GetNGunsInFireSlot( GetSlot() ); ++i )
			pBuilding->GetGunInFireSlot( GetSlot(), i )->Segment();
	}

	//для инженеров ( было, теперь все сканируют )
	if ( curTime - lastMineCheck > SConsts::ENGINEER_MINE_CHECK_PERIOD )
	{
		lastMineCheck = curTime;
		if ( !IsRestInside() )
			RevealNearestMines( pStats->etype == RPG_TYPE_ENGINEER /* && EUSN_CLEAR_MINE == GetFormation()->GetState()->GetName() */ );
	}

	if ( curTime >= nextLogicSegmTime )
	{
		if ( timeBWSegments < 700 )
			timeBWSegments += 20;

		nextLogicSegmTime = curTime + NRandom::Random( 0, timeBWSegments );
	}
	nextSegmTime = curTime + NRandom::Random( 0, 250 );
}

void CSoldier::FreezeSegment()
{
	if ( curTime - lastCheck >= SConsts::TIME_OF_HIT_NOTIFY + NRandom::Random( 0.0f, SConsts::STAND_LIE_RANDOM_DELAY ) )
	{
		lastCheck = curTime;
		if ( theHitsStore.WasHit( GetCenterPlain(), SConsts::RADIUS_OF_HIT_NOTIFY, CHitsStore::EHT_ANY ) )
		{
			// if ( IsInFormation() )
				// GetFormation()->WasHitNearUnit();

			lastHit = curTime;				
			if ( bAllowLieDown && IsInFormation() && pFormation->IsAllowedLieDown() )
				LieDown();
		}
		if ( NeedReturnToFormation() )  
		{
			FreezeByState( false );
			theGroupLogic.RegisterSegments( this, false, false );
		}
	}

	// обстрел закончился
	if ( bLying && ( !IsInFormation() || GetFormation()->IsAllowedStandUp() ) && curTime - lastHit >= SConsts::TIME_OF_LYING_UNDER_FIRE + NRandom::Random( 0.0f, SConsts::STAND_LIE_RANDOM_DELAY ) )
		StandUp();

	CAIUnit::FreezeSegment();
}

const bool CSoldier::NeedReturnToFormation() const
{
	return ( IsInFormation() && IsIdle() && GetState()->IsRestState() &&
		fabs2( GetCenterPlain() - GetUnitPointInFormation() ) > sqr( 1.0f * SAIConsts::TILE_SIZE ) );
}

void CSoldier::RevealNearestMines( const bool bIncludingAP )
{
	const int nParty = GetParty();
	bool bFoundMine = false;
 	for ( CMinesIter iter( GetCenterPlain(), SConsts::MINE_VIS_RADIUS, nParty ); !iter.IsFinished(); iter.Iterate() )
	{
		CMineStaticObject *pMine = *iter;
		const EMineType eType = checked_cast<const SMineRPGStats*>( pMine->GetStats() )->etype ;
		if ( bIncludingAP || eType == MT_TECHNICS /* || eType == MT_INFANTRY_AND_TECHNICS */ )
		{
			pMine->SetVisible( nParty, true );
			bFoundMine = true;
		}
	}
	if ( bFoundMine )
		SendAcknowledgement( NDb::ACK_MINE_FOUND, nParty == theDipl.GetMyParty() );
}

void CSoldier::SetFormation( CFormation *_pFormation )
{
	pFormation = _pFormation;
}

void CSoldier::Die( const bool fromExplosion, const float fDamage )
{
	// flamethrowers die with explosion
	if ( GetNGuns() > 0 && GetGun( 0 ) && GetGun( 0 )->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_FLAME_THROWER )
	{
		const NDb::SAIGameConsts * pConsts = Singleton<IAILogic>()->GetAIConsts(); 
		if ( pConsts->pFlamethrowerDeathExplotion )
		{
			theShellsStore.AddShell
				( new CInvisShell( curTime + 100, new CBurstExpl( 0, pConsts->pFlamethrowerDeathExplotion, 
				GetCenter(), VNULL3, 0, false, 1, true ), 0 ) );
		}
	}
	// Reassign detonatable charges, if any
	bool bTransferred = false;
	if ( HasChargesToDetonate() && pFormation ) 
	{
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			CSoldier *pFormationMember = (*pFormation)[ i ];
			if ( !pFormationMember || pFormationMember == this ) 
				continue;

			const NDb::SUnitBaseRPGStats *pMemberStats = pFormationMember->GetStats();
			NI_ASSERT( pMemberStats, "No unit stats for formation member" );
			for ( int j = 0; j < pMemberStats->GetActions()->specialAbilities.size(); ++j )
			{
				if ( pMemberStats->GetActions()->specialAbilities[ j ]->eName == NDb::ABILITY_DETONATE )
				{
					for ( list< CPtr<CMineStaticObject> >::iterator it = detonatableCharges.begin(); it != detonatableCharges.end(); ++it )
					{
						CMineStaticObject *pCharge = *it;
						pFormationMember->detonatableCharges.push_back( pCharge );
					}

					SExecutorEventParam par( EID_ABILITY_ENABLE, 0, pFormationMember->GetUniqueId() );
					CExecutorEventSpecialAbility event( par, NDb::ABILITY_DETONATE );
					pTheExecutorsContainer->RaiseEvent( event );
					bTransferred = true;
					break;
				}
			}

			if ( bTransferred )
				break;
		}
	}

	if ( !bTransferred )
	{
		// Charges left unassigned... do something?
	}

	CAIUnit::Die( fromExplosion, fDamage );
}

void CSoldier::GetFogInfo( SWarFogUnitInfo *pInfo ) const
{
	if ( !IsInSolidPlace() )
	{
		pInfo->bPlane = false;

		pInfo->vPos.x = GetCenterTile().x / AI_TILES_IN_VIS_TILE;
		pInfo->vPos.y = GetCenterTile().y / AI_TILES_IN_VIS_TILE;
		pInfo->nRadius = GetSightRadius() / SConsts::TILE_SIZE / AI_TILES_IN_VIS_TILE;

		if ( GetVisionAngle() >=32768 )
		{
			if ( !IsAngleLimited() )
				pInfo->sector = SSector();
			else
				pInfo->sector = SSector( GetMinAngle(), GetMaxAngle() );
		}
		else
			pInfo->sector = SSector( GetFrontDirection() - GetVisionAngle(), GetFrontDirection() + GetVisionAngle() );
	}
	else
		CAIUnit::GetFogInfo( pInfo );
}

CArtillery* CSoldier::GetArtilleryIfCrew() const
{
	if ( CFormation *pFormation = GetFormation() )
	{
		if ( IUnitState *pState = pFormation->GetState() )
		{
			if ( pState->GetName() == EUSN_GUN_CREW_STATE )
			{
				CFormationGunCrewState *pFullState = checked_cast<CFormationGunCrewState*>(pState);
				if ( CArtillery *pArtillery = pFullState->GetArtillery() )
				{
					if ( pArtillery && pArtillery->IsRefValid() && pArtillery->IsAlive() )
						return pArtillery;
				}
			}
		}
	}

	return 0;
}

int CSoldier::GetNGuns() const
{
	if ( CArtillery *pArtillery = GetArtilleryIfCrew() )
		return pGuns->GetNTotalGuns() + pArtillery->GetNGuns();
	else if ( !IsInBuilding() || GetSlot() == -1 )
		return pGuns->GetNTotalGuns();
	else
		return pGuns->GetNTotalGuns() + checked_cast<CBuilding*>(pObjInside)->GetNGunsInFireSlot( GetSlot() );
}

CBasicGun* CSoldier::GetGun( const int n ) const
{
	if ( n < pGuns->GetNTotalGuns() )
		return pGuns->GetGun( n );
	else if ( n >= GetNGuns() )
		return GetGuns()->GetGun( 0 );
	else if ( CArtillery *pArtillery = GetArtilleryIfCrew() )
		return pArtillery->GetGun( n - pGuns->GetNTotalGuns() );
	else
		return checked_cast<CBuilding*>(pObjInside)->GetGunInFireSlot( GetSlot(), n - pGuns->GetNTotalGuns() );
}

int CSoldier::GetNAmmo( const int nCommonGun ) const
{
	if ( nCommonGun < pGuns->GetNCommonGuns() )
		return pGuns->GetNAmmo( nCommonGun );
	else
		return 1000;
}

void CSoldier::ChangeAmmo( const int nCommonGun, const int nAmmo )
{
	if ( nCommonGun < pGuns->GetNCommonGuns() )
		CAIUnit::ChangeAmmo( nCommonGun, nAmmo );
	if ( nCommonGun == 1 && !HasGrenades() )
	{
		if ( HasGrenades() )
		{
			SExecutorEventParam par( EID_ABILITY_ENABLE, 0, GetUniqueId() );
			CExecutorEventSpecialAbility event( par, NDb::ABILITY_THROW_GRENADE );
			pTheExecutorsContainer->RaiseEvent( event );
		}
		else
		{
			SExecutorEventParam par( EID_ABILITY_DISABLE, 0, GetUniqueId() );
			CExecutorEventSpecialAbility event( par, NDb::ABILITY_THROW_GRENADE );
			pTheExecutorsContainer->RaiseEvent( event );
		}	
	}
	numControlledCharges.first = numControlledCharges.second;
	numBlastingCharges.first = numBlastingCharges.second;
	numLandMines.first = numLandMines.second;
	if ( numControlledCharges.first > 0 )
	{
		SExecutorEventParam par( EID_ABILITY_ENABLE, 0, GetUniqueId() );
		CExecutorEventSpecialAbility event( par, NDb::ABILITY_PLACE_CONTROLLED_CHARGE );
		pTheExecutorsContainer->RaiseEvent( event );
	}
	if ( numBlastingCharges.first > 0 )
	{
		SExecutorEventParam par( EID_ABILITY_ENABLE, 0, GetUniqueId() );
		CExecutorEventSpecialAbility event( par, NDb::ABILITY_PLACE_CHARGE );
		pTheExecutorsContainer->RaiseEvent( event );
	}
	if ( numLandMines.first > 0 )
	{
		SExecutorEventParam par( EID_ABILITY_ENABLE, 0, GetUniqueId() );
		CExecutorEventSpecialAbility event( par, NDb::ABILITY_LAND_MINE );
		pTheExecutorsContainer->RaiseEvent( event );
	}
}

bool CSoldier::IsCommonGunFiring( const int nCommonGun ) const
{
	if ( nCommonGun < pGuns->GetNCommonGuns() )
		return CAIUnit::IsCommonGunFiring( nCommonGun );

	return false;
}

void CSoldier::Fired( const float fGunRadius, const int nGun )
{
	if ( nGun < pGuns->GetNCommonGuns() )
		CAIUnit::Fired( fGunRadius, nGun );
}

CTurret* CSoldier::GetTurret( const int nTurret ) const 
{
	NI_ASSERT( nTurret == 0, "Wrong number of turret for soldier" );
	if ( !IsInBuilding() || GetSlot() == -1 )
		return 0;
	else
		return checked_cast<CBuilding*>(pObjInside)->GetTurretInFireSlot( GetSlot() );
}

const int CSoldier::GetNTurrets() const
{
	if ( GetTurret( 0 ) )
		return 1;
	else
		return 0;
}

void CSoldier::GetShootAreas( SShootAreas *pShootAreas, int *pnAreas ) const
{
	//construct( pShootAreas );			// this causes memory leaks
	
	if ( IsFree() || IsInBuilding() && GetTurret( 0 ) != 0 || IsInEntrenchment() && IsInFirePlace() )
		CAIUnit::GetShootAreas( pShootAreas, pnAreas );
	else
	{
		*pnAreas = 1;
		pShootAreas->areas.push_back( SShootArea() );
		SShootArea &area = pShootAreas->areas.back();
		area.eType = SShootArea::ESAT_LINE;
		area.vCenter3D = CVec3( GetCenterPlain(), 0.0f );

		area.fMinR = 0.0f;
		area.fMaxR = GetMaxFireRange();
		
		if ( IsInSolidPlace() || IsInTransport() )
		{
			area.wStartAngle = 65535;
			area.wFinishAngle = 65535;
		}
		else
		{
			area.wStartAngle = wMinAngle;
			area.wFinishAngle = wMaxAngle;
		}
	}
}

float CSoldier::GetMaxFireRange() const
{
	return pGuns->GetMaxFireRange( this );
}

void CSoldier::PrepareToDelete()
{
	if ( IsAlive() )
	{
		// удалить из статич. объекта, если нужно
		if ( IsInBuilding() )
			GetBuilding()->DelInsider( this );
		else if ( IsInEntrenchment() )
			GetEntrenchment()->DelInsider( this );
		else if ( IsInTransport() )
			GetTransportUnit()->DelPassenger( this );

		// удалить из формации, если нужно
		if ( IsInFormation() )
		{
			pFormation->DeleteSoldier( this );
			if ( GetFormation()->Size() == 0 && GetFormation()->VirtualUnitsSize() == 0 )
				pFormation->Disappear();

			if ( pMemorizedFormation )
			{
				pMemorizedFormation->DeleteSoldier( this );
				if ( pMemorizedFormation->Size() == 0 && pMemorizedFormation->VirtualUnitsSize() == 0 )
					pMemorizedFormation->Disappear();
			}
			
			if ( pVirtualFormation )
			{
				pVirtualFormation->DelVirtualUnit( this );
				pVirtualFormation = 0;
			}
		}

		CAIUnit::PrepareToDelete();
	}
}

const CVec2 CSoldier::GetUnitPointInFormation() const 
{ 
	return pFormation->GetUnitCoord( this ); 
}

void CSoldier::TakeDamage( const float fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	if ( EUSN_PARTROOP != GetState()->GetName() )
	{
		CAIUnit::TakeDamage( fDamage, pShell, nPlayerOfShoot, pShotUnit );
		if ( GetHitPoints() <= 0.0f )
		{
			SetCenter( GetHeights()->Get3DPoint( GetCenterPlain() ), true );
		}
		if ( IsAlive() && IsInBuilding() )
			GetBuilding()->InsiderDamaged( this );
	}
}

bool CSoldier::IsFree() const 
{ 
	return eInsideType == EOIO_NONE || eInsideType ==	EOIO_UNKNOWN; 
}

const bool CSoldier::CanShootToPlanes() const 
{ 
	return pGuns->CanShootToPlanes(); 
}

CBasicGun* CSoldier::ChooseGunForStatObj( class CStaticObject *pObj, NTimer::STime *pTime ) 
{ 
	return pGuns->ChooseGunForStatObj( this, pObj, pTime ); 
}

bool CSoldier::CalculateUnitVisibility4Party( const BYTE party )
{
	bool bVisibility;
	if ( IsInBuilding() )
	{
		bVisibility = 
			( GetBuilding()->IsAnyAttackers() || IsInFirePlace() ) && 
			 GetBuilding()->IsSoldierVisible( party, GetCenterPlain(), IsCamoulflated(), GetCamouflage() );
	}
	else if ( IsInSolidPlace() )
		bVisibility = false;
	else if ( party == GetParty() )
		bVisibility = true;
	else if ( IsInFormation() && GetFormation()->GetState() && GetFormation()->GetState()->GetName() == EUSN_GUN_CREW_STATE && IsValidObj( checked_cast<CFormationGunCrewState*>(GetFormation()->GetState())->GetArtillery()) )
	{
		return IsVisible( party );
		//bVisibility = checked_cast<CFormationGunCrewState*>(GetFormation()->GetState())->GetArtillery()->CalculateUnitVisibility4PartyInner( party );
	}
	else if ( IsInFirePlace() && IsInTransport() )
		bVisibility = GetTransportUnit()->IsVisible( party );
	else
		bVisibility = CalculateUnitVisibility4PartyInner( party );
	return UpdateUnitVisibilityForParty( party, bVisibility );
}

void CSoldier::SetDirection( const WORD newDir )
{
	lastDirUpdate = curTime;
	CAIUnit::SetDirection( newDir );
}

bool CSoldier::IsNoticableByUnit( class CCommonUnit *pUnit, const float fNoticeRadius )
{
	if ( IsInSolidPlace() )
		return false;
	else if ( IsInFirePlace() && IsInBuilding() && ( IsVisible( pUnit->GetParty() ) || IsRevealed() ) )
	{
		SRect buildRect;
		GetBuilding()->GetBoundRect( &buildRect );
		if ( buildRect.IsIntersectCircle( pUnit->GetCenterPlain(), fNoticeRadius ) )
			return true;
		else
			return false;
	}
	else
		return CAIUnit::IsNoticableByUnit( pUnit, fNoticeRadius );
}

bool CSoldier::ProcessAreaDamage( const CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius )
{
	const CVec3 vExpl( pExpl->GetExplCoordinates() );

	if ( !IsInBuilding() && !IsInSolidPlace() && fabs( GetVisZ() - vExpl.z ) <= GetStats()->vAABBHalfSize.y )
	{
		if ( !CAIUnit::ProcessAreaDamage( pExpl, nArmorDir, fRadius, fSmallRadius ) )
		{
			if ( IsFree() && !IsLying() && fabs2( GetCenter() - pExpl->GetExplCoordinates() ) <= sqr( fRadius ) )
			{
				TakeDamage( pExpl->GetRandomDamage() * SConsts::AREA_DAMAGE_COEFF, &(pExpl->GetShellStats()), pExpl->GetPlayerOfShoot(), pExpl->GetWhoFire() );
				return true;
			}
		}
	}

	return false;
}

void CSoldier::MemCurFormation()
{
	pMemorizedFormation = pFormation;
}

const int CSoldier::GetMinArmor() const
{
/*	
	if ( IsInFormation() && GetFormation()->GetState()->GetName() == EUSN_GUN_CREW_STATE )
		return Min( CAIUnit::GetMinArmor(), checked_cast<CFormationGunCrewState*>(GetFormation()->GetState())->GetArtillery()->GetMinArmor() );
	else
*/
		return CAIUnit::GetMinArmor();
}

const int CSoldier::GetMaxArmor() const
{
/*	
	if ( IsInFormation() && GetFormation()->GetState()->GetName() == EUSN_GUN_CREW_STATE )
		return Max( CAIUnit::GetMaxArmor(), checked_cast<CFormationGunCrewState*>(GetFormation()->GetState())->GetArtillery()->GetMaxArmor() );
	else
*/
		return CAIUnit::GetMaxArmor();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
const int CSoldier::GetMinPossibleArmor( const int nSide ) const
{
/*	
	if ( nSide == RPG_FRONT && IsInFormation() && GetFormation()->GetState()->GetName() == EUSN_GUN_CREW_STATE )
		return checked_cast<CFormationGunCrewState*>(GetFormation()->GetState())->GetArtillery()->GetMinPossibleArmor( nSide );
	else
*/
		return CAIUnit::GetMinPossibleArmor( nSide );
}

const int CSoldier::GetMaxPossibleArmor( const int nSide ) const
{
/*	
	if ( nSide == RPG_FRONT && IsInFormation() && GetFormation()->GetState()->GetName() == EUSN_GUN_CREW_STATE )
		return checked_cast<CFormationGunCrewState*>(GetFormation()->GetState())->GetArtillery()->GetMaxPossibleArmor( nSide );
	else
*/
		return CAIUnit::GetMaxPossibleArmor( nSide );
}

const int CSoldier::GetArmor( const int nSide ) const
{
/*	
	if ( nSide == RPG_FRONT && IsInFormation() && GetFormation()->GetState()->GetName() == EUSN_GUN_CREW_STATE )
		return checked_cast<CFormationGunCrewState*>(GetFormation()->GetState())->GetArtillery()->GetArmor( nSide );
	else
*/
		return CAIUnit::GetArmor( nSide );
}

const int CSoldier::GetRandomArmor( const int nSide ) const
{
/*	
	if ( nSide == RPG_FRONT && IsInFormation() && GetFormation()->GetState()->GetName() == EUSN_GUN_CREW_STATE )
		return checked_cast<CFormationGunCrewState*>(GetFormation()->GetState())->GetArtillery()->GetRandomArmor( nSide );
	else
*/
		return CAIUnit::GetRandomArmor( nSide );
}

bool CSoldier::IsAngleLimited() const 
{ 
	return
		IsInBuilding() || wMinAngle > 0 || wMaxAngle < 65535;
}

void CSoldier::GetTilesForVisibility( CTilesSet *pTiles ) const
{
	if ( IsFree() )
		CAIUnit::GetTilesForVisibility( pTiles );
	else if ( CDynamicCast<CUpdatableObj> pUnitInside = pObjInside )
		pUnitInside->GetTilesForVisibility( pTiles );
	else
		CAIUnit::GetTilesForVisibility( pTiles );
}

bool CSoldier::ShouldSuspendAction( const EActionNotify &eAction ) const
{
	return ( !IsInTransport() &&
		( eAction == ACTION_NOTIFY_GET_DEAD_UNITS_UPDATE ||
			eAction == ACTION_NOTIFY_CLOSE_PARASHUTE ||
			eAction == ACTION_NOTIFY_ENTRANCE_STATE ||
			eAction == ACTION_NOTIFY_FALLING ) );
}

bool IsTransitionalCommand( const EActionCommand &eCmd )
{
	return 		
		eCmd == ACTION_COMMAND_ENTER ||
		eCmd == ACTION_COMMAND_ENTER_TRANSPORT_NOW || 
		eCmd == ACTION_COMMAND_LOAD || eCmd == ACTION_COMMAND_LOAD_NOW ||
		eCmd == ACTION_COMMAND_LEAVE || eCmd == ACTION_COMMAND_LEAVE_NOW ||
		eCmd == ACTION_COMMAND_UNLOAD || eCmd == ACTION_COMMAND_UNLOAD_NOW;
}

bool CSoldier::CanJoinToFormation() const
{
	if ( !IsCurCmdFinished() )
	{
		bool bCan = true;
		
		if ( CAICommand *pCmd = GetCurCmd() )
			bCan = bCan && !IsTransitionalCommand( pCmd->ToUnitCmd().nCmdType );
		if ( CAICommand *pCmd = GetNextCommand() )
			bCan = bCan && !IsTransitionalCommand( pCmd->ToUnitCmd().nCmdType );

		return bCan;
	}
	else
		return false;
}

void CSoldier::SetVirtualFormation( CFormation *pFormation )
{
	pVirtualFormation = pFormation;
}

void CSoldier::MemorizeFormation() 
{ 
	pFormation = pMemorizedFormation; 
	pMemorizedFormation = 0; 

	pFormation->SetGeometryPropertiesToSoldier( this, true );
}

void CSoldier::LookForTarget( CAIUnit *pCurTarget, const bool bDamageUpdated, CAIUnit **pBestTarget, CBasicGun **pGun )
{
	CAIUnit::LookForTarget( pCurTarget, bDamageUpdated, pBestTarget, pGun );

	if ( CanShoot() )
	{
		if ( /* GetStats()->etype != RPG_TYPE_OFFICER && */ GetBehaviourMoving() != SBehaviour::EMHoldPos )
		{
			for ( int i = 0; i < GetFormation()->Size(); ++i )
			{
				IUnitState *pState = (*GetFormation())[i]->GetState();

				if ( pState && pState->IsRefValid() && pState->IsAttackingState() )
				{
					NI_ASSERT( dynamic_cast<IUnitAttackingState*>(pState) != 0, "Unexpected unit state" );
					CAIUnit *pTarget = static_cast<IUnitAttackingState*>(pState)->GetTargetUnit();

					if ( IsValidObj( pTarget ) && theDipl.GetDiplStatus( GetPlayer(), pTarget->GetPlayer() ) == EDI_ENEMY )
					{
						if ( pTarget->IsVisible( GetParty() ) || pTarget->IsRevealed() )
							AddUnitToShootEstimator( pTarget );
					}
				}
			}
		}

		*pBestTarget = GetBestShootEstimatedUnit();
		*pGun = GetBestShootEstimatedGun();
	}
}

void CSoldier::FirstSegment( const NTimer::STime timeDiff )
{
	CBasePathUnit::FirstSegment( timeDiff );
	nextPathSegmTime = curTime + NRandom::Random( 500, 1000 );
}

const float CSoldier::GetPathSegmentsPeriod() const
{
	return 1;
}

void CSoldier::FreezeByState( const bool bFreeze )
{
	if ( IsFrozenByState() != bFreeze )
	{
		if ( !bFreeze || pFormation->IsEveryUnitResting() )
			CAIUnit::FreezeByState( bFreeze );

		if ( !bFreeze )
			pFormation->FreezeByState( bFreeze );
	}
}

int CSoldier::HasGrenades() const
{
	for ( int i = 0; i < pGuns->GetNGuns(); ++i )
	{
		if ( pGuns->GetGun( i )->IsGrenade() && pGuns->GetGun( i )->GetNAmmo() > 0 )
			return pGuns->GetGun( i )->GetNAmmo();
	}
	return 0;
}

void CSoldier::SetGrenadeAutocast( bool bOn )
{
	CSoldierShootEstimator *pEstimator = checked_cast<CSoldierShootEstimator*>( GetShootEstimator() );
	pEstimator->SetGrenadeAutocast( bOn );
}

void CSoldier::SetGrenadeFixed( bool bOn )
{
	CSoldierShootEstimator *pEstimator = checked_cast<CSoldierShootEstimator*>( GetShootEstimator() );
	pEstimator->SetGrenadeFixed( bOn );
}

const bool CSoldier::CanUnitTrampled( const CBasePathUnit *pTramplerUnit ) const
{
	const CCommonUnit *pUnit = dynamic_cast<const CCommonUnit *>( pTramplerUnit );
	NI_ASSERT( pUnit != 0, "Cannot cast pTramplerUnit to CCommonUnit" );
	if ( pUnit == 0 )
		return false;

	const int nDipl = GetPlayer();
	return theDipl.GetNeutralPlayer() == nDipl || theDipl.GetDiplStatus( nDipl, pUnit->GetPlayer() ) == EDI_ENEMY;
}

const ECollidingType CSoldier::GetCollidingType( CBasePathUnit *pUnit ) const
{
	if ( GetFormation()->GetState() == 0 )
	{
		//DebugTrace( "GetFormation() = 0x%p (%d), GetFormation()->GetState() = 0x%p", GetFormation(), GetFormation()->GetUniqueID(), GetFormation()->GetState() );
		return ECT_NONE;
	}
	const EUnitStateNames eFormationStateName = GetFormation()->GetState()->GetName();

	return 
		IsFree() && eFormationStateName != EUSN_GUN_CREW_STATE && eFormationStateName != EUSN_PARTROOP
		? ECT_INTERNAL : ECT_NONE;
}

//*******************************************************************
//*														CSniper																*
//*******************************************************************

void CSniper::Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector )
{
	lastVisibilityCheck = 0;
	bVisible = false;
	bSneak = false;
	fCamouflageRemoveWhenShootProbability = 0.0f;

	CSoldier::Init( center, z, pStats, fHP, dir, player, pCollisionsCollector );
}

void CSniper::Segment()
{
	CSoldier::Segment();

	float fAdd = SConsts::SNIPER_CAMOUFLAGE_INCREASE * SConsts::AI_SEGMENT_DURATION / 1000;
	if ( fCamouflageRemoveWhenShootProbability >= fAdd )
		fCamouflageRemoveWhenShootProbability -= fAdd;
	else
		fCamouflageRemoveWhenShootProbability = 0.0f;

	// analyze camouflage and go to camouflage if not bein seed
	if ( curTime - lastVisibilityCheck >= 1000 + NRandom::Random( 0, 5 * SConsts::AI_SEGMENT_DURATION ) )
	{
		if ( IsInBuilding() && GetBuilding() )
			GetBuilding()->RemoveTransparencies();
		
		lastVisibilityCheck = curTime;
		bVisible = false;
		const float fMaxVisRadius = 30 * SConsts::TILE_SIZE;
		hash_set<SVector, STilesHash> visitedTiles;

		//SSniperTrace sniperTracer( this );
		const SVector curCenterTile( GetCenterTile() );
		
		for ( CUnitsIter<0,3> iter( GetParty(), EDI_ENEMY, GetCenterPlain(), fMaxVisRadius ); !bVisible && !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;
			if ( IsValidObj( pUnit ) && !pUnit->GetStats()->IsAviation() && theWarFog.IsUnitVisible( curCenterTile, pUnit->GetParty(), IsCamoulflated() ) )
				SetVisible();

		if ( IsInBuilding() && GetBuilding() )
			GetBuilding()->SetTransparencies();
		}
	}

	//if ( !bVisible && bSneak )
		//SetCamoulfage();
}

bool CSniper::CalculateUnitVisibility4Party( const BYTE party ) 
{
	if ( !IsCamoulflated() )
		return CSoldier::CalculateUnitVisibility4Party( party );
	else
	{
		if ( IsInSolidPlace() )
			return false;
		else if ( party == GetParty() )
			return true;
		else
			return bVisible;
	}
}

/*void CSniper::RemoveCamouflage( ECamouflageRemoveReason eReason )
{
	if ( bSneak )
	{
		switch ( eReason )
		{
		case ECRR_SELF_SHOOT:
			{
				SExecutorEventParam par( EID_FIRE_CAMOFLATED, 0, GetUniqueId() );
				theExecutorContainer.RaiseEvent( par );
			}
		// no break - ok.
		case ECRR_USER_COMMAND:
			break;
		case ECRR_SELF_MOVE:
		case ECRR_GOOD_VISIBILITY:
			return;
			break;
		}

		CAIUnit::RemoveCamouflage( eReason );
	}
	else
		CSoldier::RemoveCamouflage( eReason );
}*/

void CSniper::Fired( const float fGunRadius, const int nGun  )
{
	RemoveCamouflage( ECRR_SELF_SHOOT );
	CSoldier::Fired( fGunRadius, nGun );
}

/*void CSniper::SetSneak( const bool bSneakMode ) 
{ 
	if ( bSneak != bSneakMode )
	{
		bSneak = bSneakMode; 

		if ( bSneakMode )
		{
			LieDown();
			GetBehaviour().fire = SBehaviour::EFNoFire;
			if ( IsInFormation() )
				GetFormation()->GetBehaviour().fire = SBehaviour::EFNoFire;
			RegisterAsBored( ACK_BORED_SNIPER_SNEAK );
		}
		else
		{
			GetBehaviour().fire = SBehaviour::EFAtWill;
			GetFormation()->GetBehaviour().fire = SBehaviour::EFAtWill;
			GetFormation()->ChangeGeometry( 0 );

			UnRegisterAsBored( ACK_BORED_SNIPER_SNEAK );
		}
	}
}*/


