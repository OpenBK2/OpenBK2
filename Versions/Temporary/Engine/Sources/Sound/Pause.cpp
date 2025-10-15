#include "stdafx.h"
#include "Pause.h"

#include "MusicSystem.hpp"
#include "DBMusicSystem.h"
#include "Misc/Win32Random.h"

namespace NMusicSystem
{


// CPause


void CPause::Segment() 
{ 
	switch( eState )
	{
	case EPS_NOT_STARTED:
		timeLastCall = GetAbsTime();
		eState = EPS_ACTIVE;

		break;
	case EPS_ACTIVE:
		{
			const NTimer::STime curTime = GetAbsTime();
			timePaused += curTime - timeLastCall;
			timeLastCall = curTime;
			if ( timePaused >= timeToPause )
				eState = EPS_FINISHED;
		}

		break;
	case EPS_FINISHED:
		
		break;
	}
}

void CPause::Play()
{
	timeLastCall = GetAbsTime();
}

void CPause::OnResetTimer()
{
	timeLastCall = GetAbsTime();
}


bool CPause::IsFinished() const 
{ 
	return eState == EPS_FINISHED;
}

CPause::CPause( const NDb::SPlayPause &_Pause )
: timeToPause( _Pause.nPauseTime + NWin32Random::Random(0, _Pause.nPauseRandom ) ),
	eState( EPS_NOT_STARTED ), timeLastCall ( 0 ), timePaused( 0 )
{  
}
}

REGISTER_SAVELOAD_CLASS_NM( SOUND, 0x111813C2, CPause, NMusicSystem )

