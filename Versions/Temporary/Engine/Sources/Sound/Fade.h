#pragma once
#include "PlayElement.h"
#include "MusicSystem.h"

namespace NDb
{
	struct SFade;
}

namespace NMusicSystem
{
class CTrack;

class CFade : public IPlayListElement
{
	enum EFadeState
	{
		EFS_NOT_STARTED,
		EFS_STARTED,
		EFS_FINISHED,
	};

	OBJECT_BASIC_METHODS( CFade )
	ZDATA
	CDBPtr<NDb::SFade> pFade;
	EMusicSystemVolume eVolumeType;
	float fStartVolume;
	EFadeState eState;
	CPtr<CTrack> pTrackToEnd;

	NTimer::STime timeFaded;
	NTimer::STime timeLastCall;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFade); f.Add(3,&eVolumeType); f.Add(4,&fStartVolume); f.Add(5,&eState); f.Add(6,&pTrackToEnd); f.Add(7,&timeFaded); f.Add(8,&timeLastCall); return 0; }

	void SetCurrentVolume();
public:
	CFade() {  }
	// if fade have to be at the end of track, then track is given
	CFade( const NDb::SFade *_pFade, EMusicSystemVolume _eVolumeType, class CTrack *_pTrackToEnd );
	void Segment();
	bool IsFinished() const;
	void Stop();
	void Play();
	void OnResetTimer();
};

class CFades : public std::list<CPtr<CFade> >
{
	ZDATA_( list<CPtr<CFade> > )
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( std::list<CPtr<CFade> > *)this); return 0; }
public:
	void Update();
};

}

