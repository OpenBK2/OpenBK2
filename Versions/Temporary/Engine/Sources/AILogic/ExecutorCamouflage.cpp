#include "StdAfx.h"

#include "ExecutorCamouflage.h"
#include "UnitsIterators2.h"
#include "Formation.h"
#include "Soldier.h"

extern NTimer::STime curTime;
extern CDiplomacy theDipl;

CExecutorCamouflage::CExecutorCamouflage( CAIUnit *_pUnit	) :
CExecutorUnitCombatBonus ( NDb::ABILITY_CAMOFLAGE_MODE, _pUnit, TID_CAMOUFLAGE ), nextCheckTime( 0 )
{
}

CExecutorUnitCombatBonus::EAbilityCombatReaction CExecutorCamouflage::OnModeChange( const WORD oldModeFlags, const WORD newModeFlags )
{
	if ( GetState() == EASS_ACTIVE && ( newModeFlags & ~CExecutorUnitCombatBonus::EUM_MOVING ) )
	{					// Switch camo off if doing anything but moving
		return EACR_FORCE_DEACTIVATE;
	}
	else if ( GetState() == EASS_SWITCHING_ON && newModeFlags )
	{
		return EACR_FORCE_RECHARGE;
	}
	return EACR_IGNORE;
}

int CExecutorCamouflage::Segment()
{
	const BYTE cPartyToCheck = GetAIUnit()->GetParty() == theDipl.GetNeutralParty() ? theDipl.GetNeutralParty() : 1 - GetAIUnit()->GetParty();
	if ( GetState() == EASS_ACTIVE && GetAIUnit()->IsVisible( cPartyToCheck ) )
	{
		GetAIUnit()->SetBehaviourFire( SBehaviour::EFAtWill );
		ForceDeactivate();
	}

	return CExecutorUnitCombatBonus::Segment();
}

void CExecutorCamouflage::SwitchingOffEnd()
{
	CExecutorUnitCombatBonus::SwitchingOffEnd();

	GetAIUnit()->SetBehaviourFire( SBehaviour::EFAtWill );
	GetAIUnit()->RemoveCamouflage( ECRR_USER_COMMAND );
}

void CExecutorCamouflage::SwitchOnEnd()
{
	CExecutorUnitCombatBonus::SwitchOnEnd();

	nextCheckTime = curTime;

	if ( GetAIUnit()->IsInfantry() )
	{	// Set camo to all units in formation simultaneously
		CFormation *pFormation = GetAIUnit()->GetFormation();
		if ( pFormation )
		{
			for ( int i = 0; i < pFormation->Size(); ++i )
			{
				if ( (*pFormation)[i]->GetUnitAbilityDesc( NDb::ABILITY_CAMOFLAGE_MODE ) )
				{
					(*pFormation)[i]->SetCamoulfage();
					(*pFormation)[i]->SetBehaviourFire( SBehaviour::EFNoFire );
				}
			}
		}
	}
	else
	{
		GetAIUnit()->SetBehaviourFire( SBehaviour::EFNoFire );
		GetAIUnit()->SetCamoulfage();
	}
}

REGISTER_SAVELOAD_CLASS( 0x19139380, CExecutorCamouflage )

