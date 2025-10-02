#include "StdAfx.h"
#include "./executorsnipercamouflage.h"
#include "Soldier.h"
#include "UnitsIterators2.h"
#include "NewUpdater.h"

extern NTimer::STime curTime;
extern CEventUpdater updater;

REGISTER_SAVELOAD_CLASS( 0x11082300, CExecutorSniperCamouflage )


CExecutorSniperCamouflage::CExecutorSniperCamouflage( CAIUnit *_pUnit, bool _bAdvanced )
	: CExecutor(TID_SNIPER_CAMOUFLAGE, 1000/SConsts::AI_SEGMENT_DURATION), 
	pUnit( _pUnit ), nEnemysAround( 0 ), bWasSomeAction( false ), fCamoflage( 0.0f ),
	eState( EASS_OFF ), timeLastUpdate( 0 ), timeDisableGroup( 0 ), eStateBeforeDisable( EASS_OFF ), bMoving( false ),
	bAdvanced( _bAdvanced ), nBeginActionTime( curTime ), nLastActionTime( curTime ), bInAction( false ), bStartCounting( false ),
	eAbilityType( _bAdvanced ? NDb::ABILITY_ADAVNCED_CAMOFLAGE_MODE : NDb::ABILITY_CAMOFLAGE_MODE ) 
{ 
}

bool CExecutorSniperCamouflage::IsExecutorValidInternal() const
{
	return IsValidObj( pUnit );
}

int CExecutorSniperCamouflage::Segment()
{
	if ( !IsExecutorValidInternal() )
		return -1;

	if ( EASS_DISABLE != eState )
	{
		// calc enemies around
		nEnemysAround = 0;
		const CVec2 vPos( pUnit->GetCenterPlain() );
		for ( CUnitsIter<0,2> it( pUnit->GetParty(), EDI_ENEMY, vPos, SConsts::SPY_GLASS_RADIUS ); !it.IsFinished(); it.Iterate() )
		{
			if ( (*it)->IsRefValid() && (*it)->IsAlive() && (*it)->GetSightRadius() > fabs( vPos - (*it)->GetCenterPlain() ) )
				++nEnemysAround;
		}

		if ( bAdvanced )
		{
			if ( bMoving || bWasSomeAction )
			{
				if ( !bInAction )
				{
					bInAction = true;
					nBeginActionTime = curTime;
				}
				nLastActionTime = curTime;
			}
			else if ( curTime - nLastActionTime > SConsts::SCamouflage::SNEAK_ADVANCED_RESET_TIMEOUT )
				bInAction = false;
			bStartCounting = !bInAction || ( curTime - nBeginActionTime > SConsts::SCamouflage::SNEAK_ADVANCED_TIMEOUT );

			if ( eState == EASS_ACTIVE )
			{
				for ( CUnitsIter<0,2> it( pUnit->GetParty(), EDI_ENEMY, vPos, 5 * SConsts::TILE_SIZE ); !it.IsFinished(); it.Iterate() )
				{
					if ( (*it)->IsRefValid() && (*it)->IsAlive() )
					{
						fCamoflage = 0.0f;
						return UpdateCamoflage( eAbilityType );
					}
				}
			}
		}

		if ( !bAdvanced || bStartCounting )
		{
			if ( bMoving )
			{
				fCamoflage = Max( 0.0f, fCamoflage - nEnemysAround * SConsts::AI_SEGMENT_DURATION * SConsts::SCamouflage::SNEAK_DECREASE_PER_SECOND_MOVE / 1000 );
			}
			else if ( !bWasSomeAction && !bMoving )
			{
				fCamoflage = Min( 1.0f, fCamoflage + SConsts::AI_SEGMENT_DURATION * SConsts::SCamouflage::SNEAK_ADDITION_PER_TIME / 1000 );
			}
		}
		bWasSomeAction = false;
	}

	return UpdateCamoflage( eAbilityType );
}

void CExecutorSniperCamouflage::UpdateState( const enum EAbilitySwitchState eState, const float fParam )
{
	if ( !pUpdate )
		pUpdate = new SAISpecialAbilityUpdate;

	pUpdate->info.nAbilityType = eAbilityType;
	pUpdate->info.state.eState = eState;
	pUpdate->info.fCurValue = fParam;
	pUpdate->info.nObjUniqueID = pUnit->GetUniqueId();

	if ( 0 != memcmp( &lastSent, &pUpdate->info, sizeof(lastSent) ) )
	{
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pUnit, -1 );
		lastSent = pUpdate->info;
		pUpdate = 0;
	}
}

void CExecutorSniperCamouflage::UnCamoUnit()
{
	// SET UNIT TO visible STATE
	pUnit->RemoveCamouflage( ECRR_USER_COMMAND );
}

void CExecutorSniperCamouflage::CamoUnit()
{
	// SET UNIT TO CAMO STATE
	pUnit->SetCamoulfage();
}

