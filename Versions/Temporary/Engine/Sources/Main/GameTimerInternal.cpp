#include "StdAfx.h"

#include "GameTimerInternal.h"
#include "Misc/StrProc.h"

namespace NGameTimer
{

CGameTimer::CGameTimer()
: nSegmentDuration( 50 ), nGameTimerSpeed( 0 ), nSegmentsCounter( 0 ), timeSegment( 0 )
{
	CreateDGTimers();
}

CGameTimer::CGameTimer( const int _nSegmentDuration )
{
	nGameTimerSpeed = 0;
	nSegmentsCounter = 0;
	timeSegment = 0;
	nSegmentDuration = _nSegmentDuration;
	//
	CreateDGTimers();
}

void CGameTimer::CreateDGTimers()
{
	if ( pAbsTimer == 0 )
		pAbsTimer = new CCSTime2();
	if ( pGameTimer == 0 )
		pGameTimer = new CCSTime2();
	if ( pAbsTimerA5 == 0 )
		pAbsTimerA5 = new CCTime();
	if ( pGameTimerA5 == 0 )
		pGameTimerA5 = new CCTime();
}

void CGameTimer::SetupDGTimers()
{
	pAbsTimer->Set( timerAbs.GetTime() );
	pAbsTimerA5->Set( timerAbs.GetTime() );
	if ( !timerGame.IsPaused() )
	{
		pGameTimer->Set( timerGame.GetTime() );
		pGameTimerA5->Set( timerGame.GetTime() );
	}
}

void CGameTimer::Update( const NTimer::STime &time )
{
	timerGame.Update( time, 500 );
	timerAbs.Update( time, 0x7fffffff );
	//
	SetupDGTimers();
}

void CGameTimer::Reset( const NTimer::STime &time )
{
	timerGame.Reset( time );
	timerAbs.Reset( time );
	timeSegment = 0;
	nSegmentsCounter = 0;
	timerGame.SetPause( false );
	pauseTypes.clear();
	//
	SetupDGTimers();
}

const NTimer::STime& CGameTimer::GetGameTime() const
{
	return timerGame.GetTime();
}

const NTimer::STime& CGameTimer::GetAbsTime() const
{
	return timerAbs.GetTime();
}

const NTimer::STime& CGameTimer::GetSegmentTime() const
{
	return timeSegment;
}

int CGameTimer::GetSegment() const
{
	return nSegmentsCounter;
}

int CGameTimer::BeginSegments()
{
	return ( timerGame.GetTime() - timeSegment ) / nSegmentDuration;
}

bool CGameTimer::CanStartNextSegment() const
{
	return ( timerGame.GetTime() >= timeSegment + nSegmentDuration );
}

bool CGameTimer::NextSegment()
{
	if ( !CanStartNextSegment() )
		return false;
	timeSegment += nSegmentDuration;
	++nSegmentsCounter;
	return true;
}

void CGameTimer::ProcessPause( const bool bPause, const int nType )
{
	if ( bPause ) 
	{
		int nPrevPauseType = -1;
		// check for existed pause or insert new one
		for ( CPauseTypes::iterator it = pauseTypes.begin(); it != pauseTypes.end(); ++it )
		{
			// check for exact match
			if ( *it == nType ) 
				return;
			// check for interval
			if ( (*it > nType) && (nPrevPauseType < nType) ) 
			{
				pauseTypes.insert( it, nType );
				return;
			}
			//
			nPrevPauseType = *it;
		}
		// add this pause at the end
		pauseTypes.push_back( nType );
	}
	else
	{
		for ( CPauseTypes::iterator it = pauseTypes.begin(); it != pauseTypes.end(); ++it )
		{
			if ( *it == nType ) 
			{
				pauseTypes.erase( it );
				return;
			}
		}
	}
}

void CGameTimer::Pause( const bool bPause, const int nType )
{
	ProcessPause( bPause, nType );
	timerGame.SetPause( !pauseTypes.empty() );
}

int CGameTimer::GetPauseType() const
{
	return pauseTypes.empty() ? -1 : pauseTypes.back();
}

bool CGameTimer::HasPause( const int nType ) const
{
	for ( CPauseTypes::const_iterator it = pauseTypes.begin(); it != pauseTypes.end(); ++it )
	{
		if ( *it == nType ) 
			return true;
	}
	return false;
}

int CGameTimer::SetSpeed( const int nSpeed )
{
//	const int nMaxSpeed = +10;//+GetGlobalVar( "maxspeed", 10 );
//	const int nMinSpeed = -10;//-GetGlobalVar( "minspeed", 10 );
	nGameTimerSpeed = Clamp( nSpeed, GetMinSpeed(), GetMaxSpeed() );
	const float fCoeff = NTimer::GetCoeffFromSpeed( nGameTimerSpeed );
	timerGame.SetScale( fCoeff );
	//
	return GetSpeed();
}

int CGameTimer::GetSpeed() const
{
	return nGameTimerSpeed;
}

int CGameTimer::GetMinSpeed() const
{
	return -10;
}

int CGameTimer::GetMaxSpeed() const
{
	return +10;
}

int CGameTimer::operator&( IBinSaver &saver )
{
	saver.Add( 1, &timerGame );
	saver.Add( 2, &timerAbs );
	saver.Add( 3, &timeSegment );
	saver.Add( 4, &nSegmentsCounter );
	saver.Add( 5, &pauseTypes );
	saver.Add( 6, &nGameTimerSpeed );
	saver.Add( 7, &nSegmentDuration );
	saver.Add( 8, &pAbsTimer );
	saver.Add( 9, &pGameTimer );
	saver.Add( 10, &pAbsTimerA5 );
	saver.Add( 11, &pGameTimerA5 );

	//CRAP{ for compatibility with legacy saves
	if ( nSegmentDuration == 0 )
		nSegmentDuration = 50;
	//
	if ( saver.IsReading() )
	{
		CreateDGTimers();
		SetupDGTimers();
	}

	return 0;
}

static void SetGameSpeed( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	const string szVal = NStr::ToMBCS( paramsSet[0] );
	const int nVal = NStr::ToInt( szVal );
	Singleton<IGameTimer>()->SetSpeed( nVal );
}

static void PauseGame( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	int nType = PAUSE_TYPE_USER_PAUSE;
	if ( !paramsSet.empty() )
	{
		const string szVal = NStr::ToMBCS( paramsSet[0] );
		nType = NStr::ToInt( szVal );
	}
	Singleton<IGameTimer>()->Pause( true, nType );
}

}

IGameTimer *CreateGameTimer( const int nSegmentDuration )
{
	return new NGameTimer::CGameTimer( nSegmentDuration );
}

START_REGISTER(GameTimerCommands)
REGISTER_CMD( "game_speed", NGameTimer::SetGameSpeed )
REGISTER_CMD( "pause_game", NGameTimer::PauseGame );
FINISH_REGISTER

using namespace NGameTimer;
REGISTER_SAVELOAD_CLASS( 0x10075C05, CGameTimer );
REGISTER_SAVELOAD_CLASS( 0x751B4B00, CCSTime2 );

