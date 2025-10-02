#pragma once

#include "DG.h"
#include "Time.hpp"

DEFINE_DG_CONSTANT_NODE( CCTime, STime );
#ifdef STUPID_VISUAL_ASSIST
class CCTime;
#endif

class CTimeCounter
{
	STime prevTime;
	ZDATA
	CObj<CCTime> pTime;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTime); return 0; }
	CTimeCounter();
	void SetCurrent( STime t );
	void ResetTiming();
	void Advance( float fMult, STime currentTime );
	CCTime* GetTime() const { return pTime; }
	int GetDeltaT( float fMult, STime currentTime );
};

