#pragma once

namespace NMusicSystem
{

interface IPlayListElement : public CObjectBase
{
	virtual void Segment() = 0;
	virtual bool IsFinished() const = 0;
	virtual void Stop() = 0;
	virtual void Play() = 0;
	virtual void OnResetTimer() = 0;
};

}
