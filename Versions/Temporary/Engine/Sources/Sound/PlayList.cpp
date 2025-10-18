#include "stdafx.h"
#include "PlayList.h"

#include "DBMusicSystem.h"
#include "MusicSystem.hpp"
#include "Misc/Win32Random.h"
#include "Track.h"
#include "Pause.h"

namespace NMusicSystem
{



//CPlayList


void CPlayList::NextComposition()
{
	// calculate next composition.
	if ( ++nStillSong >= pList->stillOrder.size() )
	{
		// pick random song
		float fTotal = 0;
		for ( int i = 0; i < pList->randomOrder.size(); ++i )
			fTotal += pList->randomOrder[i].fWeight == 0.0f ? 1.0f : pList->randomOrder[i].fWeight;
		const float fRandom = NWin32Random::RandomCheck( 0.0f, fTotal );

		float fCurrent = 0;
		for ( int i = 0; i < pList->randomOrder.size(); ++i )
		{
			fCurrent += pList->randomOrder[i].fWeight == 0.0f ? 1.0f : pList->randomOrder[i].fWeight;
			if ( fCurrent > fRandom )
			{
				LaunchComposition( pList->randomOrder[i].pComposition );
				return;
			}
		}

		nStillSong = 0;
		if (pList->stillOrder.size() > nStillSong) {
			LaunchComposition(pList->stillOrder[nStillSong]);
		}
	}
	else {
		if (pList->stillOrder.size() > nStillSong) {
			LaunchComposition(pList->stillOrder[nStillSong]);
		}
	}
}

void CPlayList::LaunchComposition( const NDb::SComposition *_pComposition )
{
	elements.clear();

	if ( _pComposition == 0 || _pComposition->pTrack == 0 )
		return;

	CTrack *pTrack = new CTrack( _pComposition->pTrack, _pComposition->pPlayTime,  EST_MUSIC );
	elements.push_back( pTrack );
	elements.push_back( new CPause( _pComposition->playPauseAfter ) );

	// fades
	fades.clear();
	const NTimer::STime curTime = GetAbsTime();
	if ( _pComposition->pFadeIn )
		fades.push_back( new CFade( _pComposition->pFadeIn, EMS_MUSIC_FADE_SELF, 0 ) );
	if ( _pComposition->pFadeOut )
		fades.push_back( new CFade( _pComposition->pFadeOut, EMS_MUSIC_FADE_SELF, pTrack ) );

	fades.Update();
	pTrack->Segment();
}

void CPlayList::Init( const NDb::SPlayList *_pList )
{
	pList = _pList;
	if ( eState != EPSS_OUT )
		NextComposition();
}

void CPlayList::Segment()
{
	// process play list fade
	if ( pPlaylistFade )
	{
		NI_ASSERT( eState == EPSS_FADING_OUT || eState == EPSS_FADING_IN, "wrong playlist state" );
		pPlaylistFade->Segment();
		if ( pPlaylistFade->IsFinished() )
		{
			switch( eState )
			{
			case EPSS_FADING_OUT:
				{
					eState = EPSS_OUT;
					CElements::iterator pos = elements.begin();
					if ( pos != elements.end() )
						(*pos)->Stop();
				}
				break;
			case EPSS_FADING_IN:
				eState = EPSS_IN;

				break;
			}
			pPlaylistFade = 0;
		}
	}

	if ( eState == EPSS_OUT )
		return;

	fades.Update();

	// tracks and pauses
	for ( CElements::iterator it = elements.begin(); it != elements.end(); )
	{
		if ( (*it)->IsFinished() )
			it = elements.erase( it );
		else
		{
			(*it)->Segment();
			break;
		}
	}
	if ( elements.empty() )
		NextComposition();
}

void CPlayList::FadeOut()
{
	NI_ASSERT( eState == EPSS_IN, "wrong playlist state" );
	if ( eState == EPSS_IN )
	{
		if ( pList->pFadeOut )
		{
			eState = EPSS_FADING_OUT;
			pPlaylistFade = new CFade( pList->pFadeOut, EMS_MUSIC_FADE_SELF, 0 );
			pPlaylistFade->Segment();
		}
		else
		{
			eState = EPSS_OUT;
			CElements::iterator pos = elements.begin();
			if ( pos != elements.end() )
				(*pos)->Stop();
		}
	}
}

void CPlayList::FadeIn()
{
	if ( !pList )
		return;

	NI_ASSERT( eState == EPSS_OUT, "wrong playlist state" );
	if ( eState == EPSS_OUT )
	{
		if ( pList->pFadeIn )
		{
			eState = EPSS_FADING_IN;
			pPlaylistFade = new CFade( pList->pFadeIn, EMS_MUSIC_FADE_SELF, 0 );
			pPlaylistFade->Segment();
		}
		else
			eState = EPSS_IN;
		if ( elements.empty() )
			NextComposition();
		CElements::iterator pos = elements.begin();
		if ( pos != elements.end() )
			(*pos)->Play();
	}
}

void CPlayList::OnResetTimer()
{
	for ( CElements::iterator it = elements.begin(); it != elements.end(); )
	{
		if ( (*it)->IsFinished() )
			it = elements.erase( it );
		else
		{
			(*it)->OnResetTimer();
			break;
		}
	}
}

}
REGISTER_SAVELOAD_CLASS_NM( 0x111813C0, CPlayList, NMusicSystem  )

