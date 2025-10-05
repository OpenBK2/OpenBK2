#include "stdafx.h"

#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "SoundEngine.h"

#include "Misc/Win32Helper.h"
#include "Sound2D.h"
#include "Misc/StrProc.h"
#include "UI/UI.h"
#include "System/Commands.h"
#include "System/BasicShare.h"
#include "Sound/DBSoundDesc.h"
#include "System/VFSOperations.h"

extern CBasicShare<CDBID, CSoundSample> shareSoundSample;

static NWin32Helper::CCriticalSection critSection;
REGISTER_SAVELOAD_CLASS( 0x1107BC01, CSoundEngine );
static float s_fDistanceFactor = 10.0f;
static float s_fDopplerFactor = 1.0f;
static float s_fRollofFactor = 0.3f;
//DEBUG{
#ifndef _FINALRELEASE

CPlayLog playLog;
static const int SOUND_PLAY_LOG_SIZE = 1000;


void CPlayLog::ClearLog()
{
	log.clear();
	log.resize( SOUND_PLAY_LOG_SIZE );
	nCurPos = 0;
	bLogIsFull = false;
}

CPlayLog::CPlayLog()
{
	ClearLog();
}

void CPlayLog::Add( const std::string &szName, bool bLooped, int nStartPos )
{
	log[nCurPos].szName = szName;
	log[nCurPos].bLooped = bLooped;
	log[nCurPos].nStartPos = nStartPos;
	log[nCurPos].nStartTime = Singleton<IGameTimer>()->GetAbsTime();
	++nCurPos;
	bLogIsFull |= nCurPos == SOUND_PLAY_LOG_SIZE;
	nCurPos %= SOUND_PLAY_LOG_SIZE;
}

void CPlayLog::SaveToFile( const std::string &szFileName )
{
	const int nEntries = bLogIsFull ? SOUND_PLAY_LOG_SIZE : nCurPos + 1;
	const int nStartPos = bLogIsFull ? ( nCurPos + SOUND_PLAY_LOG_SIZE - 1 ) % SOUND_PLAY_LOG_SIZE : 0;
	
	CFileStream stream( StrFmt( "%s.soundlog", szFileName.c_str() ), CFileStream::WIN_CREATE );

	nChannels = FSOUND_GetMaxChannels();
	eOutput = (ESFXOutputType)FSOUND_GetOutput();
	nFrequ = FSOUND_GetOutputRate();

	if ( !stream.IsOk() )
		return;

	CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
	operator&( *pSaver.GetPtr() );
}

void CPlayLog::PlayFile( const std::string &szFileName, int nMaxSize )
{
	CFileStream stream( StrFmt( "%s.soundlog", szFileName.c_str() ), CFileStream::WIN_READ_ONLY );

	if ( !stream.IsOk() )
		return;

	CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_READ );
	operator&( *pSaver.GetPtr() );
	
	ISFX * pSFX = Singleton<ISFX>();
	
	int nStartPos = bLogIsFull ? ( nCurPos + SOUND_PLAY_LOG_SIZE -1 ) % SOUND_PLAY_LOG_SIZE : 0;
	int nEntries = bLogIsFull ? SOUND_PLAY_LOG_SIZE : nCurPos;
	
	std::list<FSOUND_SAMPLE *> toDelete;
	for ( int i = 0; i < Min( nEntries, nMaxSize ); ++i )
	{
		const SLogEntry &entry = log[( i + nStartPos ) % SOUND_PLAY_LOG_SIZE];
		const NDb::SSoundDesc * pDesc = NDb::Get<NDb::SSoundDesc>( entry.szName );
		
		if ( pDesc )
		{
			CFileStream steram( NVFS::GetMainVFS(), pDesc->szSoundPath );

			if ( stream.IsOk() )
			{
				int nSleepTime = i > 0 ? entry.nStartTime - log[( i - 1 + nStartPos ) % SOUND_PLAY_LOG_SIZE].nStartTime : 0;
				nSleepTime = Clamp( nSleepTime, 0, 1000 );
				Sleep( nSleepTime );
				DebugTrace( "SoundLog play: %s", entry.szName );

				const int nSize = stream.GetSize();
				std::vector<char> data(nSize);
				stream.Read( &data[0], nSize );
				FSOUND_SAMPLE *pSample = FSOUND_Sample_Load( FSOUND_UNMANAGED, &data[0], /*( b3DSoundShare ? FSOUND_HW3D : FSOUND_2D ) |*/ FSOUND_LOADMEMORY, 0, nSize );
				const int nChannel = FSOUND_PlaySoundEx( FSOUND_FREE, pSample, 0, true );
				if ( 0 != nStartPos )
					FSOUND_SetCurrentPosition( nChannel, nStartPos );
				FSOUND_SetPaused( nChannel, false );
				toDelete.push_back( pSample );
			}
		}
	}
}

