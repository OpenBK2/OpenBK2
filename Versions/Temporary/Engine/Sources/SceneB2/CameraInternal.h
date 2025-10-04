#pragma once

#include "3DLib/Transform.h"
#include "CameraBasicMouseMutator.h"
#include "CameraScriptMutators.h"

//class CScriptMoviesMutatorHolder;

namespace NCamera
{
	struct SEarthQuake
	{
		float fAmplitude;
		float fAttenuation;
		float fDuration;
		float fTime;
		//
		SEarthQuake() {	}
		SEarthQuake( float _fAmplitude, float _fAttenuation, float _fDuration )
			: fAmplitude( _fAmplitude ), fAttenuation( _fAttenuation ), fDuration( _fDuration ), fTime( 0 ) {	}
	};
}

class CCamera : public ICamera, protected NInput::CGMORegContainer
{
	OBJECT_NOCOPY_METHODS( CCamera )

	bool bMayaHandleType;
	// projection transform params
	float fScreenWidth;
	float fScreenHeight;
	float fNearClipPlane;
	float fFarClipPlane;
	bool bPerspectiveTransform;
	// transform matrices, built from data above
	SHMatrix matView;											// view matrix
	SHMatrix matProj;											// projective matrix
	CTransformStack transformStack;
	//
	//CDGPtr<NCamera::CCameraPlacement> pScriptMutator;
	CDGPtr<NCamera::CCameraBasicMouseMutator> pMouseMutator;
	CDGPtr<CScriptMoviesMutatorHolder> pScriptMutatorsHolder;
	bool bUseMovieMutator;
	bool bWasUpdated;						// camera was updated from input
	bool bWasUpdatedExternally; // camera was updated from external
	bool bIsMovieFinished;			// the movie segment has reached it`s finish
	bool bIsPlayingFinal;				// the current movie segment is the final one
	list<NCamera::SEarthQuake> earthquakes;
	NTimer::STime timeLastUpdate;
	//
	void MsgMouseRotation( const SGameMessage &msg, bool bBegin );
	void SetupDefaultMouseMutator();

public:
	CCamera();

	// scroll, rotate, apply limits and rebuild matrices
	void Update();
	//
	void SetLimits( const NCamera::ELimitsType eLimitsType, const NCamera::SCameraLimits &limits );
	void GetLimits( const NCamera::ELimitsType eLimitsType, NCamera::SCameraLimits *pLimits ) const;
	void SetAnchorLimits( const CTRect<float> &vLimits );
	// set camera's anchor
	void SetAnchor( const CVec3 &_vAnchor );
	// 
	void ResetToDefault();
	void SetPlacement( const float fDist, const float fPitch, const float fYaw );
	virtual void SetDistance( const float fDist );
	virtual float GetDistance() const;
	void SwitchAutoPositioning( const bool bAllowAutoPositioning );
	void SwitchManualScrolling( const string &szLocker, const bool bManualOn );

	const CVec3 GetListener() const;

	void GetPlacement( float *pfDist, float *pfPitch, float *pfYaw );
	void GetCameraParams( float *pfNear, float *pfFar, float *pfFOV ) const 
	{
		(*pfNear) = fNearClipPlane;
		(*pfFar) = fFarClipPlane;
		(*pfFOV) = GetFOV();
	}

	virtual void GetCameraState( CVec3 *pvPos, float *pfYaw, float *pfPitch, float *pfRoll, float *pfFOV )	const;
	virtual void SetCameraState( const CVec3 &vPos, const float fYaw, const float fPitch, const float fRoll, const float fFOV );

	const CVec3 GetPos() const;
	const CVec3 GetAnchor() const;

	// script movies engine
	void SetScriptMutatorsHolder( CScriptMoviesMutatorHolder *_pScriptMutatorsHolder ) { pScriptMutatorsHolder = _pScriptMutatorsHolder; }
	CScriptMoviesMutatorHolder* GetScriptMutatorsHolder() { return pScriptMutatorsHolder; }

	bool IsScriptHolderActive() const { return ( (pScriptMutatorsHolder != 0) && (!pScriptMutatorsHolder->IsStopped()) ); }

	void FinishMovie( bool bSmoothEnd = false );
	bool ValidateScriptPlacement( NCamera::CCameraPlacement *pPlacement ) const;
	void ConformMouseMutator2ScriptMutator();

	// border scrolling
	void ResetScrolling();
	void SetScrollSpeedX( const float _fSpeed );
	void SetScrollSpeedY( const float _fSpeed );

	// rotation
	void SetYawSpeed( const float fSpeed );

	// set projection transforms
	void SetPerspectiveTransform( float fWidth, float fHeight, float fNear, float fFar );
	void SetOrthographicTransform( float fWidth, float fHeight, float fNear, float fFar );

	float GetYaw() const;
	void SetYaw( float fYaw );
	float GetPitch() const;
	void SetPitch( float fPitch );
	float GetFOV() const;
	void SetFOV( float fFOV );

	float GetRoll() const { return DEF_ROLL; }
	void SetRoll( float _Roll ) {}

	void SetHandleType( bool _bMayaHandleType );
	bool GetHandleType() { return bMayaHandleType; }
	//
	// get view volume planes
	// get transform matrices: result transform, projection, viewport
	const SHMatrix& GetViewMatrix() const { return matView; }
	const SHMatrix& GetProjectiveMatrix() const { return matProj; }
	class CTransformStack& GetTransform() { return transformStack; }
	// projective ray
	void GetProjectiveRay( CVec3 *pvOrig, CVec3 *pvDir, const CVec2 &vScreenPos ) const;
	void GetProjectiveRayPoints( CVec3 *pvNear, CVec3 *pvFar, const CVec2 &vScreenPos ) const;
	// msg processing
	virtual bool ProcessEvent( const SGameMessage &msg );
	// updates checking
	bool WasUpdated() { return bWasUpdated; };
	// earthquakes
	void AddEarthquake( const CVec3 &vPos, float fPower );
	void ClearEarthquakes();

	virtual void SetCameraPosition( const CQuat &quat ) { }
	virtual const CQuat GetCameraPosition() { return CQuat( 0.0f, 0.0f, 0.0f, 0.0f ); }

	int operator&( IBinSaver &saver );
};


