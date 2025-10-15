#include "stdafx.h"
#include "Sound2D.h"
#include "DBSoundDesc.h"
#include "System/BasicShare.h"
#include "System/Commands.h"

CBasicShare<CDBID, CSoundSample> shareSoundSample(120, false);

REGISTER_SAVELOAD_CLASS( SOUND, 0x110B8B00, CSound2D );
REGISTER_SAVELOAD_CLASS( SOUND, 0x11191C00, CSound3D );


float g_fSFXVolume = 0.5f;
float g_fVoiceVolume = 0.5f;


//CSFXSound


int CSFXSound::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pSample );
	saver.Add( 2, &pDesc );
	saver.Add( 3, &nChannel );
	saver.Add( 4, &bLooped );
	saver.Add( 5, &nVolumeType );

	if ( saver.IsReading() )
	{
		nChannel = -1;
		Init();
	}
	return 0;
}

void CSFXSound::Init()
{
	if ( !pSample )
	{
		pSample = shareSoundSample.Get( pDesc->GetDBID() );
		pSample->SetLoop( bLooped );
	}
}

void CSFXSound::SetSample( CSoundSample *_pSample )
{ 
	pSample = _pSample; 
}

int CSFXSound::Play() 
{ 
	int nChannel = FSOUND_PlaySound( FSOUND_FREE, GetSample()->GetInternalContainer() );
	SetChannel( nChannel );
	return nChannel;
}

CSoundSample* CSFXSound::GetSample() 
{ 
	return pSample; 
}

void CSFXSound::SetChannel( int _nChannel ) 
{ 
	nChannel = _nChannel; 
}

bool CSFXSound::IsPlaying() 
{ 
	if ( (nChannel != -1) && FSOUND_IsPlaying(nChannel) )
		return FSOUND_GetCurrentSample( nChannel ) == pSample->GetInternalContainer();
	else
		return false;
}

void CSFXSound::SetLooping( bool bEnable, int nStart, int nEnd )
{
	FSOUND_Sample_SetMode( pSample->GetInternalContainer(), bEnable ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF );
	if ( (nStart != -1) && (nEnd != -1) )
		FSOUND_Sample_SetLoopPoints( pSample->GetInternalContainer(), nStart, nEnd );
}

unsigned int CSFXSound::GetLenght()
{
	return FSOUND_Sample_GetLength( pSample->GetInternalContainer() );
}

unsigned int CSFXSound::GetSampleRate()
{
	int freq = 44000;
	FSOUND_Sample_GetDefaults( pSample->GetInternalContainer(), &freq, 0, 0, 0 );
	return freq;
}


// CSound2D


int CSound2D::operator&( IBinSaver &saver )
{
	saver.Add( 2, static_cast<CSFXSound*>( this ) );
	saver.Add( 3, &fVolume );
	saver.Add( 4, &fPan );

	return 0;
}

CSound2D::CSound2D( const NDb::SSoundDesc *_pDesc, const bool _bLooped )
: CSFXSound( _pDesc, _bLooped ),
	fVolume( 0.0f ), fPan( 0.0f )
{
	Init();
}

int CSound2D::Visit( struct ISFXVisitor *pVisitor )
{ 
	return pVisitor->VisitSound2D( this ); 
}

void CSound2D::Update( ISFX * pSFX )
{
	if ( GetChannel() != -1 )
	{
		const int nPan = Clamp( int(128 + (GetPan() * 127)), 0, 255 );
		FSOUND_SetPan( GetChannel(), nPan );
		
		float fTypedVolume = 0;
		switch( GetVolumeType() )
		{
		case 2:
			fTypedVolume = g_fSFXVolume;
			break;
		case 1:
			fTypedVolume = g_fVoiceVolume;
			break;
		}
		const int nVolume = Clamp( int( fTypedVolume * ( GetVolume() >= 0.0f ? GetVolume() * 255.0f : 255.0f ) ), 0, 255 );
		FSOUND_SetVolume( GetChannel(), nVolume );
	}
}

void CSound2D::Init()
{
	CSoundSample::Set3DMode( false );
	CSFXSound::Init();
}


// CSound3D


CSound3D::CSound3D( const NDb::SSoundDesc *_pDesc, const bool _bLooped )
: CSFXSound( _pDesc, _bLooped ),
	vPos( VNULL3 ), vSpeed( VNULL3 ), fMax( 1000000000.0f ), fMin( 1.0f ), fVolume( 1.0f )
{
	Init();
}

int CSound3D::Visit( struct ISFXVisitor *pVisitor )
{
	return pVisitor->VisitSound3D( this );
}

void CSound3D::Update( ISFX * pSFX )
{
	if ( GetChannel() != -1 )
	{
		const CVec3 vP( vPos.x, vPos.z, vPos.y );
		const CVec3 vS( vSpeed.x, vSpeed.z, vSpeed.y );
		//if ( fMax != 0.0f )
			//FSOUND_3D_SetMinMaxDistance( GetChannel(), fMin, fMax );
		const int nVolume = Clamp( int(GetVolume() >= 0.0f ? GetVolume() * 255.0f : 255.0f), 0, 255 );
		FSOUND_SetVolume( GetChannel(), nVolume );
		FSOUND_3D_SetAttributes( GetChannel(), vP.m, vS.m );
	}
}

void CSound3D::Init()
{
	CSoundSample::Set3DMode( true );
	CSFXSound::Init();
}

START_REGISTER(Sound)
REGISTER_VAR_EX( "Sound.SFXVolume", NGlobal::VarFloatHandler, &g_fSFXVolume, 0.5f, STORAGE_USER );
REGISTER_VAR_EX( "Sound.VoiceVolume", NGlobal::VarFloatHandler, &g_fVoiceVolume, 0.5f, STORAGE_USER );
FINISH_REGISTER