#endif
//DEBUG}

class CPlayVisitor : public ISFXVisitor
{
	CSoundEngine *pSFX;
	//
public:
	//
	void Init( class CSoundEngine *_pSFX ) { pSFX = _pSFX; }
	//
	int VisitSound2D( CSFXSound *pSound )
	{
		FSOUND_SAMPLE *sample = pSound->GetSample()->GetInternalContainer();
		if ( sample == 0 )
			return -1;
		const int nChannel = FSOUND_PlaySoundEx( FSOUND_FREE, sample, 0, true );
		pSound->SetChannel( nChannel );
		pSFX->MapSound( pSound, nChannel );
		pSound->Update( pSFX );
		return nChannel;
	}
	int VisitSound3D( CSFXSound *pSound )
	{
		FSOUND_SAMPLE *sample = pSound->GetSample()->GetInternalContainer();
		if ( sample == 0 )
			return -1;
		const int nChannel = FSOUND_PlaySoundEx( FSOUND_FREE, sample, 0, true );
		pSound->SetChannel( nChannel );
		pSFX->MapSound( pSound, nChannel );
		pSound->Update( pSFX );
		return nChannel;
	}
};

static CPlayVisitor thePlayVisitor;

CSoundEngine::CSoundEngine() 
: bInited( false ), bPaused( false ), bEnableSFX( true ), bEnableStreaming( true ), 
bSoundCardPresent( false ), timeLastUpdate( -1 )
{  
}

bool CSoundEngine::SearchDevices()
{
	if ( FSOUND_GetVersion() < FMOD_VERSION )
	{
		OutputDebugString( "Error : You are using the wrong DLL version!\n" );
		return false;
	}
	FSOUND_SetOutput( FSOUND_OUTPUT_DSOUND );
	int nNumDrivers = FSOUND_GetNumDrivers();
	drivers.resize( nNumDrivers );
	for ( int i = 0; i < nNumDrivers; ++i )
	{
		SDriverInfo &dr = drivers[i];
		dr.szDriverName = (const char *) FSOUND_GetDriverName( i );
		unsigned int nCaps;
		FSOUND_GetDriverCaps( i, &nCaps );
		dr.isHardware3DAccelerated = nCaps & FSOUND_CAPS_HARDWARE;
		dr.supportEAXReverb = nCaps & FSOUND_CAPS_EAX2;//FSOUND_CAPS_EAX;
		dr.supportReverb = nCaps & FSOUND_CAPS_EAX2;
		dr.supportEAX3 = nCaps & FSOUND_CAPS_EAX3;
	}
	return ( nNumDrivers != 0 );
}

bool CSoundEngine::IsInitialized()
{
	return bInited;
}

CObjectBase* CSoundEngine::QI( int nInterfaceTypeID )
{
	if ( nInterfaceTypeID == 0 ) 
		return reinterpret_cast<CObjectBase*>( FSOUND_GetOutputHandle() );
	return 0;
}

