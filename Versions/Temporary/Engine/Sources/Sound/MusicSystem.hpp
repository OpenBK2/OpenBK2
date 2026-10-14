#pragma once
#include "Fade.h"
#include "MusicSystem.h"
#include "vendor/fmod/api/inc/fmod.h"

namespace NDb
{
struct SMapMusic;
struct SComposition;
struct SMusicTrack;
} // namespace NDb

namespace NMusicSystem
{
FSOUND_STREAM *OpenTrack( CDataStream *pTrack );
NTimer::STime GetAbsTime();
class CTrack;
class CPlayList;

//

class CMusicSystem : public IMusicSystem
{
	OBJECT_NOCOPY_METHODS( CMusicSystem );

	enum EPlayListSwitchState
	{
		EPST_NOT_INITTED,
		EPST_JUST_STARTED,
		EPST_PLAYING_CURRENT,
		EPST_FADING_OLD_OUT,
		EPST_FADING_NEW_IN,
	};

	int nPlayList;
	std::vector<CPtr<CPlayList>> playlists;
	CPtr<CTrack> pVoiceTrack;
	std::vector<float> volumes;
	std::vector<int> pauses;
	std::vector<int> channels;

	CFades fades;
	int nDesiredPlayList;
	EPlayListSwitchState eState;
	CDBPtr<NDb::SMapMusic> pCurrentMapMusic;
	int operator&( IBinSaver &f ) override;
	void InitDefault();

	void UpdatePlayListChange();

public:
	CMusicSystem()
	{
		InitDefault();
	}

	bool IsPaused( EStreamType eType ) const override;
	void OnResetTimer() override;
	void Update() override;
	void Clear() override;
	void SetVolume( EMusicSystemVolume eType, float fVolume ) override;
	float GetVolume( EMusicSystemVolume eType ) const override;
	void PauseMusic( EMusicSystemVolume eType, bool bPause ) override;
	void PlayVoice( const NDb::SVoice *pVoice ) override;
	void Init( const NDb::SMapMusic *pMapMusic, int nActivePlayList ) override;
	void ChangePlayList( int _nPlayList ) override;
	bool CanChangePlayList() const override;
	int GetPlayList() const override;

	int GetNPlayLists() const override
	{
		return playlists.size();
	}

	void SetChannel( int nChannel, EStreamType eType )
	{
		channels[eType] = nChannel;
	}

	int GetChannel( EStreamType eType ) const
	{
		return channels[eType];
	}
};

CMusicSystem *GetMusicSystem();
} // namespace NMusicSystem
