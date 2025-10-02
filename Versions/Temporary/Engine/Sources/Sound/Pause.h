#pragma once
#include "PlayElement.h"

namespace NDb
{
	struct SPlayPause;
}

namespace NMusicSystem
{

class CPause : public IPlayListElement
{
	OBJECT_BASIC_METHODS(CPause)
	enum EPauseState
	{
		EPS_NOT_STARTED,
		EPS_ACTIVE,
		EPS_FINISHED,
	};
	ZDATA
	NTimer::STime timeToPause;
	NTimer::STime timeLastCall;
	NTimer::STime timePaused;
	EPauseState eState;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&timeToPause); f.Add(3,&timeLastCall); f.Add(4,&timePaused); f.Add(5,&eState); return 0; }
public:
	CPause() { }
	CPause( const NDb::SPlayPause &_Pause );

	void Segment();
	bool IsFinished() const ;
	void Stop() {}

	void Play();
	void OnResetTimer();

};
}

