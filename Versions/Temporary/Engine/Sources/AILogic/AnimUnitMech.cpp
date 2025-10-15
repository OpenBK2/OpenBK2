#include "stdafx.h"

#include "AnimUnitMech.h"
#include "AIUnit.h"
#include "NewUpdater.h"

#include "AILogic_export.h"

extern CEventUpdater updater;
extern NTimer::STime curTime;

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1508D487, CAnimUnitMech );
BASIC_REGISTER_CLASS( AILOGIC, IAnimUnit );

void CAnimUnitMech::Init( CAIUnit *_pOwner )
{
	pOwner = _pOwner;
}

void CAnimUnitMech::Moved()
{
	if ( movingState.state == SMovingState::EMS_STOPPED || movingState.state == SMovingState::EMS_MOVING_TO_STOPPED )
	{
		movingState.state = SMovingState::EMS_STOPPED_TO_MOVING;
		movingState.timeOfIntentionStart = curTime;
	}
}

void CAnimUnitMech::Stopped()
{
	if ( movingState.state == SMovingState::EMS_MOVING || movingState.state == SMovingState::EMS_STOPPED_TO_MOVING )
	{
		movingState.state = SMovingState::EMS_MOVING_TO_STOPPED;
		movingState.timeOfIntentionStart = curTime;
	}
}

void CAnimUnitMech::AnimationSet( int nAnimation )
{
}

void CAnimUnitMech::Segment()
{
	if ( pOwner->IsAlive() )
	{
		if ( movingState.state == SMovingState::EMS_STOPPED_TO_MOVING || movingState.state == SMovingState::EMS_MOVING_TO_STOPPED )
		{
			if ( movingState.timeOfIntentionStart + 200 < curTime )
			{
				movingState.state = 
					movingState.state == SMovingState::EMS_STOPPED_TO_MOVING ? SMovingState::EMS_MOVING : SMovingState::EMS_STOPPED;

				if ( movingState.state == SMovingState::EMS_STOPPED )
				{
					updater.AddUpdate( 0, ACTION_NOTIFY_STOP, pOwner, -1 );
					updater.AddUpdate( 0, pOwner->GetIdleAction(), pOwner, -1 );
				}
				else
					updater.AddUpdate( 0, pOwner->GetMovingAction(), pOwner, -1 );
			}
		}
	}
}


