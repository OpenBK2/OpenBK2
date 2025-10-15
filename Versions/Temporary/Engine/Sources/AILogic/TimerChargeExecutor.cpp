#include "stdafx.h"

#include "TimerChargeExecutor.h"

#include "AILogic_export.h"

extern NTimer::STime curTime;

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x12086B40, CTimerChargeExecutor )

CTimerChargeExecutor::CTimerChargeExecutor( CMineStaticObject *_pObj, int nOffset ) 
: pCharge( _pObj ), nTimeToExplode( curTime + nOffset ),
	CExecutor( TID_TIMED_CHARGE, 1 )
{
}

int CTimerChargeExecutor::Segment()
{
	if ( !IsValidObj( pCharge ) )
		return -1;
	int nDiff = nTimeToExplode - curTime;
	if ( nDiff > 1000 )
		return 1000 / SAIConsts::AI_SEGMENT_DURATION;
	if ( nDiff > 0 )
		return nDiff / SAIConsts::AI_SEGMENT_DURATION ;
	pCharge->Die( 1.0f );
	return -1;
}

bool CTimerChargeExecutor::IsExecutorValid() const
{
	return IsValidObj( pCharge );
}


