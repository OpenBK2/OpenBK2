#pragma once

#include "SFX.h"
#include "SoundSample.h"

namespace NDb
{
	struct SSoundDesc;
}

class CSFXSound : public ISound 
{
	CPtr<CSoundSample> pSample;
	CDBPtr<NDb::SSoundDesc> pDesc;
	int nChannel;
	bool bLooped;
	int nVolumeType;
protected:
	virtual void Init();
	int GetChannel() const { return nChannel; }

public:
	int operator&( IBinSaver &saver );
	CSFXSound( const NDb::SSoundDesc *_pDesc, const bool _bLooped )
		: pDesc( _pDesc ), bLooped( _bLooped ), nVolumeType( 2 )
	{
	}
	CSFXSound() : nChannel( -1 ), bLooped( false )
	{  }
	virtual ~CSFXSound() {  }

	void SetVolumeType( int _nVolumeType ) { nVolumeType = _nVolumeType; }
	int GetVolumeType() const { return nVolumeType; }

	void SetSample( CSoundSample *_pSample );
	CSoundSample* GetSample();
	void SetChannel( int _nChannel );
	bool IsPlaying();
	void SetLooping( bool bEnable, int nStart = -1, int nEnd = -1 );
	unsigned int GetLenght();
	unsigned int GetSampleRate();
	int Play();

	virtual void Update( ISFX * pSFX ) = 0;
};

class CSound2D : public CSFXSound
{
	OBJECT_NOCOPY_METHODS( CSound2D );
	float fVolume;
	float fPan;
	void Init();
public:
	int operator&( IBinSaver &saver ); 

	CSound2D( const NDb::SSoundDesc *pDesc, const bool bLooped );
	CSound2D() : fVolume( 0.0f ), fPan( 0.0f )
	{  }

	int Visit( interface ISFXVisitor *pVisitor );
	void Update( ISFX * pSFX );

	// 2D
	void SetVolume( float _fVolume )
	{ 
		NI_ASSERT( fVolume != -4.3160208e+008, "unitialized!" );
		fVolume = _fVolume; 
	}
	float GetVolume() const { return fVolume; }
	void SetPan( float _fPan ) { fPan = _fPan; }
	float GetPan() const { return fPan; }


	// 3D
	void SetMinMax( const float fMinDist, const float fMaxDist ) { NI_ASSERT( false, "WRONG CALL" ); }
	CVec3 GetPos() const { NI_ASSERT( false, "WRONG CALL" ); return VNULL3; }
	void SetPos( const CVec3 &vPos ) { NI_ASSERT( false, "WRONG CALL" ); }
	CVec3 GetSpeed() const { NI_ASSERT( false, "WRONG CALL" ); return VNULL3; }
	void SetSpeed( const CVec3 &vSpeed ) { NI_ASSERT( false, "WRONG CALL" ); }
};


class CSound3D : public CSFXSound
{
	OBJECT_NOCOPY_METHODS( CSound3D );
	ZDATA_(CSFXSound)
	CVec3 vPos;
	CVec3 vSpeed;
	float fMin;
	float fMax;
	float fVolume;
	ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CSFXSound*)this); f.Add(2,&vPos); f.Add(3,&vSpeed); f.Add(4,&fMin); f.Add(5,&fMax); f.Add(6,&fVolume); OnSerialize( f ); return 0; }
	void Init();
public:
	void OnSerialize( IBinSaver &f )
	{
		if ( f.IsReading() )
		{
			Init();
		}
	}

	CSound3D( const NDb::SSoundDesc *pDesc, const bool bLooped );
	CSound3D() : vPos( VNULL3), vSpeed( VNULL3 ), fVolume( 1.0f ) {  }

	int Visit( interface ISFXVisitor *pVisitor );
	void Update( ISFX * pSFX );


	// 3D
	void SetMinMax( const float fMinDist, const float fMaxDist )
	{
		fMin = fMinDist;
		fMax = fMaxDist;
	}

	CVec3 GetPos() const 
	{ 
		return vPos; 
	}
	void SetPos( const CVec3 &_vPos ) 
	{ 
		vPos = _vPos;
	}
	CVec3 GetSpeed() const 
	{ 
		return vSpeed; 
	}
	void SetSpeed( const CVec3 &_vSpeed ) 
	{ 
		vSpeed = _vSpeed;
	}

	// 2D
	void SetVolume( float _fVolume ) { fVolume = _fVolume; }
	float GetVolume() const { return fVolume; }
	void SetPan( float _fPan ) { NI_ASSERT( false, "WRONG CALL" ); }
	float GetPan() const { NI_ASSERT( false, "WRONG CALL" ); return 0.0f; }

};
