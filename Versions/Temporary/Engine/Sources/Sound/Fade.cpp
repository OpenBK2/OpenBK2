#include "StdAfx.h"
#include "./fade.h"

#include "MusicSystem.hpp"
#include "DBMusicSystem.h"
#include "Track.h"

namespace NMusicSystem
{


//	CFade


CFade::CFade( const NDb::SFade *_pFade, EMusicSystemVolume _eVolumeType, CTrack *_pTrackToEnd )
: pFade( _pFade ), eVolumeType( _eVolumeType ), fStartVolume( -1 ),
eState( EFS_NOT_STARTED ), pTrackToEnd( _pTrackToEnd ), timeFaded( 0 )
{
}

void CFade::SetCurrentVolume()
{
	if ( pFade->nFadeTime == 0 )
	{
		Singleton<IMusicSystem>()->SetVolume( eVolumeType, pFade->fFinalVolume * 0.01f );
	}
	else
	{
		const float fCoef = ( pFade->fFinalVolume - fStartVolume ) / pFade->nFadeTime;
		const float fTime = timeFaded;

		Singleton<IMusicSystem>()->SetVolume( eVolumeType, 
			0.01f * Clamp( fStartVolume + fCoef * fTime, Min( fStartVolume, pFade->fFinalVolume ), 
			Max( fStartVolume, pFade->fFinalVolume ) ) );
	}
}

void CFade::Stop()
{
}

void CFade::Play()
{
	timeLastCall = GetAbsTime();
}

void CFade::OnResetTimer()
{
	timeLastCall = GetAbsTime();
}

void CFade::Segment()
{
	const NTimer::STime curTime = GetAbsTime();
	switch( eState )
	{
	case EFS_NOT_STARTED:
		{
			timeLastCall = GetAbsTime();

			bool bStarting = false;
			if ( pTrackToEnd )
			{
				if ( pTrackToEnd->IsTimeToEndFade( pFade->nFadeTime ) )
					bStarting = true;
			}
			else 
				bStarting = true;

			if ( bStarting )
			{
				fStartVolume = 100.0f * Singleton<IMusicSystem>()->GetVolume( eVolumeType );
				eState = EFS_STARTED;
				if ( pFade->bPause && pFade->fFinalVolume != 0.0f  )
					Singleton<IMusicSystem>()->PauseMusic( eVolumeType, false );
				SetCurrentVolume();
			}
		}

		break;
	case EFS_STARTED:
		{
			const NTimer::STime curTime = GetAbsTime();
			timeFaded += curTime - timeLastCall;
			timeLastCall = curTime;

			SetCurrentVolume();
			if ( timeFaded > pFade->nFadeTime )
			{
				eState = EFS_FINISHED;
				if ( pFade->bPause && pFade->fFinalVolume == 0.0f )
					Singleton<IMusicSystem>()->PauseMusic( eVolumeType, true );
			}
		}

		break;
	case EFS_FINISHED:

		break;
	}
}

bool CFade::IsFinished() const
{
	return EFS_FINISHED == eState;
}


//CFades


void CFades::Update()
{
	// fades
	for ( CFades::iterator it = begin(); it != end(); ++it )
	{
		if ( (*it)->IsFinished() )
			it = erase( it );
		else
			(*it)->Segment();
	}
}
}
REGISTER_SAVELOAD_CLASS_NM( 0x111813C1, CFade, NMusicSystem )

