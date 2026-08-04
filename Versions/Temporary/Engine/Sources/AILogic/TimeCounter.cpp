#include "stdafx.h"

#include "TimeCounter.h"

NAI::CTimeCounter timeCounter;
extern NTimer::STime curTime;

namespace NAI
{
CTimeCounter::CTimeCounter()
 : printTime( 0 ), counters( 100 ), startTimes( 100 ), names( 100 ), variables( 100 ), 
	 nMaxIndex( -1 ), nMaxVar( -1 )
{
	memset( &counters[0], 0, sizeof(counters[0] * counters.size()) );
	memset( &startTimes[0], 0, sizeof(startTimes[0] * startTimes.size()) );
	RegisterCounter( 0, "GroupLogic" );
	RegisterCounter( 1, "Warfog" );
	RegisterCounter( 2, "KilledUnits" );
	RegisterCounter( 3, "Scripts" );
	RegisterCounter( 4, "HitsStore" );
	RegisterCounter( 5, "LinkObject" );
	RegisterCounter( 6, "StatObjects" );
	RegisterCounter( 7, "AAManager" );
	RegisterCounter( 8, "SoonBeDead" );
	RegisterCounter( 9, "UnitCreation" );
	RegisterCounter( 10, "SuspendedUpdates" );
	RegisterCounter( 11, "CombatEstimator" );
	RegisterCounter( 13, "LogicStates" );
	RegisterCounter( 14, "ShellsStore" );
	RegisterCounter( 15, "Follow" );
	RegisterCounter( 16, "AI" );
	RegisterCounter( 17, "FirstPathSegment" );
	RegisterCounter( 18, "StayTime" );
	RegisterCounter( 19, "HandoutCollisions" );
	RegisterCounter( 20, "SecondPathSegment" );
	RegisterCounter( 21, "LookForTarget" );
	RegisterCounter( 22, "Artillery" );
	RegisterCounter( 23, "Aviation" );
	RegisterCounter( 24, "Soldier" );
	RegisterCounter( 25, "MilitaryCar" );
	RegisterCounter( 26, "QueueUnitSegment" );
	RegisterCounter( 27, "AIUnitSegment" );
	RegisterCounter( 28, "SegmentsOperations " );
	RegisterCounter( 29, "CommonUnitSegment" );
	RegisterCounter( 30, "Clear" );
	RegisterCounter( 31, "Guns" );

	RegisterCounter( 31, "1" );
	RegisterCounter( 32, "2" );
	RegisterCounter( 33, "3" );
	RegisterCounter( 34, "4" );
	RegisterCounter( 35, "5" );
	RegisterCounter( 36, "6" );
	RegisterCounter( 37, "7" );
	RegisterCounter( 38, "8" );
	
	RegisterCounter( 39, "SoldierShootEstimator" );
	RegisterCounter( 40, "TankShootEstimator" );
	RegisterCounter( 41, "GunCrewState" );
	RegisterCounter( 42, "GunsState" );
	RegisterCounter( 43, "SoldierAttackState" );
	RegisterCounter( 44, "MechAttackState" );
	RegisterCounter( 45, "SwarmState" );
	RegisterCounter( 46, "RestState" );
	
	RegisterCounter( 47, "CollisionSegment" );
	RegisterCounter( 48, "CollisionChange" );
	RegisterCounter( 49, "CollisionFindCandidates" );
	RegisterCounter( 50, "CollisionSuspendedPoint" );
	RegisterCounter( 51, "FirstSegmentActions" );
	RegisterCounter( 52, "FirstSegmentClear" );
	RegisterCounter( 53, "FirstSegmentCheck" );
	
	RegisterCounter( 54, "Iterating" );
	RegisterCounter( 55, "Storages" );
	
	RegisterCounter( 56, "InitRest" );

	RegisterCounter( 57, "1" );
	RegisterCounter( 58, "2" );
	RegisterCounter( 59, "3" );
	RegisterCounter( 60, "4" );
	RegisterCounter( 61, "5" );
	RegisterCounter( 62, "6" );
	

	RegisterCounter( 63, "general" );
	RegisterCounter( 64, "general_removedead" );
	RegisterCounter( 65, "general_removeenemies" );
	RegisterCounter( 66, "general_commander" );
	RegisterCounter( 67, "general_artillery" );
	RegisterCounter( 68, "general_intendant" );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CTimeCounter::RegisterCounter( const int nName, const std::string &szName )
{
	names[nName] = szName;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CTimeCounter::Count( const int nName, const bool bStart )
{
	nMaxIndex = (std::max)( nName, nMaxIndex );

	if ( bStart )
	{
		NHPTimer::GetTime( &(startTimes[nName]) );
	}
	else
	{
		const double time = NHPTimer::GetTimePassed( &(startTimes[nName]) );
		counters[nName] += time;

		if ( curTime > printTime )
		{
			printTime = curTime + 20000;
			PrintCounters();
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CTimeCounter::Count( const std::string &szName, const bool bStart )
{
	if ( bStart )
		NHPTimer::GetTime( &(szStartTimes[szName]) );
	else
	{
		const double time = NHPTimer::GetTimePassed( &(szStartTimes[szName]) );

		if ( szCounters.find( szName ) == szCounters.end() )
			szCounters[szName] = 0;

		szCounters[szName] += time;

		if ( curTime > printTime )
		{
			printTime = curTime + 20000;
			PrintCounters();
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CTimeCounter::PrintCounters()
{
	IConsoleBuffer *pConsoleBuffer = Singleton<IConsoleBuffer>();
	pConsoleBuffer->WriteASCII( 11, "Time counters", 0, true );

	for ( int i = 0; i <= nMaxIndex; ++i )
	{
		const int name = i;
		const double fTime = counters[i];
		if ( fTime > 0.0f )
			pConsoleBuffer->WriteASCII( 11, StrFmt( "%s = %g", names[name].c_str(), fTime ), 0, true );
	}
	
	for ( det_map<std::string, double>::const_iterator iter = szCounters.begin(); iter != szCounters.end(); ++iter )
	{
		const double fTime = iter->second;
		if ( fTime > 0.0f )
			pConsoleBuffer->WriteASCII( 11, StrFmt( "%s = %g", (iter->first).c_str(), fTime ), 0, true );
	}

	pConsoleBuffer->WriteASCII( 11, "", 0, true );
	pConsoleBuffer->WriteASCII( 11, "Variables", 0, true );
	for ( int i = 0; i <= nMaxVar; ++i )
	{
		if ( variables[i] != 0 )
			pConsoleBuffer->WriteASCII( 11, StrFmt( "variable %d = %g", i, variables[i] ), 0, true );
	}

	pConsoleBuffer->WriteASCII( 11, "=================================================", 0, true );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CTimeCounter::ChangeVar( const int nIndex, const float fChange )
{
	if ( nIndex >= variables.size() )
		variables.resize( nIndex * 1.5, 0.0f );

	nMaxVar = (std::max)( nIndex, nMaxVar );
	variables[nIndex] += fChange;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CTimeCounter::SetVar( const int nIndex, const float fValue )
{
	if ( nIndex >= variables.size() )
		variables.resize( nIndex * 1.5, 0.0f );

	nMaxVar = (std::max)( nIndex, nMaxVar );
	variables[nIndex] = fValue;
}

}