bool CSoundEngine::Init( HWND hWnd, int nDriver, ESFXOutputType output, int nMixRate, int nMaxChannels )
{
	if ( !SearchDevices() )
		return false;
	
	NI_ASSERT( nDriver < drivers.size(), StrFmt("Can't find driver %d (max found %d)", nDriver, drivers.size()) );
	FSOUND_SetDriver( nDriver );
	bSoundCardPresent = true;
	FSOUND_SetHWND( hWnd );
	FSOUND_SetMinHardwareChannels( nMaxChannels );
	FSOUND_SetBufferSize( 100 );
	FSOUND_Stream_SetBufferSize( 1000 );

	if ( !FSOUND_Init( nMixRate, 128, 0 ) )
	{
		OutputDebugString( "NFMSound::Start():error!\n" );
		//NI_ASSERT( 0, StrFmt("Failed to init FMOD: %d", FSOUND_GetError()) );
		//soft reaction on error: sound card not found.
		bSoundCardPresent = false;
		return true;
		//return false;
	}

#ifdef _DEBUG
	OutputDebugString( "Using \"" );
	OutputDebugString( drivers[nDriver].szDriverName.c_str() );
	OutputDebugString( "\" sound driver.\n" );
	if ( drivers[nDriver].isHardware3DAccelerated )
		OutputDebugString("- Driver supports hardware 3D sound!\n" );
	if ( drivers[nDriver].supportEAXReverb )
		OutputDebugString("- Driver supports EAX reverb!\n" );
	if ( drivers[nDriver].supportReverb )
		OutputDebugString("- Driver supports EAX 2.0 !\n" );
	if ( drivers[nDriver].supportEAX3 )
		OutputDebugString("- Driver supports EAX 3.0 !\n" );
	
	OutputDebugString("Mixer = ");
	switch ( FSOUND_GetMixer() )
	{
		case FSOUND_MIXER_BLENDMODE:	
			OutputDebugString("FSOUND_MIXER_BLENDMODE\n"); 
			break;
		case FSOUND_MIXER_MMXP5:		
			OutputDebugString("FSOUND_MIXER_MMXP5\n"); 
			break;
		case FSOUND_MIXER_MMXP6:		
			OutputDebugString("FSOUND_MIXER_MMXP6\n"); 
			break;
		case FSOUND_MIXER_QUALITY_FPU:	
			OutputDebugString("FSOUND_MIXER_QUALITY_FPU\n"); 
			break;
		case FSOUND_MIXER_QUALITY_MMXP5:
			OutputDebugString("FSOUND_MIXER_QUALITY_MMXP5\n"); 
			break;
		case FSOUND_MIXER_QUALITY_MMXP6:
			OutputDebugString("FSOUND_MIXER_QUALITY_MMXP6\n"); 
			break;
	};
#endif

	//
	vFormerListener = VNULL3;
	bInited = true;
	return true;
}

CSoundEngine::~CSoundEngine()
{
	drivers.clear();
	channelsMap.clear();
	soundsMap.clear();
	FSOUND_Close();
}

void CSoundEngine::SetDistanceFactor( float fFactor )
{
	FSOUND_3D_SetDistanceFactor( fFactor );
}

void CSoundEngine::SetRolloffFactor( float fFactor )
{
	NI_VERIFY( (fFactor >= 0) && (fFactor <= 10), StrFmt("Rolloff factor (%g) must be in range [0..10]", fFactor), return );
	FSOUND_3D_SetRolloffFactor( fFactor );
}

void CSoundEngine::Update( const CVec3 &vListener, const CVec3 &vCameraDir, NTimer::STime timeDiff )
{
	if ( b3DMode )
	{
		CVec3 vPos( vListener.x, vListener.z, vListener.y );
		
		// camera speed - ignore it
		CVec3 vSpeed( VNULL3 );//timeDiff != 0 ? ( vFormerListener - vListener ) / timeDiff : VNULL3 );
		
		vFormerListener = vPos;

		CVec3 vFwd( vCameraDir.x, vCameraDir.z, vCameraDir.y );
		CVec2 vTop2D( fabs( vCameraDir.x, vCameraDir.y ), -vCameraDir.z );

		CVec3 vTop( vCameraDir.x * vTop2D.x, vTop2D.y, vCameraDir.y * vTop2D.x );
		Normalize( &vFwd );
		Normalize( &vTop );
		FSOUND_3D_Listener_SetAttributes( vPos.m, vSpeed.m, vFwd.x, vFwd.y, vFwd.z, vTop.x, vTop.y, vTop.z );
		FSOUND_Update();
	}
	//
	timeLastUpdate = Singleton<IGameTimer>()->GetAbsTime();
	//
	const int nNumChannels = FSOUND_GetChannelsPlaying();

#ifndef _FINALRELEASE
	IDebugSingleton *pDebug = Singleton<IDebugSingleton>();
	IStatsSystemWindow *pStatsSystemWindow = pDebug->GetStatsWindow();
	if ( pStatsSystemWindow )
		pStatsSystemWindow->UpdateEntry( L"SoundChannels", NStr::ToUnicode(StrFmt( "%d", nNumChannels )), 0xff00ff00 );
#endif // _FINALRELEASE

	if ( nNumChannels > 0 )
		ClearChannels();
}

