#include "stdafx.h"

#include <thread>

#include "3Dmotor/GfxBuffers.h"
#include "GBinkPlayer.h"
#include "System/VFSOperations.h"
#include "ffmpeg_facade.h"

#include <chrono>
#include <iostream>

namespace NGScene
{

// CBinkVideoPlayer
class CBinkVideoPlayer: public IVideoPlayer
{
	OBJECT_NOCOPY_METHODS(CBinkVideoPlayer);
	std::unique_ptr<FFmpeg> ffmpeg_player;
	std::unique_ptr<CDataStream> pStream;
	ZDATA
	bool bForceUpdate = false;
	bool bStopped = false;
	uint32_t dwCopyFlags = 0;
	uint32_t dwPlayFlags = 0;
	/////
	std::string szFileName;
	bool bNeedUpdate = false; // BinkWait returned 0
	int nEndFrame = -1;
	int nFrameSkip = 0;
	ZEND int operator&( IBinSaver &f ) override {
		f.Add(2,&bForceUpdate);
		f.Add(3,&bStopped);
		f.Add(4,&dwCopyFlags);
		f.Add(5,&dwPlayFlags);
		f.Add(6,&szFileName);
		f.Add(7,&bNeedUpdate);
		f.Add(8,&nEndFrame);
		f.Add(9,&nFrameSkip);
		return 0;
	}

protected:
	void Recalc() override;
	bool NeedUpdate() override;

public:
	CBinkVideoPlayer() = default;
	CBinkVideoPlayer( const std::string &filename, uint32_t dwFlags );

	~CBinkVideoPlayer() override = default;

	void Play() override;
	bool Stop() override;
	bool Pause( bool bPause )override;
	bool OpenVideo() override;
	bool IsPlaying() const override;

	int GetCurrentFrame() const override;
	void SetCurrentFrame( int nFrame ) override;

	int GetLength() const override;
	int GetNumFrames() const override;
	void GetSize( CTPoint<int> *pSize ) const override;

