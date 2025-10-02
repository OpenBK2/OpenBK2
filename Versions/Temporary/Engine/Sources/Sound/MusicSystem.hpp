#pragma once
#include "MusicSystem.h"

#include "vendor/fmod/api/inc/fmod.h"
#include "Fade.h"

namespace NDb
{
	struct SMapMusic;
	struct SComposition;
	struct SMusicTrack;
}
namespace NMusicSystem
{
FSOUND_STREAM* OpenTrack( CDataStream *pTrack );
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
	vector<CPtr<CPlayList> > playlists;
	CPtr<CTrack> pVoiceTrack;
	vector<float> volumes;
	vector<int> pauses;
	vector<int> channels;

	CFades fades;
	int nDesiredPlayList;
	EPlayListSwitchState eState;
	CDBPtr<NDb::SMapMusic> pCurrentMapMusic;
	int operator&( IBinSaver &f );
	void InitDefault();
	
	void UpdatePlayListChange();
public:
	CMusicSystem() 
	{
		InitDefault();
	}

	bool IsPaused( EStreamType eType ) const;
	void OnResetTimer();
	void Update();
	void Clear();
	void SetVolume( EMusicSystemVolume eType, float fVolume );
	float GetVolume( EMusicSystemVolume eType ) const;
	void PauseMusic( EMusicSystemVolume eType, bool bPause );
	void PlayVoice( const NDb::SVoice *pVoice ) ;
	void Init( const NDb::SMapMusic *pMapMusic, int nActivePlayList );
	void ChangePlayList( int _nPlayList );
	bool CanChangePlayList() const;
	int GetPlayList() const;
	int GetNPlayLists() const
	{
		return playlists.size();
	}

	void SetChannel( int nChannel, EStreamType eType ) { channels[eType] = nChannel; }
	int GetChannel( EStreamType eType ) const  { return channels[eType]; }
};

CMusicSystem * GetMusicSystem();
}

