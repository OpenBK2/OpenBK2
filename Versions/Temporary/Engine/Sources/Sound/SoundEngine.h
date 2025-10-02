#ifndef __SOUNDENGINE_H__
#define __SOUNDENGINE_H__
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../vendor/fmod/api/inc/fmod.h"
#include "SFX.h"
#include "..\Misc\HashFuncs.h"

//DEBUG{
#ifndef _FINALRELEASE

struct SLogEntry
{
	ZDATA
	string szName;
	bool bLooped;
	int nStartPos;
	unsigned long nStartTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szName); f.Add(3,&bLooped); f.Add(4,&nStartPos); f.Add(5,&nStartTime); return 0; }

	//void ToString( string *pString );
	//void FromString( const string &szCode );
};

class CPlayLog
{
	ZDATA
	vector<SLogEntry> log;
	int nCurPos;
	bool bLogIsFull;
	
	int nChannels;
	
	ESFXOutputType eOutput;
	int nFrequ;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&log); f.Add(3,&nCurPos); f.Add(4,&bLogIsFull); f.Add(5,&nChannels); f.Add(6,&eOutput); f.Add(7,&nFrequ); return 0; }
public:
	CPlayLog();
	void ClearLog();
	void Add( const string &szName, bool bLooped, int nStartPos );
	void SaveToFile( const string &szFilename );
	void PlayFile( const string &szFileName, int nMaxSize );
};
#endif 

typedef hash_map<ISound*, int, SDefaultPtrHash> CSoundChannelMap;
typedef hash_map<int, CPtr<ISound> > CChannelSoundMap;

class CSoundEngine : public ISFX
{
	OBJECT_NOCOPY_METHODS( CSoundEngine );
	//
	struct SDriverInfo
	{
		string szDriverName;
		bool isHardware3DAccelerated;				// this driver supports hardware accelerated 3d sound.
		bool supportEAXReverb;							// this driver supports EAX reverb
		bool supportReverb;									// this driver supports EAX2/A3D3 reverb  
		bool supportEAX3;										// this driver supports EAX3
	};
	// initialization info - drivers
	typedef vector<SDriverInfo> CDriversInfo;
	CDriversInfo drivers;									// [0] is default driver
	//
	NTimer::STime timeLastUpdate;
	//
	// channels management
	CSoundChannelMap channelsMap;					// sound => channel map
	CChannelSoundMap soundsMap;						// channel => sound map
	//
	bool bInited;
	bool bEnableSFX;											// enable SFXes playing
	bool bEnableStreaming;								// enable streaming playing
	bool bSoundCardPresent;								
	bool bPaused;													// is all SFX sounds paused?
	bool bStreamingPaused;								// is streaming sound paused
	//
	bool b3DMode;
	CVec3 vFormerListener;
	//
	void ClearChannels();
	//
	bool SearchDevices();
	//
	void ReEnableSounds();
	//
	~CSoundEngine();
	
public:
	CSoundEngine();
	int operator&( IBinSaver &saver ); 

	void Set3DMode( bool _b3DMode );
	// internal-use service functions
	void MapSound( ISound *pSound, int nChannel );
	//
	virtual CObjectBase* QI( int nInterfaceTypeID );
	// init and close sound system
	virtual bool IsInitialized();
	virtual bool Init( HWND hWnd, int nDriver, ESFXOutputType output, int nMixRate, int nMaxChannels );
	//
	// enable SFXes and streaming
	virtual void EnableSFX( bool bEnable ) { bEnableSFX = bEnable; ReEnableSounds(); }
	virtual bool IsSFXEnabled()const { return bEnableSFX && bSoundCardPresent; }
	//
	// setup
	virtual void SetDistanceFactor( float fFactor );
	virtual void SetRolloffFactor( float fFactor );
	//
	// sample sounds
	int PlaySample( ISound *pSound, bool bLooped = false, unsigned int nStartPos = 0 );
	void StopSample( ISound *pSound );
	void UpdateSample( ISound *pSound );
	void StopChannel( int nChannel );

	// Update sounds ( that is needed for 3D sounds )
	virtual void Update( const CVec3 &vListener, const CVec3 &vCameraDir, NTimer::STime timeDiff );
	//
	virtual bool Pause( bool bPause );
	virtual bool IsPaused();
	virtual bool IsPlaying( ISound *pSound );

	unsigned int GetCurrentPosition( ISound * pSound );
	virtual void SetCurrentPosition( ISound * pSound, unsigned int pos );
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __SOUNDENGINE_H__
