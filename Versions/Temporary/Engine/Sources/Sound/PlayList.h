#pragma once

#include "vendor/fmod/api/inc/fmod.h"
#include "Fade.h"

namespace NDb
{
	struct SPlayTime;
	struct SMusicTrack;
	struct SFade;
	struct SPlayList;
	struct SPlayPause;
	struct SComposition;
}

namespace NMusicSystem
{

class CPlayList : public CObjectBase
{
	OBJECT_BASIC_METHODS( CPlayList )
	typedef std::list<CPtr<IPlayListElement> > CElements;

	enum EPlayListState
	{
		EPSS_IN,
		EPSS_FADING_OUT,
		EPSS_FADING_IN,
		EPSS_OUT,
	};

	ZDATA
	CDBPtr<NDb::SPlayList> pList;
	int nStillSong;													// still song index. when all still songs are finished, random songs are played

	//bool bPlaying;
	CElements elements;
	CFades fades;
	EPlayListState eState;
	CPtr<CFade> pPlaylistFade;									// for fading whole playlist
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pList); f.Add(3,&nStillSong); f.Add(4,&elements); f.Add(5,&fades); f.Add(6,&eState); f.Add(7,&pPlaylistFade); return 0; }

	void NextComposition();
	void LaunchComposition( const NDb::SComposition *_pComposition );
	
public:

	CPlayList() : eState( EPSS_OUT ), nStillSong( -1 ) { }
	CPlayList( const NDb::SPlayList *_pList )
		: eState( EPSS_OUT ), nStillSong( -1 )
	{
		Init( _pList );
	}
	
	void Init( const NDb::SPlayList *_pList );
	
	void Segment();
	/*
	bool IsPlaying() const;
	void Stop();
	void Play();
*/
	// for fading play list in and out
	void FadeOut();
	void FadeIn();
	bool IsIn() const { return eState == EPSS_IN; }
	bool IsOut() const { return eState == EPSS_OUT; }

	void OnResetTimer();
};

}

