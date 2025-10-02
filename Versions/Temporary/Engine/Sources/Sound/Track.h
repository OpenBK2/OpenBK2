#pragma once
#include "PlayElement.h"

#include "../vendor/fmod/api/inc/fmod.h"
#include "PlayTime.h"
#include "MusicSystem.h"

namespace NDb
{
	struct SPlayTime;
	struct SMusicTrack;
}

namespace NMusicSystem
{

class CTrack : public IPlayListElement
{
	OBJECT_NOCOPY_METHODS( CTrack )

	FSOUND_STREAM * pStreamingSound;
	enum ETrackState
	{
		ETS_NOT_STARTED,
		ETS_PLAYING,
		ETS_FINISHED,
	};

	EStreamType eType;
	NTimer::STime timePlayed;
	NTimer::STime timeLastCall;
	CPlayTime playTime;
	CDBPtr<NDb::SMusicTrack> pTrack;
	ETrackState eState;
	CDataStream *pTrackStream;

	void PlayTrack( int nTrackTime );
	void OpenTrack();
public:
	int operator&( IBinSaver &f );
	
public:
	CTrack(): pStreamingSound(0), pTrackStream( 0 ) { }
	CTrack( const NDb::SMusicTrack *_pTrack, const NDb::SPlayTime *_pPlayTime, EStreamType _eType )
		: playTime( _pPlayTime, _pTrack ), eType( _eType ), pTrack( _pTrack ), pStreamingSound( 0 ), eState( ETS_NOT_STARTED ),
		timePlayed( 0 ), timeLastCall( 0 ), pTrackStream( 0 )
	{
	}
	~CTrack();

	void NotifyTrackFinished();

	void Segment();
	bool IsFinished() const;
	void Stop();

	const CPlayTime &GetPlayTime() const { return playTime; }
	bool IsTimeToEndFade( NTimer::STime timeEndFade );
	void Play();
	void OnResetTimer();
};
}
