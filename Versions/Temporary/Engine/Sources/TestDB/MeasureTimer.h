#pragma once
#include "../Misc/HPTimer.h"

namespace NTest
{

class CMeasureTimer
{
	NHPTimer::STime timeStart;
public:
	CMeasureTimer( const char *pszTestName )
	{
		DebugTrace( "*** Start test: %s ***", pszTestName );
		NHPTimer::GetTime( &timeStart );
	}
	~CMeasureTimer()
	{
		double fSeconds = NHPTimer::GetTimePassed( &timeStart );
		DebugTrace( "*** Test passed in %g msec ***", float(fSeconds * 1000.0f) );
	}
};

}
