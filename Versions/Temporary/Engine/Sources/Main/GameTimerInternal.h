#pragma once
#include "..\System\DG.h"
#include "..\System\Time.h"

namespace NGameTimer
{

DEFINE_DG_CONSTANT_NODE( CCSTime2, NTimer::STime );

class CGameTimer : public IGameTimer
{
	OBJECT_BASIC_METHODS( CGameTimer );
	//
	typedef list<int> CPauseTypes;
	//
	CScaleTimer timerGame;								// game timer
	CScaleTimer timerAbs;									// absolute timer
	NTimer::STime timeSegment;						// last segment independent time
	int nSegmentsCounter;									// segments counter
	CPauseTypes pauseTypes;								// pause types
	int nGameTimerSpeed;									// game timer game-friendly speed [-10..+10]
	int nSegmentDuration;
	CObj<CCSTime2> pAbsTimer;
	CObj<CCSTime2> pGameTimer;
	CObj<CCTime> pAbsTimerA5;
	CObj<CCTime> pGameTimerA5;
	//
	void ProcessPause( const bool bPause, const int nType );
	void SetupDGTimers();
	void CreateDGTimers();
	//
public:
	CGameTimer();
	CGameTimer( const int nSegmentDuration );
	// update/reset timer
	void Update( const NTimer::STime &time );
	void Reset( const NTimer::STime &time );
	// get timer DG nodes
	CFuncBase<NTimer::STime> *GetGameTimer() const { return pGameTimer; }
	CFuncBase<NTimer::STime> *GetAbsTimer() const { return pAbsTimer; }
	CFuncBase<STime> *GetGameTimerA5() const { return pGameTimerA5; }
	CFuncBase<STime> *GetAbsTimerA5() const { return pAbsTimerA5; }
	// get current independent game time
	const NTimer::STime& GetGameTime() const;
	// get current independent absolute time
	const NTimer::STime& GetAbsTime() const;
	// get current independent segment time
	const NTimer::STime& GetSegmentTime() const;
	virtual const float GetSegmentDuration() const { return nSegmentDuration; }
	// get segment number
	int GetSegment() const;
	// begin segments block. returns number of segments to process
	int BeginSegments();
	
	bool CanStartNextSegment() const;
	// increment for next segment. return false if can't increment for next segment
	bool NextSegment();
	// pause
	void Pause( const bool bPause, const int nType = 0 );
	int GetPauseType() const;
	bool HasPause( const int nType ) const;
	// time speed increase/decrease
	int SetSpeed( const int nSpeed );
	int GetSpeed() const;
	int GetMinSpeed() const;
	int GetMaxSpeed() const;
	//
	int operator&( IBinSaver &saver );
};

}

