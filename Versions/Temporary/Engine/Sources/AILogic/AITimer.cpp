#include "StdAfx.h"
#include "..\system\time.h"
#include ".\aitimer.h"
#include "..\Main\GameTimer.h"


IGameTimer * CAITimer::pTimer = 0;

CAITimer::CAITimer()
{
	pTimer = Singleton<IGameTimer>();
}

void CAITimer::ToClientTime( NTimer::STime *pTime )
{
	//pTime -= SConsts::AI_SEGMENT_DURATION * 3500;
}

NTimer::STime CAITimer::GetSegmentTime()
{
	return pTimer->GetSegmentTime() ;//+ SConsts::AI_SEGMENT_DURATION * 3500 ;
}

NTimer::STime CAITimer::GetGameTime() 
{
	return pTimer->GetGameTime() ;//+ SConsts::AI_SEGMENT_DURATION * 3500 ;
}

void CAITimer::SetSpeed( int nSpeed )
{
	pTimer->SetSpeed( nSpeed );
}


