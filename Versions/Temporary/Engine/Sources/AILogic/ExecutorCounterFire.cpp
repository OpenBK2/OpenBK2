#include "StdAfx.h"

#include "ExecutorCounterFire.h"
#include "NewUpdater.h"
#include "GroupLogic.h"
#include "Artillery.h"
#include "AntiArtillery.h"
#include "AntiArtilleryManager.h"
#include "UnitStates.h"
#include "Guns.h"
#include "Stats_B2_M1/StatusUpdates.h"

extern NTimer::STime curTime;
extern CEventUpdater updater;
extern CGroupLogic theGroupLogic;
extern CAntiArtilleryManager theAAManager;

#define RETARGET_DELTA	10.0f

CExecutorCounterFire::CExecutorCounterFire( CArtillery *_pUnit ) :
pUnit( _pUnit ), CExecutor( TID_COUNTER_FIRE, 1000/SConsts::AI_SEGMENT_DURATION ), fSearchRadius( 0.0f ), 
bBonusApplied( false ), vSearchCenter( VNULL2 ), vLastCirclePos( VNULL2 ), timeLastHeard( curTime )
{
	state.eState = EASS_READY_TO_ON;
	state.bAutocast = false;
	stateBeforeDisable.eState = EASS_DISABLE;
	stateBeforeDisable.bAutocast = false;
}

void CExecutorCounterFire::UpdateState( )
{
	if ( !pUpdate )
		pUpdate = new SAISpecialAbilityUpdate;

	pUpdate->info.nAbilityType = NDb::ABILITY_ANTIATRILLERY_FIRE;
	pUpdate->info.state = state;
	pUpdate->info.fCurValue = 0.0f;
	pUpdate->info.nObjUniqueID = pUnit->GetUniqueId();

	if ( 0 != memcmp( &lastSent, &pUpdate->info, sizeof(lastSent) ) )
	{
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pUnit, -1 );
		lastSent = pUpdate->info;
		pUpdate = 0;
	}

	if ( state.eState == EASS_DISABLE )
		updater.AddUpdate( CreateStatusUpdate( EUS_COUNTER_FIRE, false, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, pUnit, -1 );
	else
		updater.AddUpdate( CreateStatusUpdate( EUS_COUNTER_FIRE, true, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, pUnit, -1 );
}

void CExecutorCounterFire::ApplyBonus( const bool bForward )
{
	if ( !IsValid( pUnit ) )
		return;

	const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetUnitAbilityDesc( NDb::ABILITY_ANTIATRILLERY_FIRE );

	if ( pSA && pSA->pStatsBonus && bForward != bBonusApplied )
	{
		pUnit->ApplyStatsModifier( pSA->pStatsBonus, bForward );
		bBonusApplied = bForward;
	}
}

int CExecutorCounterFire::Segment()
{
	if ( !IsExecutorValid() || !IsValid( pUnit ) )
		return -1;					// The executor is to be deleted
		
	const EUnitStateNames eUnitState = pUnit->GetState()->GetName();
	const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetUnitAbilityDesc( NDb::ABILITY_ANTIATRILLERY_FIRE );
	if ( !pSA )
		return -1;
	CSegment marker;
	CCircle searchCircle( vSearchCenter, fSearchRadius );

	if ( state.eState == EASS_ACTIVE && eUnitState != EUSN_BOMBARDMANET )
	{																												//Stopped firing for some reason (user command, no ammo, etc)
		state.eState = EASS_READY_TO_ON;
		state.bAutocast = false;
		//Remove bonus
		ApplyBonus( false );
		UpdateState();
	}
	else switch( state.eState ) 
	{
	case EASS_SWITCHING_ON:					//Need to choose target (or check if started firing)
		{
			if ( eUnitState == EUSN_BOMBARDMANET )		//Started firing where we ordered
			{
				state.eState = EASS_ACTIVE;
			}
			else									//Choose target
			{
				bool bTargetFound = false;
				pCurrentTarget = 0;

				//Cycle through all anti-artilleries
				CCircle curCircle;
				float fDist2;
				float fMinDist2 = fSearchRadius * fSearchRadius;
				for( CAntiArtilleryManager::CIterator it( pUnit->GetParty() ); !it.IsFinished(); it.Iterate() )
				{
					curCircle = *it;
					/*marker.p1 = CVec2( curCircle.center.x - 50, curCircle.center.y );
					marker.p2 = CVec2( curCircle.center.x + 50, curCircle.center.y );
					marker.dir = marker.p2 - marker.p1;
					DebugInfoManager()->CreateSegment( OBJECT_ID_FORGET, marker, 2, NAIVisInfo::RED );*/

					fDist2 = fabs2( vSearchCenter - curCircle.center );
					//CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, StrFmt( "AArt: (%f,%f) - %f ", curCircle.center.x, curCircle.center.y, fDist2 ) );
					if ( fDist2 < fMinDist2 )
					{
						/*marker.p1 = CVec2( curCircle.center.x, curCircle.center.y - 50 );
						marker.p2 = CVec2( curCircle.center.x, curCircle.center.y + 50 );
						marker.dir = marker.p2 - marker.p1;
						DebugInfoManager()->CreateSegment( OBJECT_ID_FORGET, marker, 2, NAIVisInfo::RED );*/

						pCurrentTarget = it.GetAntiArtillery();
						fMinDist2 = fDist2;
						bTargetFound = true;
					}

				}

				if( bTargetFound )			//Issue command if target found
				{
					vLastCirclePos = pCurrentTarget->GetLastRevealCenter( pUnit->GetParty() );
					SAIUnitCmd cmd( ACTION_COMMAND_ART_BOMBARDMENT, vLastCirclePos );
					theGroupLogic.UnitCommand( cmd, pUnit, true );

					/*marker.p1 = CVec2( vTarget.x - 30, vTarget.y - 30 );
					marker.p2 = CVec2( vTarget.x + 30, vTarget.y + 30 );
					marker.dir = marker.p2 - marker.p1;
					DebugInfoManager()->CreateSegment( OBJECT_ID_FORGET, marker, 2, NAIVisInfo::WHITE );
					marker.p1 = CVec2( vTarget.x - 30, vTarget.y + 30 );
					marker.p2 = CVec2( vTarget.x + 30, vTarget.y - 30 );
					marker.dir = marker.p2 - marker.p1;
					DebugInfoManager()->CreateSegment( OBJECT_ID_FORGET, marker, 2, NAIVisInfo::WHITE );*/
				}
				else
					state.eState = EASS_READY_TO_ON;

				ApplyBonus( bTargetFound );
			}

			UpdateState();
		}
		break;
	case EASS_ACTIVE:								// Check if expired, retarget if necessary
		{
			if ( IsValid( pCurrentTarget ) )		// Target alive, check when it last shot.
				timeLastHeard = pCurrentTarget->GetLastHeardTime( pUnit->GetParty() );
			//otherwise, keep previous time

			if ( curTime - timeLastHeard >= pSA->nWorkTime )				// No activity for specified time
			{
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), pUnit, false );
				state.eState = EASS_READY_TO_ON;
				ApplyBonus( false );
			}
			else if ( IsValid( pCurrentTarget ) )
			{					// Check if the circle moved since last time
				const CVec2 vOldPos( vLastCirclePos );
				vLastCirclePos = pCurrentTarget->GetLastRevealCenter( pUnit->GetParty() );
				
				if ( fabs2( vOldPos - vLastCirclePos ) > RETARGET_DELTA )			// Circle moved sufficiently, retarget
				{
					const CVec2 vNewPos = ( vLastCirclePos + vOldPos ) * 0.5f;
					SAIUnitCmd cmd( ACTION_COMMAND_ART_BOMBARDMENT, vNewPos );
					theGroupLogic.UnitCommand( cmd, pUnit, true );
				}
			}
			UpdateState();
		}
		break;
	case EASS_READY_TO_ON:
		{
			if ( state.bAutocast )			//Start searching for target immediately
				state.eState = EASS_SWITCHING_ON;
		}
		break;
	}

	//theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_ART_BOMBARDMENT, iter->vAttackPos ), pUnit, false );

	return GetNextTime();
}

