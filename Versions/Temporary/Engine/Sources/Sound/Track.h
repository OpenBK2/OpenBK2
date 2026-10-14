#pragma once
#include "MusicSystem.h"
#include "PlayElement.h"
#include "PlayTime.h"
#include "vendor/fmod/api/inc/fmod.h"

namespace NDb
{
struct SPlayTime;
struct SMusicTrack;
} // namespace NDb

namespace NMusicSystem
{

class CTrack : public IPlayListElement
{
	OBJECT_NOCOPY_METHODS( CTrack )

	FSOUND_STREAM *pStreamingSound;

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
	int operator&( IBinSaver &f ) override;

public:
	CTrack()
		: pStreamingSound( 0 )
		, pTrackStream( 0 )
	{
	}

	CTrack( const NDb::SMusicTrack *_pTrack, const NDb::SPlayTime *_pPlayTime, EStreamType _eType )
		: playTime( _pPlayTime, _pTrack )
		, eType( _eType )
		, pTrack( _pTrack )
		, pStreamingSound( 0 )
		, eState( ETS_NOT_STARTED )
		, timePlayed( 0 )
		, timeLastCall( 0 )
		, pTrackStream( 0 )
	{
	}

	~CTrack() override;

	void NotifyTrackFinished();

	void Segment() override;
	bool IsFinished() const override;
	void Stop() override;

	const CPlayTime &GetPlayTime() const
	{
		return playTime;
	}

	bool IsTimeToEndFade( NTimer::STime timeEndFade );
	void Play() override;
	void OnResetTimer() override;
};
} // namespace NMusicSystem