int CExecutorSniperCamouflage::UpdateCamoflage( const EUnitSpecialAbility eAbility )
{
	switch( eState )
	{
	case EASS_ACTIVE:
		UpdateState( EASS_ACTIVE, fCamoflage );
		if ( 0.0f == fCamoflage )
		{
			UnCamoUnit();
			eState = EASS_SWITCHING_OFF;
		}

		break;
	case EASS_DISABLE:
		{
			const int nTimePassed = curTime - timeLastUpdate;
			const int nTimeNeeded = timeDisableGroup;
			if ( nTimePassed >= nTimeNeeded )
				eState = eStateBeforeDisable;
			else
				UpdateState( EASS_DISABLE, float(nTimePassed) / float(nTimeNeeded) );
		}	

		break;
	case EASS_OFF:
		{
			if ( 1.0f == fCamoflage )
			{
				eState = EASS_READY_TO_ON;
				UpdateState( EASS_READY_TO_ON, fCamoflage );
			}
			else
			{
				UpdateState( EASS_OFF, fCamoflage );
			}
		}	

		break;
	case EASS_SWITCHING_ON:
		{
			const int nTimePassed = curTime - timeLastUpdate;
			if ( const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetUnitAbilityDesc( eAbility ) )
			{
				const int nTimeNeeded = pSA->nSwitchOnTime;
				if ( nTimePassed >= nTimeNeeded )
				{
					eState = EASS_ACTIVE;
					CamoUnit();
					fCamoflage = 1.0f;
				}
				else
					UpdateState( EASS_SWITCHING_ON, float(nTimePassed) / float(nTimeNeeded) );
			}
		}

		break;
	case EASS_SWITCHING_OFF:
		{
			const int nTimePassed = curTime - timeLastUpdate;
			if ( const NDb::SUnitSpecialAblityDesc *pSA = pUnit->GetUnitAbilityDesc( eAbility ) )
			{
				const int nTimeNeeded = pSA->nSwitchOffTime;
				if ( nTimePassed >= nTimeNeeded )
				{
					UnCamoUnit();
					eState = EASS_OFF;
					fCamoflage = 0.0f;
				}
				else
					UpdateState( EASS_SWITCHING_OFF, float(nTimePassed) / float(nTimeNeeded) );
			}
		}

		break;
	case EASS_READY_TO_ON:
		//CRAP{ executor must sleep now
		//CRAP}

		break;
	}
	return GetNextTime();
}

void CExecutorSniperCamouflage::RegisterOnEvents( IExecutorContainer *pContainer )
{
	SExecutorEventParam par;
	par.nUnitID = pUnit->GetUniqueId();

	par.eEventID = EID_NEW_COMMAND_RECIEVED;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_FIRE_CAMOFLATED;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_START_MOVE;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_START_IDLE;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_ACTIVATE;
	pContainer->RegisterOnEvent( this, par );

	par.eEventID = EID_ABILITY_DEACTIVATE;
	pContainer->RegisterOnEvent( this, par );
}

bool CExecutorSniperCamouflage::IsCamoflageAbility( const EUnitSpecialAbility eAbility )
{
	return eAbility == NDb::ABILITY_CAMOFLAGE_MODE || eAbility == NDb::ABILITY_ADAVNCED_CAMOFLAGE_MODE;
}

bool CExecutorSniperCamouflage::NotifyEvent( const CExecutorEvent &event )
{
	if ( pUnit->GetUniqueId() != event.GetParam().nUnitID )
		return false;

	const CExecutorEventSpecialAbility *pEv = static_cast<const CExecutorEventSpecialAbility *>( &event );
	if ( pEv == 0 )
		return false;

	if ( pEv->GetAbility() != eAbilityType )
		return false;

	const EExecutorEventID eCurID = event.GetParam().eEventID;
	switch( eCurID )
	{
	case EID_FIRE_CAMOFLATED:
		bWasSomeAction = true;
		if ( !bAdvanced || bStartCounting )
			fCamoflage = Max( 0.0f, fCamoflage - nEnemysAround * SConsts::SCamouflage::SNEAK_DECREASE_PER_SHOT );
		
		return true;
	case EID_START_IDLE:
		// mark as if unit is camoflating
		if ( pEv->GetAbility() != eAbilityType )
			return false;
		bMoving = false;

		break;
	case EID_START_MOVE:
		if ( pEv->GetAbility() != eAbilityType )
			return false;
		
		bMoving = true;

		return true;
	case EID_ABILITY_DEACTIVATE:
	case EID_NEW_COMMAND_RECIEVED:
		if ( EID_ABILITY_DEACTIVATE == eCurID && pEv->GetAbility() != eAbilityType )
			return false;
		if ( EID_NEW_COMMAND_RECIEVED == eCurID && eState != EASS_SWITCHING_ON )
			return false;
		{
			const CExecutorEventSpecialAbilityActivate *pEv( static_cast<const CExecutorEventSpecialAbilityActivate *>( &event ) );

			// the parent unit notified executor about ability start.
			if ( pEv->GetParam().nUnitID == pUnit->GetUniqueId() ) 
			{
				// this ablity finished
				timeLastUpdate = curTime;
				UnCamoUnit();
				eState = EASS_SWITCHING_OFF;
			}
		}

		return true;
	case EID_ABILITY_ACTIVATE:
		{
			if ( pEv->GetAbility() != eAbilityType )
				return false;
			const CExecutorEventSpecialAbilityActivate *pEv( static_cast<const CExecutorEventSpecialAbilityActivate *>( &event ) );
			// the parent unit notified executor about ability start.
			if ( pEv->GetParam().nUnitID == pUnit->GetUniqueId() )
			{
				if ( eAbilityType == pEv->GetAbility() ) // this ablity started
				{
					timeLastUpdate = curTime;
					eState = EASS_SWITCHING_ON;
				}
				else if ( pUnit->GetUnitAbilityDesc( eAbilityType ) && pUnit->GetUnitAbilityDesc( eAbilityType )->eGroupID == pEv->GetDesc()->eGroupID ) // ability from this group started
				{
					if ( EASS_DISABLE != eState )
						eStateBeforeDisable = eState;
					eState = EASS_DISABLE;
					//CRAP{ 
					//UnCamoUnit();
					//CRAP}
					timeDisableGroup = pEv->GetDesc()->nDisableGroupTime;
				}
			}
		}
		return true;
	}
	return false;
}

