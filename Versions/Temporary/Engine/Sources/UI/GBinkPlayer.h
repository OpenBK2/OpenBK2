#pragma once
#include "System/Dg.h"

namespace NGfx
{
	class CTexture;
}

namespace NGScene
{
	struct IVideoPlayer : public CPtrFuncBase<NGfx::CTexture>
	{
		enum
		{
			COPY_ALL = 0x00000001,
			PLAY_LOOPED = 0x00000002,
			//PLAY_FROM_MEMORY = 0x00000004,
			PLAY_WITH_SOUND = 0x00000008,
			PLAY_NO_TIME_UPDATE = 0x00000010,
		};

		virtual void Play() = 0;
		virtual bool Stop() = 0;
		virtual bool Pause(bool bPause) = 0;
		virtual bool IsPlaying() const = 0;
		virtual bool OpenVideo() = 0;

		virtual int GetCurrentFrame() const = 0;
		virtual void SetCurrentFrame(int nFrame) = 0;

		virtual int GetLength() const = 0;
		virtual int GetNumFrames() const = 0;
		virtual void GetSize(CTPoint<int>* pSize) const = 0;

		virtual void PlayFragment(int nStartFrame, int nEndFrame, int nFrameSkip = 0) = 0;
	};

	IVideoPlayer* CreateVideoPlayer(const std::string& szFileName, DWORD dwFlags);

}
