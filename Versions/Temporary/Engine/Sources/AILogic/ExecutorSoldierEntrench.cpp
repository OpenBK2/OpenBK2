#include "stdafx.h"
#include "ExecutorSoldierEntrench.h"
#include "GroupLogic.h"
#include "Soldier.h"

#include "AILogic_export.h"

extern CGroupLogic theGroupLogic;

CExecutorSoldierEntrench::CExecutorSoldierEntrench( CAIUnit *_pUnit )
: pUnit( _pUnit ), 
	CExecutorUnitBase( TID_SOLDIER_ENTRENCH_SELF, (1000 + NRandom::Random(1000))/SConsts::AI_SEGMENT_DURATION, 
	NDb::ABILITY_ENTRENCH_SELF )
{
	RecordRandomCall();
	for ( int i = 0; i < (std::min<int>) ( pUnit->GetStats()->GetActions()->specialAbilities.size(), pUnit->GetAbilityLevel() ); ++i )
	{
		const int nAbility = pUnit->GetStats()->GetActions()->specialAbilities[i]->eName;
		if ( nAbility == NDb::ABILITY_MOBILE_FORTRESS ) 
		{
			const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetStats()->GetActions()->specialAbilities[i];
			NI_ASSERT( pSA, "Ability desc (Mobile Fortress) not found");
			if ( pSA )
				SetSpeedCoeff( pSA->fParameter );
			break;
		}
	}
}

CExecutorSoldierEntrench::~CExecutorSoldierEntrench()
{
}

float CExecutorSoldierEntrench::OnAbilityActive()
{
	// check if unit is entrenched, if not - switch off ability
	if ( !pUnit->IsInTankPit() && !pUnit->IsVirtualTankPit() )
		return 1.0f;
	return 0.0f;
}

const bool CExecutorSoldierEntrench::IsEntrenchAllowed() const
{
	if ( !pUnit->IsInfantry() )
		return true;
	const SVector tile( pUnit->GetCenterTile() );
	return GetTerrain()->CanDigEntrenchment( tile.x, tile.y );
}

float CExecutorSoldierEntrench::OnAbilityOff()
{
	// check if it is time to disable ability
	const bool bAllowEntrenchNow = IsEntrenchAllowed();

	if ( bAllowEntrenchNow && GetState() == EASS_DISABLE )
		NotifyEvent( SExecutorEventParam( EID_ABILITY_ENABLE, 0, pUnit->GetUniqueId() ) );
	else if ( !bAllowEntrenchNow && GetState() != EASS_DISABLE )
		NotifyEvent( SExecutorEventParam( EID_ABILITY_DISABLE, 0, pUnit->GetUniqueId() ) );

	return 0.0f;
}

void CExecutorSoldierEntrench::SwitchingOffEnd()
{
}

void CExecutorSoldierEntrench::SwitchingOffStart()
{
	theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_MOVE_LEAVE_SELF_ENTRENCH), pUnit );
}

void CExecutorSoldierEntrench::SwitchOnEnd()
{
}

bool CExecutorSoldierEntrench::ActivateDuringDisable()
{
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_SELF_ENTRENCH), pUnit, false );
	return true;
}

void CExecutorSoldierEntrench::SwitchOnStart( const class CAICommand *pCommand )
{
	theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_SELF_ENTRENCH), pUnit, false );
}

CCommonUnit * CExecutorSoldierEntrench::GetUnit()
{
	return pUnit;
}

const bool CExecutorSoldierEntrench::IsValidInternal() const
{
	return IsValidObj( pUnit );
}

int CExecutorSoldierEntrench::Segment()
{
	if ( !IsValidInternal() )
		return -1;

	return CExecutorUnitBase::Segment();
}

bool CExecutorSoldierEntrench::NotifyEvent( const CExecutorEvent &event )
{
	if ( pUnit->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	const CExecutorEventSpecialAbilityActivate *pEv( static_cast<const CExecutorEventSpecialAbilityActivate *>( &event ) );
	if ( pEv->GetAbility() == GetAbility() )
		return CExecutorUnitBase::NotifyEvent( event );
	
	return false;
}

bool CExecutorSoldierEntrench::IsExecutorValid() const
{
	return IsValid( pUnit );
}

void CExecutorSoldierEntrench::RegisterOnEvents( IExecutorContainer *pContainer )
{
	std::vector<EExecutorEventID> events;
	events.push_back( EID_ABILITY_ACTIVATE );
	events.push_back( EID_ABILITY_DEACTIVATE );
	events.push_back( EID_NEW_COMMAND_RECIEVED );
	RegisterOnUnitEvents( pContainer, events, pUnit->GetUniqueId() );
}

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x110AB2C0, CExecutorSoldierEntrench )