	void PlayFragment( int nStartFrame, int _nEndFrame, int _nFrameSkip = 0 ) override;
};

IVideoPlayer* CreateVideoPlayer( const std::string &filename, uint32_t dwFlags )
{
	return new CBinkVideoPlayer( filename, dwFlags );
}

CBinkVideoPlayer::CBinkVideoPlayer( const std::string &filename, uint32_t _dwFlags ):
	dwPlayFlags( _dwFlags ), szFileName( filename )
{
}

int CBinkVideoPlayer::GetCurrentFrame() const
{
	if ( !IsPlaying() ) {
		return -1;
	}
	return ffmpeg_player->FrameNum;
}

void CBinkVideoPlayer::SetCurrentFrame( int nFrame )
{
	if ( !IsPlaying() ) {
		return;
	}

	ffmpeg_player->Goto(nFrame);
	Updated();
	bForceUpdate = true;

	if ( ( dwPlayFlags & PLAY_NO_TIME_UPDATE ) != 0 )
	{
		// "SlideShow" mode
		while ( !ffmpeg_player->Wait() )
		{
			std::this_thread::yield();
		}
	}
}

void CBinkVideoPlayer::Play()
{
	if ( OpenVideo() ) {
		ffmpeg_player->Play();
	}
}

bool CBinkVideoPlayer::Stop()
{
	if ( ffmpeg_player )
	{
		ffmpeg_player.reset();
		pStream.reset();
	}

	return true;
}

bool CBinkVideoPlayer::Pause( bool bPause )
{
	if ( !ffmpeg_player ) {
		return false;
	}

	return ffmpeg_player->Pause(bPause);
}

bool CBinkVideoPlayer::IsPlaying() const
{
	return ffmpeg_player && !bStopped;
}

int CBinkVideoPlayer::GetLength() const
{
	ASSERT( ffmpeg_player );
	return ffmpeg_player && ( ffmpeg_player->FrameRate > 0 ) ? 1000 * ffmpeg_player->Frames / ffmpeg_player->FrameRate : 0;
}

int CBinkVideoPlayer::GetNumFrames() const
{
	ASSERT( ffmpeg_player );
	return ffmpeg_player ? ffmpeg_player->Frames : 0;
}

void CBinkVideoPlayer::GetSize( CTPoint<int> *pSize ) const
{
	ASSERT( pSize );
	if ( !ffmpeg_player )
	{
		pSize->x = 0;
		pSize->y = 0;
		return;
	}

	pSize->x = ffmpeg_player->Width;
	pSize->y = ffmpeg_player->Height;
}

// Internal functions

void CBinkVideoPlayer::Recalc()
{
	if ( !ffmpeg_player ) {
		return;
	}

	if ( !IsValid( pValue ) )
	{
		CTPoint sSize( GetNextPow2( ffmpeg_player->Width ), GetNextPow2( ffmpeg_player->Height ) );
		pValue = NGfx::MakeTexture( sSize.x, sSize.y, 1, NGfx::SPixel8888::ID, NGfx::DYNAMIC_TEXTURE, NGfx::CLAMP );
	}
	if ( !IsValid( pValue ) ) {
		return;
	}

	bool bFrameUpdated = false;
	if ( ( dwPlayFlags & PLAY_NO_TIME_UPDATE ) == 0 )
	{
		while ( !bStopped && bNeedUpdate )
		{
			bFrameUpdated = true;
			ffmpeg_player->DoFrame();
			ffmpeg_player->NextFrame();

			if ( nEndFrame >= 0 )
			{
				// No frame skipping!

				if ( ffmpeg_player->FrameNum == nEndFrame )
				{
					Pause( true );
					nEndFrame = -1;
					nFrameSkip = 0;
					bFrameUpdated = false;
					bForceUpdate = true;
					break;
				}
			}

			bNeedUpdate = false;

			if ( ( ( dwPlayFlags & PLAY_LOOPED ) == 0 ) && ( ffmpeg_player->FrameNum == ffmpeg_player->Frames ) )
				bStopped = true;
		}
	}

	if ( bForceUpdate && !bFrameUpdated ) {
		ffmpeg_player->DoFrame();
	}

	NGfx::CTextureLock<NGfx::SPixel8888> sLock( pValue, 0, NGfx::EAccess::READWRITE );
	ffmpeg_player->CopyToBuffer( &sLock[0][0], sLock.GetStride(), ffmpeg_player->Height, 0, 0 );

	bForceUpdate = false;
	bNeedUpdate = false;
}

bool CBinkVideoPlayer::NeedUpdate()
{
	if (!ffmpeg_player) {
		return false;
	}
	if ( !ffmpeg_player->Wait() || bForceUpdate )
	{
		// Set NeedUpdate flag
		bNeedUpdate = true;

		return true;
	}
	return false;
}

bool CBinkVideoPlayer::OpenVideo()
{
	{
		pStream = std::unique_ptr<CDataStream>{ NVFS::GetMainVFS()->OpenFile( szFileName ) };
		if ( !pStream || !pStream->IsOk() || !pStream->CanRead() )
			return false;

		ffmpeg_player = FFmpeg::Open( (pStream->GetBuffer()), pStream->GetSize() );
	}

	if ( ffmpeg_player )
	{
		const float fSFXMasterVolume = NGlobal::GetVar( "Sound.SFXVolume", 1.0f );
		const long nVolume = 32768 * fSFXMasterVolume;
		ffmpeg_player->SetVolume(nVolume);
	}
	return ffmpeg_player != nullptr;
}

void CBinkVideoPlayer::PlayFragment( int nStartFrame, int _nEndFrame, int _nFrameSkip )
{
	if ( !IsPlaying() ) {
		Play();
	}

	if ( !IsPlaying() ) {
		return;			// Something went wrong
	}

	const int nStart = Clamp( nStartFrame, 0, ffmpeg_player->Frames - 1);
	nEndFrame = Clamp( _nEndFrame, nStart, ffmpeg_player->Frames - 1 );
	SetCurrentFrame( nStart );
	nFrameSkip = _nFrameSkip;
	Pause( false );
}

} // namespace

using namespace NGScene;
REGISTER_SAVELOAD_CLASS( UI, 0xB3320170, CBinkVideoPlayer );
