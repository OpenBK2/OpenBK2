#include "stdafx.h"

#include "ExecutorSpyMode.h"
#include "NewUpdater.h"

extern CEventUpdater updater;

CExecutorSpyMode::CExecutorSpyMode( CAIUnit *_pUnit	) :
CExecutorUnitCombatBonus ( NDb::ABILITY_SPY_MODE, _pUnit, TID_SPY_MODE ), bMasked( false )
{
}

CExecutorUnitCombatBonus::EAbilityCombatReaction CExecutorSpyMode::OnModeChange( const WORD oldModeFlags, const WORD newModeFlags )
{
	if ( GetState() == EASS_ACTIVE && ( newModeFlags & CExecutorUnitCombatBonus::EUM_FIGHTING ) )
	{
		return EACR_FORCE_DEACTIVATE;
	}
	else if ( GetState() == EASS_SWITCHING_ON && newModeFlags )
	{
		return EACR_FORCE_RECHARGE;
	}
	return EACR_IGNORE;
}

void CExecutorSpyMode::SwitchingOffEnd()
{
	CExecutorUnitCombatBonus::SwitchingOffEnd();

	// Change model back
	if ( bMasked )
	{
		SwitchModel( GetAIUnit()->GetStats() );
		pEnemyStats = 0;
		bMasked = false;
	}

	GetAIUnit()->SetBehaviourFire( SBehaviour::EFAtWill );
	GetAIUnit()->RemoveCamouflage( ECRR_USER_COMMAND );
}

void CExecutorSpyMode::SwitchOnEnd()
{
	CExecutorUnitCombatBonus::SwitchOnEnd();

	// Change model to enemy, if not yet
	if ( !bMasked && pEnemyStats != 0 )
	{
		SwitchModel( pEnemyStats );

		bMasked = true;
	}

	GetAIUnit()->SetBehaviourFire( SBehaviour::EFNoFire );
	GetAIUnit()->SetCamoulfage();
}

void CExecutorSpyMode::SwitchModel( const NDb::SHPObjectRPGStats *pStats )
{
	CPtr<SAIChangeDBIDUpdate> pUpdate = new SAIChangeDBIDUpdate;

	pUpdate->nObjUniqueID = GetAIUnit()->GetUniqueID();
	pUpdate->info.pStats = pStats;
	updater.AddUpdate( pUpdate, ACTION_NOTIFY_CHANGE_DBID, GetAIUnit(), -1 );

	updater.AddUpdate( 0, ACTION_NOTIFY_IDLE, GetAIUnit(), -1 );
}

void CExecutorSpyMode::RegisterOnEvents( IExecutorContainer *pContainer )
{
	std::vector<EExecutorEventID> unitEvents;

	unitEvents.push_back( EID_ABILITY_SET_TARGET );

	RegisterOnUnitEvents( pContainer, unitEvents, pUnit->GetUniqueId() );

	CExecutorUnitCombatBonus::RegisterOnEvents( pContainer );
}

bool CExecutorSpyMode::NotifyEvent( const CExecutorEvent &event )
{
	//Filter out wrong calls
	if ( GetAIUnit()->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	const CExecutorEventSpecialAbility *pEv = static_cast<const CExecutorEventSpecialAbility *>( &event );
	if ( pEv == 0 )
		return false;

	if ( pEv->GetAbility() != GetAbility() )
		return false;
	//end - Filter out wrong calls

	if ( event.GetParam().eEventID == EID_ABILITY_SET_TARGET )
	{
		const CExecutorEventSpecialAbilitySetTarget *pEST = static_cast<const CExecutorEventSpecialAbilitySetTarget *>( pEv );
		if ( !pEST )
			return false;

		pEnemyStats = pEST->GetTarget();
		if ( GetState() == EASS_ACTIVE && !bMasked )
		{
			SwitchModel( pEnemyStats );
			bMasked = true;
		}
	}

	return CExecutorUnitCombatBonus::NotifyEvent( event );
}

int CExecutorSpyMode::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CExecutorUnitCombatBonus*>( this ) );
	saver.Add( 2, &pEnemyStats );
	saver.Add( 3, &bMasked );

	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x1916C380, CExecutorSpyMode )