bool CExecutorCounterFire::NotifyEvent( const CExecutorEvent &event )
{
	//Filter out wrong calls
	if ( pUnit->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	const CExecutorEventSpecialAbility *pEv = static_cast<const CExecutorEventSpecialAbility *>( &event );
	if ( pEv == 0 )
		return false;

	if ( pEv->GetAbility() != NDb::ABILITY_ANTIATRILLERY_FIRE )
		return false;
	//end - Filter out wrong calls

	switch( event.GetParam().eEventID ) 
	{
	case EID_ABILITY_SET_TARGET:
		{
			const CExecutorEventSpecialAbilitySetTarget *pEvSetTarget 
				= static_cast<const CExecutorEventSpecialAbilitySetTarget*>( pEv );
			NI_ASSERT( pEvSetTarget, "Wrong event passed to executor (SetTarget expected)" );
			if ( !pEvSetTarget ) 
				return false;

			state.eState = EASS_SWITCHING_ON;
			state.bAutocast = false;

			const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetUnitAbilityDesc( NDb::ABILITY_ANTIATRILLERY_FIRE );
			vSearchCenter = pEvSetTarget->GetTargetCoords();
			fSearchRadius = pSA->fParameter;				//Parameter in ability is search radius
		}
		return true;
	case EID_ABILITY_ACTIVATE_AUTOCAST:
		{
			vSearchCenter = CVec2( pUnit->GetCenter().x, pUnit->GetCenter().y );
			fSearchRadius = pUnit->GetFirstArtilleryGun()->GetFireRangeMax();

			state.bAutocast = true;
			state.eState = EASS_READY_TO_ON;
			UpdateState();
		}
		return true;
	case EID_ABILITY_DEACTIVATE_AUTOCAST:
		{
			state.bAutocast = false;
			state.eState = EASS_READY_TO_ON;
			UpdateState();
		}
		return true;
	case EID_ABILITY_ENABLE:
		{
			state.eState = EASS_READY_TO_ON;
			state.bAutocast = false;
			UpdateState();
		}
		return true;
	case EID_ABILITY_DISABLE:
		{
			state.eState = EASS_DISABLE;
			state.bAutocast = false;
			UpdateState();
		}
		return true;
	}
	return false;
}

void CExecutorCounterFire::RegisterOnEvents( IExecutorContainer *pContainer )
{
	SExecutorEventParam par;
	par.nUnitID = pUnit->GetUniqueId();

	par.eEventID = EID_ABILITY_SET_TARGET;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_ACTIVATE_AUTOCAST;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_DEACTIVATE_AUTOCAST;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_ENABLE;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_DISABLE;
	pContainer->RegisterOnEvent( this, par );
}

REGISTER_SAVELOAD_CLASS( 0x1912D300, CExecutorCounterFire )



