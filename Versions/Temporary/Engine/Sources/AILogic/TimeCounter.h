#ifndef __TIME_COUNTER_H__
#define __TIME_COUNTER_H__
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma ONCE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "..\Misc\HPTimer.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NAI
{
class CTimeCounter
{
	vector<double> counters;
	vector<NHPTimer::STime> startTimes;
	vector<string> names;

	hash_map<string, double> szCounters;
	hash_map<string, NHPTimer::STime> szStartTimes;

	NTimer::STime printTime;
	int nMaxIndex;

	vector<float> variables;
	int nMaxVar;
public:
	CTimeCounter();

	// bStart true - начать counter, false - закончить
	void Count( const int nName, const bool bStart );
	// медленный и неточный, bStart true - начать counter, false - закончить
	void Count( const string &szName, const bool bStart );

	void PrintCounters();

	void RegisterCounter( const int nName, const string &szName );

	void ChangeVar( const int nIndex, const float fChange );
	void SetVar( const int nIndex, const float fValue );
};
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __TIME_COUNTER_H__

