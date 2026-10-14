#pragma once

#include "GBinkPlayer.h"
#include "System/Dg.h"
#include "Window.h"

#include <cstdint>

class CWindowPlayer : public CWindow, public IPlayer
{
	OBJECT_BASIC_METHODS( CWindowPlayer )

	struct SMovie
	{
		std::string szFileName; // short file name (with respect to 'data' subdir)
		bool bCanSkipMovie;     // can we this movie with mouse/SPACE/ENTER/[ESC] buttons
		bool bCanSkipSequence;  // we can skip all movies sequence with ESC button
		bool bSlideShowMode;    // "SlideShow" mode (wait for complete loading of each frame)

		SMovie()
			: bCanSkipMovie( false )
			, bCanSkipSequence( false )
			, bSlideShowMode( false )
		{
		}

		//
		int operator&( IXmlSaver &saver );
		int operator&( IBinSaver &saver );
	};

	std::vector<SMovie> movies; // all movies to play
	int nCurrMovie;             // current movie to play
	bool bPaused;
	CDGPtr<NGScene::IVideoPlayer> pPlayer;

	CPtr<NDb::SWindowPlayer> pInstance;
	CDBPtr<NDb::SWindowPlayerShared> pShared;

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

	uint32_t GetSceneFlags();
	void UpdatePlayer();

public:
	int operator&( IBinSaver &saver ) override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;
	void Reposition( const CTRect<float> &parentRect ) override;

	void Visit( struct IUIVisitor *pVisitor ) override;

	//IPlayer{
	void SetSequence( const std::string &szFileName ) override;
	void Play() override;
	bool Stop() override;
	bool Pause( bool bPause ) override;
	bool IsPlaying() const override;
	bool IsPaused() const override;
	void SkipMovie() override;
	void SkipSequence() override;

	int GetCurrentFrame() const override;
	void SetCurrentFrame( int nFrame ) override;
	int GetNumFrames() const override;

	void PlayFragment( int nStartFrame, int nEndFrame, int nFrameSkip = 0 ) override;
	//IPlayer}
};
