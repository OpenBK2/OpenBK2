#ifndef __WINDOW_PLAYER_H__
#define __WINDOW_PLAYER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "window.h"
#include "GBinkPlayer.h"
#include "..\System\DG.h"


class CWindowPlayer : public CWindow, public IPlayer
{
	OBJECT_BASIC_METHODS(CWindowPlayer)
	
	struct SMovie
	{
		string szFileName;							// short file name (with respect to 'data' subdir)
		bool bCanSkipMovie;									// can we this movie with mouse/SPACE/ENTER/[ESC] buttons
		bool bCanSkipSequence;							// we can skip all movies sequence with ESC button
		bool bSlideShowMode;						// "SlideShow" mode (wait for complete loading of each frame)
		
		SMovie() : bCanSkipMovie( false ), bCanSkipSequence( false ), bSlideShowMode( false ) {}
		
		//
		int operator&( IXmlSaver &saver );
		int operator&( IBinSaver &saver );
	};

	vector<SMovie> movies;						// all movies to play
	int nCurrMovie;												// current movie to play
	bool bPaused;
	CDGPtr<NGScene::IVideoPlayer> pPlayer;

	CPtr<NDb::SWindowPlayer> pInstance;
	CDBPtr<NDb::SWindowPlayerShared> pShared;
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }
	
	DWORD GetSceneFlags();
	void UpdatePlayer();
public:
	int operator&( IBinSaver &saver );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );
	virtual void Reposition( const CTRect<float> &parentRect );

	virtual void Visit( interface IUIVisitor *pVisitor );

	//IPlayer{
	virtual void SetSequence( const string &szFileName );
	virtual void Play();
	virtual bool Stop();
	virtual bool Pause( bool bPause );
	virtual bool IsPlaying() const;
	virtual bool IsPaused() const;
	virtual void SkipMovie();
	virtual void SkipSequence();

	int GetCurrentFrame() const;
	void SetCurrentFrame( int nFrame );
	int GetNumFrames() const;

	virtual void PlayFragment( int nStartFrame, int nEndFrame, int nFrameSkip = 0 );
	//IPlayer}
};


#endif //__WINDOW_PLAYER_H__
