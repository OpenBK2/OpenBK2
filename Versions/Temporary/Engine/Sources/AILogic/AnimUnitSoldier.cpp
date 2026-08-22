#include "stdafx.h"

#include "AnimUnitSoldier.h"
#include "Soldier.h"
#include "NewUpdater.h"
#include "Stats_B2_M1/AnimationType.h"
#include "Formation.h"
#include "UnitStates.h"

#include "AILogic_export.h"

#include <cfenv>

extern CEventUpdater updater;
extern NTimer::STime curTime;

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1508D488, CAnimUnitSoldier );

void CAnimUnitSoldier::Init( CAIUnit *_pOwner )
{
	pOwner = checked_cast<CSoldier*>(_pOwner);
	nCurAnimation = NDb::ANIMATION_IDLE;
	timeOfFinishAnimation = 0;
	bMustFinishCurAnimation = true;
	
	pOwnerStats = checked_cast<const SInfantryRPGStats*>(pOwner->GetStats());
	bComplexAttack = !pOwnerStats->bCanAttackDown || !pOwnerStats->bCanAttackUp;
}

void CAnimUnitSoldier::Moved()
{
	if ( movingState.state == SMovingState::EMS_STOPPED || movingState.state == SMovingState::EMS_MOVING_TO_STOPPED )
	{
		movingState.state = SMovingState::EMS_STOPPED_TO_MOVING;
		movingState.timeOfIntentionStart = 0;
	}
}

void CAnimUnitSoldier::Stopped()
{
	if ( movingState.state == SMovingState::EMS_MOVING || movingState.state == SMovingState::EMS_STOPPED_TO_MOVING )
	{
		movingState.state = SMovingState::EMS_MOVING_TO_STOPPED;
		movingState.timeOfIntentionStart = curTime;
	}
}

void CAnimUnitSoldier::AnimationSet( int nAnimation )
{
	nCurAnimation = nAnimation;
	switch ( nAnimation )
	{
		case NDb::ANIMATION_IDLE:
		case NDb::ANIMATION_IDLE_DOWN:
		case NDb::ANIMATION_MOVE:
		case NDb::ANIMATION_CRAWL:
		case NDb::ANIMATION_DEATH:
		case NDb::ANIMATION_DEATH_DOWN:
		case NDb::ANIMATION_USE:
		case NDb::ANIMATION_USE_DOWN:
//		case NDb::ANIMATION_POINTING:
		case NDb::ANIMATION_BINOCULARS:
		case NDb::ANIMATION_BINOCULARS_DOWN:
		case NDb::ANIMATION_DEATH_FATALITY:
			bMustFinishCurAnimation = false;
			break;

		case NDb::ANIMATION_AIMING:
		case NDb::ANIMATION_AIMING_TRENCH:
		case NDb::ANIMATION_AIMING_DOWN:
			bMustFinishCurAnimation = true;
			timeOfFinishAnimation = curTime + pOwnerStats->GetAnimTime( nAnimation );
			timeOfFinishAnimation += 2000;
			break;
				
		case NDb::ANIMATION_SHOOT:
		case NDb::ANIMATION_SHOOT_DOWN:
			bMustFinishCurAnimation = true;
			{
				NI_ASSERT( std::fegetround() == FE_TONEAREST, "something changed processor control word" );
				const SInfantryRPGStats::SInfantryGun &gun = pOwnerStats->GetGun( pOwner->GetUniqueID(), 0, 0 );
				timeOfFinishAnimation = curTime + 
					(std::max)( pOwnerStats->GetAnimTime( nAnimation ),
					Float2Int( gun.pWeapon->nAmmoPerBurst * 60000 / gun.pWeapon->shells[0].fFireRate - 0.5f ) );
			}
			
			if ( bComplexAttack )
				timeOfFinishAnimation += 800;
			break;

		case NDb::ANIMATION_SHOOT_TRENCH:
		case NDb::ANIMATION_THROW: 
		case NDb::ANIMATION_THROW_TRENCH: 
//		case NDb::ANIMATION_RADIO:
		case NDb::ANIMATION_LIE:
		case NDb::ANIMATION_STAND:
		case NDb::ANIMATION_THROW_DOWN:
//		case NDb::ANIMATION_PRISONING:
			bMustFinishCurAnimation = true;
			timeOfFinishAnimation = curTime + pOwnerStats->GetAnimTime( nAnimation );
			break;

		case NDb::ANIMATION_INSTALL:
		case NDb::ANIMATION_UNINSTALL:
		case NDb::ANIMATION_INSTALL_ROT:
		case NDb::ANIMATION_UNINSTALL_ROT:
		case NDb::ANIMATION_INSTALL_PUSH:
		case NDb::ANIMATION_UNINSTALL_PUSH:
			NI_ASSERT( false, fmt::format( "Wrong animation for soldier ({})", nAnimation ) );

		default:
			NI_ASSERT( false, fmt::format( "Unknown animation for soldier ({})", nAnimation ) );
	}
}

void CAnimUnitSoldier::Segment()
{
	if ( movingState.state == SMovingState::EMS_STOPPED_TO_MOVING || 
			 movingState.state == SMovingState::EMS_MOVING_TO_STOPPED )
	{
		if ( movingState.timeOfIntentionStart + 100 < curTime )
		{
			movingState.state = 
				movingState.state == SMovingState::EMS_STOPPED_TO_MOVING ? 
														 SMovingState::EMS_MOVING : SMovingState::EMS_STOPPED;

			if ( nCurAnimation == NDb::ANIMATION_MOVE || nCurAnimation == NDb::ANIMATION_CRAWL ||
				/*!bMustFinishCurAnimation || */
						movingState.state == SMovingState::EMS_MOVING && nCurAnimation != NDb::ANIMATION_STAND && nCurAnimation != NDb::ANIMATION_LIE )
			{
				bMustFinishCurAnimation = false;												
				StopCurAnimation();
			}
		}
	}

	if ( bMustFinishCurAnimation && curTime >= timeOfFinishAnimation )
	{
		if ( movingState.state == SMovingState::EMS_MOVING || movingState.state == SMovingState::EMS_STOPPED_TO_MOVING )
		{
			updater.AddUpdate( 0, pOwner->GetMovingAction(), pOwner, -1 );
			movingState.state = SMovingState::EMS_MOVING;
		}
		else
		{
			updater.AddUpdate( 0, ACTION_NOTIFY_STOP, pOwner, -1 );
			updater.AddUpdate( 0, pOwner->GetIdleAction(), pOwner, -1 );
			movingState.state = SMovingState::EMS_STOPPED;
		}

		bMustFinishCurAnimation = false;
	}
}

void CAnimUnitSoldier::StopCurAnimation()
{
	//if ( pOwner->GetFormation()->GetState()->GetName() == EUSN_GUN_CREW_STATE )
		//return;
	if ( movingState.state == SMovingState::EMS_STOPPED )
	{
		updater.AddUpdate( 0, ACTION_NOTIFY_STOP, pOwner, -1 );
		updater.AddUpdate( 0, pOwner->GetIdleAction(), pOwner, -1 );
	}
	else
		updater.AddUpdate( 0, pOwner->GetMovingAction(), pOwner, -1 );
}