void CSoundEngine::MapSound( ISound *pSound, int nChannel )
{
	channelsMap.insert( std::pair<ISound*, int>( pSound, nChannel ) );
	soundsMap.insert( std::pair<int, CPtr<ISound> >( nChannel, pSound ) );
}

bool CSoundEngine::IsPaused()
{
	return bPaused;
}

bool CSoundEngine::Pause( bool bPause )
{
	if ( bPaused != bPause ) 
	{
		for ( CChannelSoundMap::iterator it = soundsMap.begin(); it != soundsMap.end(); ++it )
		{
			FSOUND_SetPaused( it->first, bPause );
		}
		bPaused = bPause;
	}
	return bPause;
}

void CSoundEngine::ClearChannels()
{
	if ( bPaused )
		return;
	//
	std::list<int> channels;
	// collect finished and invalid channels
	for ( CChannelSoundMap::iterator it = soundsMap.begin(); it != soundsMap.end(); ++it )
	{
		if ( !IsValid( it->second ) )
		{
			FSOUND_StopSound( it->first );
		}
		if ( FSOUND_IsPlaying( it->first ) == 0 )
		{
			channels.push_back( it->first );
			FSOUND_StopSound( it->first );
		}
	}
	// clear it
	for ( std::list<int>::iterator it = channels.begin(); it != channels.end(); ++it )
	{
		const int nChannel = *it;
		ISound *pSound = soundsMap[nChannel];
		soundsMap.erase( nChannel );
		channelsMap.erase( pSound );
	}
}

int CSoundEngine::PlaySample( ISound *pSound, bool bLooped, unsigned int nStartPos )
{
	if ( pSound == 0 || !bEnableSFX )
		return -1;
	//
	thePlayVisitor.Init( this );
	if ( checked_cast<CSFXSound*>( pSound )->GetSample() == 0 )
		return -1;

	CSoundSample *pSample = checked_cast<CSFXSound*>( pSound )->GetSample();
	
	int nLength = pSound->GetLenght();
	int nSampleRate = pSound->GetSampleRate();
	int nTimeLength = 1000 * nLength / nSampleRate;
	if ( bLooped )
		nStartPos %= nTimeLength;
	if ( nTimeLength == 0 )
		return 0;

	if ( nTimeLength < nStartPos )
	{
		return 0;
	}

	pSample->SetLoop( bLooped );
	const int nChannel = pSound->Visit( &thePlayVisitor );

	if ( 0 != nStartPos )
		FSOUND_SetCurrentPosition( nChannel, nStartPos );
	FSOUND_SetPaused( nChannel, false );
#ifndef _FINALRELEASE
	//DEBUG{
	// add sound to sounds play log
	playLog.Add( pSample->GetName(), bLooped, nStartPos );
	//DEBUG}
#endif
	
	return nChannel;
}

void CSoundEngine::UpdateSample( ISound *pSound )
{
	checked_cast<CSFXSound*>( pSound )->Update( this );
}

void CSoundEngine::StopSample( ISound *pSound )
{
	CSoundChannelMap::iterator pos = channelsMap.find( pSound );
	if ( pos != channelsMap.end() )
	{		
		StopChannel( pos->second );
	}
}

bool CSoundEngine::IsPlaying( ISound *pSound )
{
	if ( !pSound )
		return false;
	//
	CSoundChannelMap::iterator pos = channelsMap.find( pSound );
	return pos != channelsMap.end();
}

void CSoundEngine::StopChannel( int nChannel )
{
 	if ( nChannel == -1 )
		return;
	//
	const bool bRes = FSOUND_StopSound( nChannel );
	CChannelSoundMap::iterator pos = soundsMap.find( nChannel );
	if ( pos != soundsMap.end() )
	{
		
		channelsMap.erase( pos->second );
		soundsMap.erase( pos );
	}
}

