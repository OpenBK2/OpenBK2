#include "stdafx.h"
#include "GBinkPlayer.h"
#include "System/Basic.h"

namespace NGScene {

	int IVideoPlayer::GetCurrentFrame()
	{
		return frame_;
	}

	void IVideoPlayer::SetCurrentFrame(int frame)
	{
		frame_ = frame;
	}

	int IVideoPlayer::GetNumFrames()
	{
		return 0;
	}

	bool IVideoPlayer::IsPlaying()
	{
		return false;
	}

	void IVideoPlayer::Stop()
	{
	}

	void IVideoPlayer::Play()
	{
	}

	bool IVideoPlayer::Pause(bool paused)
	{
		paused_ = paused;
		return paused_;
	}

	void IVideoPlayer::PlayFragment(int start_frame, int end_frame, int frame_skip)
	{
	}

	void IVideoPlayer::GetSize(void *) {

	}

	IVideoPlayer * CreateVideoPlayer(const string filename, int flags)
	{
		return new IVideoPlayer();
	}


}

using namespace NGScene;

BASIC_REGISTER_CLASS( IVideoPlayer );

