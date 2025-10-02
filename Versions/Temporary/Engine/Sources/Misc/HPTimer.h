#ifndef __HPTIMER_H_
#define __HPTIMER_H_
#include "Misc_export.h"

namespace NHPTimer
{
	typedef int64 STime;
	double GetSeconds( const STime &a );
	// получить текущее время
	MISC_EXPORT void GetTime( STime *pTime );
	// получить время, прошедшее с момента, записанного в *pTime, при этом в *pTime будет записано текущее время
	MISC_EXPORT double GetTimePassed( STime *pTime );
	// получить частоту процессора
	MISC_EXPORT double GetClockRate();
	// recalc CPU frequency, call regularly to support SpeedStep processors
	void UpdateHPTimerFrequency();
};

#endif

