#include "StdAfx.h"
#include "HPTimer.h"

using namespace NHPTimer;
static double fProcFreq1 = 1;

double NHPTimer::GetSeconds( const NHPTimer::STime &a )
{
	return (static_cast<double>(a)) * fProcFreq1;
}

// Time counters

static inline void GetCounter( int64_t *pTime )
{
	__asm
	{
		rdtsc
		mov esi, pTime
		mov [esi], eax
		mov [esi+4], edx
	}
}

double NHPTimer::GetClockRate()
{
	return 1 / fProcFreq1;
}

void NHPTimer::GetTime( STime *pTime )
{
	GetCounter( pTime );
}

double NHPTimer::GetTimePassed( STime *pTime )
{
	STime old(*pTime );
	GetTime( pTime );
	return GetSeconds( *pTime - old );
}

void NHPTimer::UpdateHPTimerFrequency()
{
	static int64_t freq, start, fin;
	static double fTStart, fTFinish, fPassed;
	static STime tStart;
	static DWORD dwStart;
	static bool bUpdateInitiated = false;
	if ( bUpdateInitiated )
	{
		STime tTest( tStart );
		fPassed = GetTimePassed( &tTest );
		QueryPerformanceCounter( (_LARGE_INTEGER*) &fin );
		DWORD dwFinish = GetTickCount();
		if ( dwFinish - dwStart < 50 )
			return;
		fTStart = double( start );
		fTFinish = double( fin );
		float fTickTime = ( dwFinish - dwStart ) / 1024.0f;
		float fPCTime = (float)( ( fTFinish - fTStart ) / static_cast<double>( freq ) );
		if ( fabs( fTickTime - fPCTime ) < 0.05f )
		{
			double fProcFreq = (fPassed) * (static_cast<double>( freq )) / (fTFinish-fTStart) / fProcFreq1;
			fProcFreq1 = 1 / fProcFreq;
		}
	}
	else
	{
		QueryPerformanceFrequency( (_LARGE_INTEGER*) &freq );
	}
//	Sleep( 100 );
	bUpdateInitiated = true;
	dwStart = GetTickCount();
	GetTime( &tStart );
	QueryPerformanceCounter( (_LARGE_INTEGER*) &start );
}

static void InitHPTimer()
{
	for(;;)
	{
		UpdateHPTimerFrequency();
		if ( fProcFreq1 != 1 )
			break;
		Sleep( 100 );
	}
	//cout << "freq = " << fpProcFreq / 1000000 <<  "Mhz" << endl;
}

// это вспомогательная структура для автоматической инициализации HP timer'а
struct SHPTimerInit
{
	SHPTimerInit() { InitHPTimer(); }
};
static SHPTimerInit hptInit;


