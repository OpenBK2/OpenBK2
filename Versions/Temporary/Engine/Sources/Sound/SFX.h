
#pragma once


#include "Sound_export.h"

enum ESFXOutputType
{
	SFX_OUTPUT_NO,
	SFX_OUTPUT_WINMM,
	SFX_OUTPUT_DSOUND,
	SFX_OUTPUT_EAX2,
	SFX_OUTPUT_EAX3,
};

struct ISFXVisitor
{
	virtual int VisitSound2D( class CSFXSound *pSound ) = 0;
	virtual int VisitSound3D( class CSFXSound *pSound ) = 0;
};

struct ISound : public CObjectBase
{
	// visiting
	virtual int Visit( struct ISFXVisitor *pVisitor ) = 0;
	
	// 0.0f ... 1.0f
	virtual void SetVolume( float nVolume ) = 0;
	virtual float GetVolume()const = 0;

	virtual void SetVolumeType( int nVolumeType ) = 0;
	virtual int GetVolumeType() const = 0;
	
	// -1.0f ... 1.0f
	virtual void SetPan( float nPan ) = 0;
	virtual float GetPan() const = 0;

	virtual CVec3 GetPos() const = 0;
	virtual void SetPos( const CVec3 &vPos ) = 0;

	virtual CVec3 GetSpeed() const = 0;
	virtual void SetSpeed( const CVec3 &vSpeed ) = 0;
	
	virtual void SetMinMax( const float fMinDist, const float fMaxDist ) = 0;
	
	// продолжительность звука в самплах
	virtual unsigned int GetLenght() =0;
	virtual unsigned int GetSampleRate() =0;
};

struct ISFX : public CObjectBase
{
	enum { tidTypeID = 0x110BDC40 };
	//
	virtual CObjectBase* QI( int nInterfaceTypeID ) = 0;
	// Init and close sound system
	virtual bool IsInitialized() = 0;
	virtual bool Init( HWND hWnd, int nDriver, ESFXOutputType output, int nMixRate, int nMaxChannels ) = 0;
	//
	// enable SFXes and streaming
	virtual void EnableSFX( bool bEnable ) = 0;
	virtual bool IsSFXEnabled()const=0;
	//
	// setup
	virtual void SetDistanceFactor( float fFactor ) = 0;
	virtual void SetRolloffFactor( float fFactor ) = 0;
	//
	// sample sounds
	virtual int PlaySample( ISound *pSound, bool bLooped = false, unsigned int nStartPos = 0 ) = 0;
	
	// update sound acording to internal parametres
	virtual void UpdateSample( ISound *pSound ) = 0;
	virtual void StopSample( ISound *pSound ) = 0;
	virtual void StopChannel( int nChannel ) = 0;
	//
	// Update sounds ( that is needed for 3D sounds )
	virtual void Update( const CVec3 &vListener, const CVec3 &vCameraDir, NTimer::STime timeDiff ) = 0;
	virtual void Set3DMode( bool b3DMode ) = 0;
	//
	virtual bool Pause( bool bPause ) = 0;
	virtual bool IsPaused() = 0;
	
	virtual bool IsPlaying( ISound *pSound ) = 0;

	// текущая позиция прогирыша
	virtual unsigned int GetCurrentPosition( ISound * pSound ) = 0;
	virtual void SetCurrentPosition( ISound * pSound, unsigned int pos ) = 0;
};

SOUND_EXPORT ISFX *CreateSoundEngine();


