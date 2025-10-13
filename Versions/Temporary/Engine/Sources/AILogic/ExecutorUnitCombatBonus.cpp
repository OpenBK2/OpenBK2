#include "stdafx.h"

#include "ExecutorUnitCombatBonus.h"
#include "UnitStates.h"
#include "Guns.h"

#include <cstdint>

CExecutorUnitCombatBonus::CExecutorUnitCombatBonus( EUnitSpecialAbility eAbility, CAIUnit *_pUnit, EExecutorTypeID eTID ) :
CExecutorUnitBonus ( eAbility, _pUnit, eTID ), modeFlags ( 0 )
{
}

int CExecutorUnitCombatBonus::Segment()
{
	if ( !IsExecutorValid() ) 
		return -1;

	uint16_t oldMode = modeFlags;
	modeFlags = 0;
	if ( GetAIUnit()->IsMoving() )
		modeFlags |= EUM_MOVING;

	if ( GetAIUnit()->GetState()->IsAttackingState() )
	{
		for ( int i = 0; i < GetAIUnit()->GetNGuns(); ++i )
		{
			if ( GetAIUnit()->GetGun( i )->IsFiring() )
			{
				modeFlags |= EUM_FIGHTING;
				break;
			}
		}
	}

	EAbilityCombatReaction eReaction = EACR_IGNORE;

	if ( oldMode != modeFlags )									//Unit started/stopped moving/fighting
		eReaction = OnModeChange( oldMode, modeFlags );

	//Process reaction
	switch ( eReaction ) 
	{
	case EACR_FORCE_ACTIVATE:
		ForceActivate();
		break;
	case EACR_FORCE_DEACTIVATE:
		ForceDeactivate();
		break;
	case EACR_FORCE_RECHARGE:
		ForceRecharge();
		break;
	}

	return CExecutorUnitBonus::Segment();
}

int CExecutorUnitCombatBonus::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CExecutorUnitBonus*>( this ) );
	saver.Add( 2, &modeFlags );

	return 0;
}

//REGISTER_SAVELOAD_CLASS( 0x19128500, CExecutorUnitCombatBonus )


