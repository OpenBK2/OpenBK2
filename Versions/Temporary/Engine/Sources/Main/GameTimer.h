#pragma once
#include "../System/Time.hpp"
template<class T> class CFuncBase;


namespace NTimer
{
	inline float GetCoeffFromSpeed( const int nSpeed )
	{
		return nSpeed >= 0 ? nSpeed + 1 : 1.0f / fabsf( float(nSpeed - 1) );
	}
};

class CScaleTimer
{
	NTimer::STime prevTime;						// current dependent time
	NTimer::STime currTime;						// current independent time
	float fScale;											// time scaling
	float fError;											// time rounding error (for scaling)
	bool bPaused;											// is this timer paused
public:
	CScaleTimer() : prevTime( 0 ), currTime( 0 ), fScale( 1 ), fError( 0 ), bPaused( false ) {  }
	// scale
	void SetScale( const float _fScale ) { fScale = _fScale; }
	float GetScale() const { return fScale; }
	// pause
	void SetPause( bool _bPaused ) { bPaused = _bPaused; }
	bool IsPaused() const { return bPaused; }
	// get current time value
	const NTimer::STime& GetTime() const { return currTime; }
	// reset timer to this time
	void Reset( const NTimer::STime &time ) { prevTime = time; currTime = 0; fError = 0; }
	// update timer with new time
	void Update( const NTimer::STime &time, const NTimer::STime &timeMaxDelta )
	{
		NTimer::STime dT = prevTime == 0 ? 0 : time - prevTime;
		if ( dT > timeMaxDelta ) 
			dT = timeMaxDelta;
		prevTime = time;
		if ( !bPaused ) 
		{
			const float fdt = float( dT*fScale ) + fError;
			dT = NTimer::STime( Float2Int( fdt ) );
			fError = fdt - float( dT );
			currTime += dT;
		}
	}
	// serialize
	int operator&( IBinSaver &saver )
	{
//		saver.Add( 1, &prevTime );
		saver.Add( 2, &currTime );
		saver.Add( 3, &fScale );
		saver.Add( 4, &fError );
		saver.Add( 5, &bPaused );
		if ( saver.IsReading() ) 
			prevTime = 0;
		return 0;
	}
};

struct IGameTimer : public CObjectBase
{
	// type ID
	enum { tidTypeID = 0x10075C05 };
	// update/reset timer
	virtual void Update( const NTimer::STime &time ) = 0;
	virtual void Reset( const NTimer::STime &time ) = 0;
	// get timer DG nodes
	virtual CFuncBase<NTimer::STime> *GetGameTimer() const = 0;
	virtual CFuncBase<NTimer::STime> *GetAbsTimer() const = 0;
	virtual CFuncBase<STime> *GetGameTimerA5() const = 0;
	virtual CFuncBase<STime> *GetAbsTimerA5() const = 0;
	//
	// get current independent game time
	virtual const NTimer::STime& GetGameTime() const = 0;
	// get current independent absolute time
	virtual const NTimer::STime& GetAbsTime() const = 0;
	// get current independent segment time
	virtual const NTimer::STime& GetSegmentTime() const = 0;
	virtual const float GetSegmentDuration() const = 0;

	virtual bool CanStartNextSegment() const = 0;
	// get segment number
	virtual int GetSegment() const = 0;
	// begin segments block. returns number of segments to process
	virtual int BeginSegments() = 0;
	// increment for next segment. return false if can't increment for next segment
	virtual bool NextSegment() = 0;
	// pause
	virtual void Pause( const bool bPause, const int nType = 0 ) = 0;
	virtual int GetPauseType() const = 0;
	virtual bool HasPause( const int nType ) const = 0;
	// time speed increase/decrease
	virtual int SetSpeed( const int nSpeed ) = 0;
	virtual int GetSpeed() const = 0;
	virtual int GetMinSpeed() const = 0;
	virtual int GetMaxSpeed() const = 0;
};
inline IGameTimer* GameTimer() { return Singleton<IGameTimer>(); }
IGameTimer *CreateGameTimer( const int nSegmentDuration );

enum EPauseType
{
	PAUSE_TYPE_USER_PAUSE = 0,
	PAUSE_TYPE_INACTIVE		= 1,
	PAUSE_TYPE_INTERFACE	= 2, // pause specified by interface
	
	PAUSE_TYPE_WAIT_FOR_CONNECT = 3,
	PAUSE_TYPE_MP_NO_SEGMENT_DATA = 100,	// no segment data in multiplayer mode
};


