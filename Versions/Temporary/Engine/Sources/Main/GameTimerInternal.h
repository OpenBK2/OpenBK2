#pragma once
#include "System/Dg.h"
#include "System/Time.h"

namespace NGameTimer
{

DEFINE_DG_CONSTANT_NODE( CCSTime2, NTimer::STime );

class CGameTimer : public IGameTimer
{
	OBJECT_BASIC_METHODS( CGameTimer );
	//
	typedef std::list<int> CPauseTypes;
	//
	CScaleTimer timerGame;     // game timer
	CScaleTimer timerAbs;      // absolute timer
	NTimer::STime timeSegment; // last segment independent time
	int nSegmentsCounter;      // segments counter
	CPauseTypes pauseTypes;    // pause types
	int nGameTimerSpeed;       // game timer game-friendly speed [-10..+10]
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
	void Update( const NTimer::STime &time ) override;
	void Reset( const NTimer::STime &time ) override;

	// get timer DG nodes
	CFuncBase<NTimer::STime> *GetGameTimer() const override
	{
		return pGameTimer;
	}

	CFuncBase<NTimer::STime> *GetAbsTimer() const override
	{
		return pAbsTimer;
	}

	CFuncBase<STime> *GetGameTimerA5() const override
	{
		return pGameTimerA5;
	}

	CFuncBase<STime> *GetAbsTimerA5() const override
	{
		return pAbsTimerA5;
	}

	// get current independent game time
	const NTimer::STime &GetGameTime() const override;
	// get current independent absolute time
	const NTimer::STime &GetAbsTime() const override;
	// get current independent segment time
	const NTimer::STime &GetSegmentTime() const override;

	const float GetSegmentDuration() const override
	{
		return nSegmentDuration;
	}

	// get segment number
	int GetSegment() const override;
	// begin segments block. returns number of segments to process
	int BeginSegments() override;

	bool CanStartNextSegment() const override;
	// increment for next segment. return false if can't increment for next segment
	bool NextSegment() override;
	// pause
	void Pause( const bool bPause, const int nType = 0 ) override;
	int GetPauseType() const override;
	bool HasPause( const int nType ) const override;
	// time speed increase/decrease
	int SetSpeed( const int nSpeed ) override;
	int GetSpeed() const override;
	int GetMinSpeed() const override;
	int GetMaxSpeed() const override;
	//
	int operator&( IBinSaver &saver ) override;
};

} // namespace NGameTimer