unsigned int CSoundEngine::GetCurrentPosition( ISound * pSound )
{
	CSoundChannelMap::iterator pos = channelsMap.find( pSound );
	if ( pos != channelsMap.end() )
	{
		int nChannel = (*pos).second;
		return FSOUND_GetCurrentPosition( nChannel );
	}
	return 0;
}

void CSoundEngine::SetCurrentPosition( ISound * pSound, unsigned int pos )
{
	CSoundChannelMap::iterator it = channelsMap.find( pSound );
	if ( it != channelsMap.end() )
	{
		int nChannel = (*it).second;
		FSOUND_SetCurrentPosition( nChannel, pos );
	}
}

void CSoundEngine::Set3DMode( bool _b3DMode )
{
	b3DMode = _b3DMode;
	if ( b3DMode )
	{
		FSOUND_3D_SetDistanceFactor( s_fDistanceFactor );
		FSOUND_3D_SetDopplerFactor( s_fDopplerFactor );
		FSOUND_3D_SetRolloffFactor( s_fRollofFactor );
	}
	ClearChannels();
}

void CSoundEngine::ReEnableSounds()
{
	// turn all SFXes off
	if ( !bEnableSFX )
	{
		for ( CChannelSoundMap::iterator it = soundsMap.begin(); it != soundsMap.end(); ++it )
		{
			if ( FSOUND_IsPlaying(it->first) )
				FSOUND_StopSound( it->first );
		}
		soundsMap.clear();
		channelsMap.clear();
	}
}

int CSoundEngine::operator&( IBinSaver &saver )
{
	if ( saver.IsReading() )
	{
		channelsMap.clear();
		soundsMap.clear();
		shareSoundSample.Clear();
	}

	saver.Add( 3, &bSoundCardPresent );
	saver.Add( 4, &timeLastUpdate );
	saver.Add( 12, &bPaused );
	saver.Add( 13, &bStreamingPaused );
	saver.Add( 14, &b3DMode );
	saver.Add( 15, &vFormerListener );

	if ( saver.IsReading() )
		Pause( true );

	return 0;
}

ISFX *CreateSoundEngine() { return new CSoundEngine(); }

void SoundParams( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() < 3 )
		return;

	FSOUND_3D_SetDistanceFactor( NStr::ToFloat( NStr::ToMBCS( paramsSet[0] ) ) );
	FSOUND_3D_SetDopplerFactor( NStr::ToFloat( NStr::ToMBCS( paramsSet[1] ) ) );
	FSOUND_3D_SetRolloffFactor( NStr::ToFloat( NStr::ToMBCS( paramsSet[2] ) ));
}

void SaveSoundLog( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() < 1 )
		return;
#ifndef _FINALRELEASE
	playLog.SaveToFile( NStr::ToMBCS( paramsSet[0] ) );
#endif
}

void PlaySoundLog( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() < 1 )
		return;
#ifndef _FINALRELEASE
	if ( paramsSet.size() < 2 )
		playLog.PlayFile( NStr::ToMBCS( paramsSet[0] ), SOUND_PLAY_LOG_SIZE );
	else
	{
		playLog.PlayFile( NStr::ToMBCS( paramsSet[0] ), NStr::ToInt( NStr::ToMBCS( paramsSet[1] ) ) );
	}
#endif
}

START_REGISTER(SFXParams)
REGISTER_VAR_EX( "Sound.DistanceFactor", NGlobal::VarFloatHandler, &s_fDistanceFactor, 10.0f, STORAGE_USER );
REGISTER_VAR_EX( "Sound.DopplerFactor", NGlobal::VarFloatHandler, &s_fDopplerFactor, 1.0f, STORAGE_USER );
REGISTER_VAR_EX( "Sound.RollofFactor", NGlobal::VarFloatHandler, &s_fRollofFactor, 0.3f, STORAGE_USER );

REGISTER_CMD( "SoundParams", SoundParams );
REGISTER_CMD( "PlaySoundLog", PlaySoundLog );
REGISTER_CMD( "SaveSoundLog", SaveSoundLog );

FINISH_REGISTER

