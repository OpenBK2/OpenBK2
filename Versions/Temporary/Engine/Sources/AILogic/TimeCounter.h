/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "Misc/HPTimer.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NAI
{
class CTimeCounter
{
	std::vector<double> counters;
	std::vector<NHPTimer::STime> startTimes;
	std::vector<std::string> names;

	det_map<std::string, double> szCounters;
	det_map<std::string, NHPTimer::STime> szStartTimes;

	NTimer::STime printTime;
	int nMaxIndex;

	std::vector<float> variables;
	int nMaxVar;
public:
	CTimeCounter();

	// bStart true - начать counter, false - закончить
	void Count( const int nName, const bool bStart );
	// медленный и неточный, bStart true - начать counter, false - закончить
	void Count( const std::string &szName, const bool bStart );

	void PrintCounters();

	void RegisterCounter( const int nName, const std::string &szName );

	void ChangeVar( const int nIndex, const float fChange );
	void SetVar( const int nIndex, const float fValue );
};
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

