#pragma once
#include "System/time.hpp"

namespace NGScene
{

struct SExpFader
{
private:
	ZDATA
	STime tPrev;
	float fSize;
	float fOnTime, fOffTime;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&tPrev); f.Add(3,&fSize); f.Add(4,&fOnTime); f.Add(5,&fOffTime); return 0; }
	//
	SExpFader( float _fStartSize = 0 ) : tPrev(0), fOnTime(1), fOffTime(1), fSize(_fStartSize) {}
	//
	SExpFader( float _fOnTime, float _fOffTime, float _fStartSize = 0 )
		: tPrev(0), fOnTime(_fOnTime), fOffTime(_fOffTime), fSize(_fStartSize)
	{
		fOnTime = (std::max)( 1e-3f, fOnTime );
		fOffTime = (std::max)( 1e-3f, fOffTime );
	}
	void Update( STime tCur, float fTargetSize )
	{
		float fDeltaT = (std::max)( 0.0f, ((float)tCur - tPrev)/1024.0f );
		fSize = fTargetSize - ( fTargetSize - fSize ) * exp( -fDeltaT / fOnTime );
		tPrev = tCur;
	}
	void SetPrevTime( STime t ) { tPrev = t; }
	float GetSize() const { return fSize; }
};

inline float GefFadeLatency( float fLatency )
{
	return -fLatency / log( 0.01f );
}

} // namespace NGScene


