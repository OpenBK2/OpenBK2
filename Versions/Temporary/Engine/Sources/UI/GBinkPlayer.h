#pragma once

#include "UI_export.h"

namespace NGScene
{
	class IVideoPlayer: virtual public CObjectBase
	{
		OBJECT_BASIC_METHODS(IVideoPlayer)
	public:
		enum {
			COPY_ALL = 1,
			PLAY_NO_TIME_UPDATE = 2,
			PLAY_WITH_SOUND = 4,
		};
		int GetCurrentFrame();
		void SetCurrentFrame(int frame);
		int GetNumFrames();

		bool IsPlaying();

		void Play();
		void Stop();
		bool Pause(bool paused);
		void PlayFragment(int start_frame, int end_frame, int frame_skip);
		void GetSize(void *);

	private:
		bool paused_;
		int frame_;
	};
	
	UI_EXPORT IVideoPlayer * CreateVideoPlayer(std::string filename, int flags);
}

