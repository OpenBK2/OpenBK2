#pragma once

#include "Sound_export.h"

namespace NDb
{
	struct SVoice;
	struct SMapMusic;
}
enum EMusicSystemVolume
{
	EMS_MASTER,													// master volume

	EMS_MUSIC_MASTER,										// MUSIC STREAM volume
	EMS_MUSIC_FADE_SELF,								// MUSIC STREAM volume, set by "fader" that launch MUSIC stream
	EMS_MUSIC_FADE_FROM_VOICE,					// MUSIC STREAM volume, set by voice stream faders

	EMS_VOICE_MASTER,										// VOICE STREAM volume
	EMS_VOICE_SELF,										  // VOICE STREAM volume, set by voice stream faders

	_EMS_COUNT,
};

enum EStreamType
{
	EST_MUSIC,
	EST_VOICE,
};

// music has 2 streams: MUSIC & voice. "MUSIC STREAM" generally is music. 
// "VOICE STREAM" is generally voice over and some short streaming sounds.
// MUSIC STREAM is running always with settings, set in MapMusic structure. It has pauses and fade in/fade out.
// VOICE STREAM is running periodically, it is launched through specific functions.
interface IMusicSystem : public CObjectBase
{
	enum { tidTypeID = 0x11181340 };

	virtual void Update() = 0;
	virtual void Clear() = 0;	

	// voice stream functions
	virtual void PlayVoice( const NDb::SVoice *pVoice ) = 0;

	// play lists
	virtual void Init( const NDb::SMapMusic *pMapMusic, int nActivePlayList ) = 0;
	virtual void ChangePlayList( int nPlayList ) = 0;
	virtual bool CanChangePlayList() const = 0;
	virtual int GetPlayList() const = 0;
	virtual int GetNPlayLists() const = 0;

	// volume
	virtual void SetVolume( EMusicSystemVolume eType, float fVolume ) = 0;
	virtual float GetVolume( EMusicSystemVolume eType ) const = 0;

	// pause
	virtual void PauseMusic( EMusicSystemVolume eType, bool bPause ) = 0;
	virtual bool IsPaused( EStreamType eType ) const = 0;

	// Need to call after reseting timer
	virtual void OnResetTimer() = 0;
};

SOUND_EXPORT IMusicSystem * CreateMusicSystem();

