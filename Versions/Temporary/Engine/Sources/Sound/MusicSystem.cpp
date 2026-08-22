#include "stdafx.h"
#include "MusicSystem.hpp"
#include "DBMusicSystem.h"

#include "PlayList.h"
#include "Track.h"
#include "System/Commands.h"
#include "System/VFSOperations.h"

float s_fMusicVolume;

IMusicSystem * CreateMusicSystem()
{
	return new NMusicSystem::CMusicSystem;
}

namespace NMusicSystem
{
CMusicSystem * GetMusicSystem()
{
	return checked_cast<CMusicSystem*>( Singleton<IMusicSystem>() );
}

NTimer::STime GetAbsTime()
{
	return Singleton<IGameTimer>()->GetAbsTime();
}

FSOUND_STREAM * OpenTrack( CDataStream *pTrack )
{
	if ( pTrack == 0 || !pTrack->IsOk() )
		return 0;

	FSOUND_STREAM *pStreamingSound = FSOUND_Stream_Open( (const char*)(pTrack->GetBuffer()), FSOUND_2D | FSOUND_LOOP_OFF | FSOUND_LOADMEMORY, 0, pTrack->GetSize() );
	return pStreamingSound;
}

bool CMusicSystem::IsPaused( EStreamType eType ) const
{
	if ( eType == EST_MUSIC )
		return pauses[EMS_MASTER] +
		pauses[EMS_MUSIC_MASTER] +
		pauses[EMS_MUSIC_FADE_SELF] +
		pauses[EMS_MUSIC_FADE_FROM_VOICE];
	else
		return pauses[EMS_MASTER] +
		pauses[EMS_VOICE_MASTER] +
		pauses[EMS_VOICE_SELF];
}

void CMusicSystem::OnResetTimer()
{
	for ( int i = 0; i < playlists.size(); ++i )
		playlists[i]->OnResetTimer();
}

void CMusicSystem::Update()
{
	SetVolume( EMS_MASTER, s_fMusicVolume );

	UpdatePlayListChange();
	for ( int i = 0; i < playlists.size(); ++i )
		playlists[i]->Segment();

	if ( pVoiceTrack )
	{
		if ( pVoiceTrack->IsFinished() )
			pVoiceTrack = 0;
		else
			pVoiceTrack->Segment();
	}

	fades.Update();

	const float fMusicVolume = volumes[EMS_MASTER] * 
		volumes[EMS_MUSIC_MASTER] *
		volumes[EMS_MUSIC_FADE_SELF] *
		volumes[EMS_MUSIC_FADE_FROM_VOICE] ;
	const float fVoiceVolume = volumes[EMS_MASTER] *
		volumes[EMS_VOICE_MASTER] *
		volumes[EMS_VOICE_SELF] ;
	
	if ( channels[EST_MUSIC] > 0 )
	{
		if ( int(fMusicVolume * 255) != FSOUND_GetVolume( channels[EST_MUSIC] ) )
			FSOUND_SetVolume( channels[EST_MUSIC], fMusicVolume * 255 );
		if ( bool(FSOUND_GetPaused( channels[EST_MUSIC] )) != IsPaused( EST_MUSIC ) )
			FSOUND_SetPaused( channels[EST_MUSIC], IsPaused( EST_MUSIC ) );
	}

	if ( channels[EST_VOICE] > 0 )
	{
		if ( int(fVoiceVolume * 255) != FSOUND_GetVolume( channels[EST_VOICE] ) )
			FSOUND_SetVolume( channels[EST_VOICE], fVoiceVolume * 255 );
		if ( bool(FSOUND_GetPaused( channels[EST_VOICE] )) != IsPaused( EST_VOICE ) )
			FSOUND_SetPaused( channels[EST_VOICE], IsPaused( EST_VOICE ) );
	}
}

void CMusicSystem::SetVolume( EMusicSystemVolume eType, float fVolume )
{
	volumes[eType] = fVolume;
}

float CMusicSystem::GetVolume( EMusicSystemVolume eType ) const
{
	return volumes[eType];
}

void CMusicSystem::PauseMusic( EMusicSystemVolume eType, bool bPause )
{
	pauses[eType] = bPause;
}

void CMusicSystem::PlayVoice( const NDb::SVoice *pVoice ) 
{
	if ( pVoiceTrack )
	{
		pVoiceTrack->Stop();
		pVoiceTrack = 0;
		fades.clear();
	}

	const NTimer::STime curTime = GetAbsTime();
	pVoiceTrack = new CTrack( pVoice->pTrack, pVoice->pPlayTime, EST_VOICE );

	if ( pVoice->pMusicStreamFadeOut )
		fades.push_back( new CFade( pVoice->pMusicStreamFadeOut, EMS_MUSIC_FADE_FROM_VOICE, 0 ) );
	if ( pVoice->pMusicStreamFadeIn )
		fades.push_back( new CFade( pVoice->pMusicStreamFadeIn, EMS_MUSIC_FADE_FROM_VOICE, pVoiceTrack ) );
	if ( pVoice->pFadeIn )
		fades.push_back( new CFade( pVoice->pFadeIn, EMS_VOICE_SELF, 0 ) );
	if ( pVoice->pFadeOut )
		fades.push_back( new CFade( pVoice->pFadeOut, EMS_VOICE_SELF, pVoiceTrack ) );

	fades.Update();

	pVoiceTrack->Segment();
}

void CMusicSystem::Init( const NDb::SMapMusic *pMapMusic, int nActivePlayList ) 
{
	// just don't do anything if already all is ok
	if ( pMapMusic == pCurrentMapMusic && nActivePlayList == nPlayList )
		return;

	pCurrentMapMusic = pMapMusic;
	if ( pMapMusic )
	{
		playlists.resize( pMapMusic->playLists.size() );
		for ( int i = 0; i < playlists.size(); ++i )
			playlists[i] = new CPlayList( pMapMusic->playLists[i] );
		nPlayList = 0;
		nDesiredPlayList = 0;
		ChangePlayList( nActivePlayList );
		eState = EPST_JUST_STARTED;
	}
	else
	{
		InitDefault();
		eState = EPST_NOT_INITTED;
	}
}

void CMusicSystem::InitDefault()
{
	nDesiredPlayList = nPlayList = 0;
	eState = EPST_NOT_INITTED;

	volumes.clear();
	volumes.resize( _EMS_COUNT, 1.0f );
	pauses.clear();
	pauses.resize( _EMS_COUNT, 0 );
	channels.clear();
	channels.resize( 2, 0 );
	pCurrentMapMusic = 0;
	playlists.clear();
	pVoiceTrack = 0;

	const float fMusic = NGlobal::GetVar( "stream_music_volume", 1.0f );
	const float fVoice = NGlobal::GetVar( "stream_voice_volume", 1.0f );
	SetVolume( EMS_MASTER, s_fMusicVolume );
	SetVolume( EMS_MUSIC_MASTER, fMusic );
	SetVolume( EMS_VOICE_MASTER, fVoice );
}

void CMusicSystem::Clear()
{
	playlists.clear();
	pVoiceTrack = 0;
	fades.clear();
	InitDefault();
}

bool CMusicSystem::CanChangePlayList() const
{
	return eState != EPST_FADING_NEW_IN;
}

int CMusicSystem::GetPlayList() const
{
	return nPlayList;
}

void CMusicSystem::ChangePlayList( int _nPlayList ) 
{
	if ( _nPlayList >= playlists.size() || _nPlayList < 0 )
		return;

	if ( CanChangePlayList() )
		nDesiredPlayList = _nPlayList;
}

void CMusicSystem::UpdatePlayListChange()
{
	switch( eState )
	{
	case EPST_NOT_INITTED:

		break;
	case EPST_JUST_STARTED:
		playlists[nDesiredPlayList]->FadeIn();
		eState = EPST_FADING_NEW_IN;

		break;
	case EPST_PLAYING_CURRENT:
		if ( nDesiredPlayList != nPlayList )
		{
			playlists[nPlayList]->FadeOut();
			eState = EPST_FADING_OLD_OUT;
		}

		break;
	case EPST_FADING_OLD_OUT:
		if ( playlists[nPlayList]->IsOut() )
		{
			playlists[nDesiredPlayList]->FadeIn();
			eState = EPST_FADING_NEW_IN;
		}

		break;
	case EPST_FADING_NEW_IN:
		if ( playlists[nDesiredPlayList]->IsIn() )
		{
			nPlayList = nDesiredPlayList;
			eState = EPST_PLAYING_CURRENT;
		}

		break;
	}
}

int CMusicSystem::operator&( IBinSaver &f ) 
{ 
	f.Add(2,&nPlayList); 
	f.Add(3,&playlists); 
	f.Add(4,&pVoiceTrack); 
	f.Add(5,&volumes); 
	f.Add(6,&pauses); 
	f.Add(7,&channels); 
	f.Add(8,&fades); 
	f.Add( 9, &nDesiredPlayList );
	f.Add( 10, &eState );
	f.Add( 11, &pCurrentMapMusic );
	if ( f.IsReading() )
	{
		channels.clear();
		channels.resize( 2, 0 );
	}
	return 0; 
}
}

REGISTER_SAVELOAD_CLASS_NM( SOUND, 0x11181340, CMusicSystem, NMusicSystem  );

START_REGISTER(MusicSystem)
REGISTER_VAR_EX( "Sound.MusicVolume", NGlobal::VarFloatHandler, &s_fMusicVolume, 0.99f, STORAGE_USER );
FINISH_REGISTER


