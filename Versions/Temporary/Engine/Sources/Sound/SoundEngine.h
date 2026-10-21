#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "Misc/HashFuncs.h"
#include "SFX.h"
#include "vendor/fmod/api/inc/fmod.h"

//DEBUG{
#ifndef _FINALRELEASE

struct SLogEntry
{
	ZDATA
	std::string szName;
	bool bLooped;
	int nStartPos;
	// uint32_t and not unsigned long, which is four bytes on Windows and eight
	// on Linux: this entry is serialized, so the two wrote different sizes for
	// the same log.
	uint32_t nStartTime;

	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &szName );
		f.Add( 3, &bLooped );
		f.Add( 4, &nStartPos );
		f.Add( 5, &nStartTime );
		return 0;
	}

	//void ToString( string *pString );
	//void FromString( const string &szCode );
};

class CPlayLog
{
	ZDATA
	std::vector<SLogEntry> log;
	int nCurPos;
	bool bLogIsFull;

	int nChannels;

	ESFXOutputType eOutput;
	int nFrequ;

	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &log );
		f.Add( 3, &nCurPos );
		f.Add( 4, &bLogIsFull );
		f.Add( 5, &nChannels );
		f.Add( 6, &eOutput );
		f.Add( 7, &nFrequ );
		return 0;
	}

public:
	CPlayLog();
	void ClearLog();
	void Add( const std::string &szName, bool bLooped, int nStartPos );
	void SaveToFile( const std::string &szFilename );
	void PlayFile( const std::string &szFileName, int nMaxSize );
};
#endif

typedef std::unordered_map<ISound *, int> CSoundChannelMap;
typedef std::unordered_map<int, CPtr<ISound>> CChannelSoundMap;

class CSoundEngine : public ISFX
{
	OBJECT_NOCOPY_METHODS( CSoundEngine );

	//
	struct SDriverInfo
	{
		std::string szDriverName;
		bool isHardware3DAccelerated; // this driver supports hardware accelerated 3d sound.
		bool supportEAXReverb;        // this driver supports EAX reverb
		bool supportReverb;           // this driver supports EAX2/A3D3 reverb
		bool supportEAX3;             // this driver supports EAX3
	};

	// initialization info - drivers
	typedef std::vector<SDriverInfo> CDriversInfo;
	CDriversInfo drivers; // [0] is default driver
	//
	NTimer::STime timeLastUpdate;
	//
	// channels management
	CSoundChannelMap channelsMap; // sound => channel map
	CChannelSoundMap soundsMap;   // channel => sound map
	//
	bool bInited;
	bool bEnableSFX;       // enable SFXes playing
	bool bEnableStreaming; // enable streaming playing
	bool bSoundCardPresent;
	bool bPaused; // is all SFX sounds paused?
	// The two the constructor does not name. Stated here instead, so that the
	// value does not depend on which of Set3DMode, a loaded save or neither
	// has run yet: Update reads b3DMode on the first frame after the engine is
	// created, and operator& writes bStreamingPaused into every save.
	bool bStreamingPaused = false; // is streaming sound paused
	//
	bool b3DMode = false;
	CVec3 vFormerListener;
	//
	void ClearChannels();
	//
	bool SearchDevices();
	//
	void ReEnableSounds();
	//
	~CSoundEngine() override;

public:
	CSoundEngine();
	int operator&( IBinSaver &saver ) override;

	void Set3DMode( bool _b3DMode ) override;
	// internal-use service functions
	void MapSound( ISound *pSound, int nChannel );
	//
	CObjectBase *QI( int nInterfaceTypeID ) override;
	// init and close sound system
	bool IsInitialized() override;
	bool Init( HWND hWnd, int nDriver, ESFXOutputType output, int nMixRate, int nMaxChannels ) override;

	//
	// enable SFXes and streaming
	void EnableSFX( bool bEnable ) override
	{
		bEnableSFX = bEnable;
		ReEnableSounds();
	}

	bool IsSFXEnabled() const override
	{
		return bEnableSFX && bSoundCardPresent;
	}

	//
	// setup
	void SetDistanceFactor( float fFactor ) override;
	void SetRolloffFactor( float fFactor ) override;
	//
	// sample sounds
	int PlaySample( ISound *pSound, bool bLooped = false, unsigned int nStartPos = 0 ) override;
	void StopSample( ISound *pSound ) override;
	void UpdateSample( ISound *pSound ) override;
	void StopChannel( int nChannel ) override;

	// Update sounds ( that is needed for 3D sounds )
	void Update( const CVec3 &vListener, const CVec3 &vCameraDir, NTimer::STime timeDiff ) override;
	//
	bool Pause( bool bPause ) override;
	bool IsPaused() override;
	bool IsPlaying( ISound *pSound ) override;

	unsigned int GetCurrentPosition( ISound *pSound ) override;
	void SetCurrentPosition( ISound *pSound, unsigned int pos ) override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
