#include "StdAfx.h"
#include "DBMusicSystem.h"
#include "MusicSystem.hpp"
#include "Track.h"
#include "System/VFSOperations.h"
#include "vendor/fmod/api/inc/fmod.h"

namespace NMusicSystem
{
signed char F_CALLBACKAPI TrackFinishedCallBack( FSOUND_STREAM *stream, void *buff, int len, void *userdata )
{
	CTrack *pSFX = reinterpret_cast<CTrack*>( userdata );
	pSFX->NotifyTrackFinished();
	return true;
}


// CTrack


void CTrack::NotifyTrackFinished()
{
	PlayTrack( 0 );
}

void CTrack::PlayTrack( int nTrackTime )
{
	const int nChannel = FSOUND_Stream_PlayEx( FSOUND_FREE, pStreamingSound, 0, true );
	GetMusicSystem()->SetChannel( nChannel, eType );
	FSOUND_Stream_SetEndCallback( pStreamingSound, TrackFinishedCallBack, this );
	FSOUND_Stream_SetTime( pStreamingSound, nTrackTime );
	FSOUND_SetPaused( nChannel, false );
}

void CTrack::OpenTrack()
{
	if ( pTrackStream )
		delete pTrackStream;

	pTrackStream = new CFileStream( NVFS::GetMainVFS(), pTrack->szMusicFileName );
	pStreamingSound = NMusicSystem::OpenTrack( pTrackStream );
}

void CTrack::Segment()
{
	switch( eState )
	{
	case ETS_NOT_STARTED:
		OpenTrack();
		if ( pStreamingSound )
			PlayTrack( 0 );
		timeLastCall = GetAbsTime();
		eState = pStreamingSound ? ETS_PLAYING : ETS_FINISHED;

		break;
	case ETS_PLAYING:
		{
			const NTimer::STime curTime = GetAbsTime();
			timePlayed += curTime - timeLastCall;
			timeLastCall = curTime;
		}
		if ( timePlayed >= playTime.GetPlayTime() )
		{
			eState = ETS_FINISHED;
			FSOUND_Stream_SetEndCallback( pStreamingSound, 0, this );
			FSOUND_Stream_Stop( pStreamingSound );
			GetMusicSystem()->SetChannel( 0, eType );
		}
		else
		{
			// init after load
			if ( !GetMusicSystem()->GetChannel( eType ) )
			{
				int nTime = FSOUND_Stream_GetLengthMs( pStreamingSound );
				int nTrackTime = timePlayed % nTime;
				PlayTrack( nTrackTime );
			}
		}

		break;
	case ETS_FINISHED:
		break;
	}
}

void CTrack::Play() 
{
	timeLastCall = GetAbsTime();
}

void CTrack::OnResetTimer() 
{
	timeLastCall = GetAbsTime();
}


bool CTrack::IsFinished() const
{
	return eState == ETS_FINISHED || timePlayed >= playTime.GetPlayTime();
}

void CTrack::Stop()
{
	FSOUND_Stream_SetEndCallback( pStreamingSound, 0, this );
	FSOUND_Stream_Stop( pStreamingSound );
	GetMusicSystem()->SetChannel( 0, eType );
}

CTrack::~CTrack()
{
	if ( pStreamingSound )
	{
		FSOUND_Stream_SetEndCallback( pStreamingSound, 0, this );
		FSOUND_Stream_Stop( pStreamingSound );
	}
	pStreamingSound = 0;
	delete pTrackStream;
}

bool CTrack::IsTimeToEndFade( NTimer::STime timeEndFade )
{
	return timePlayed + timeEndFade > playTime.GetPlayTime();
}

int CTrack::operator&( IBinSaver &f ) 
{ 
	f.Add(2,&eType); 
	f.Add(3,&timePlayed); 
	f.Add(4,&timeLastCall); 
	f.Add(5,&playTime); 
	f.Add( 6, &eState );
	f.Add( 7, &pTrack );
	if ( f.IsReading() )
		OpenTrack();

	return 0; 
}
}
REGISTER_SAVELOAD_CLASS_NM( 0x111813C3, CTrack, NMusicSystem )

