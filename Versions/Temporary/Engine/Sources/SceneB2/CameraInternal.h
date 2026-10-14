#pragma once

#include "3DLib/Transform.h"
#include "CameraBasicMouseMutator.h"
#include "CameraScriptMutators.h"

//class CScriptMoviesMutatorHolder;

namespace NCamera
{
struct SEarthQuake
{
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	float fAmplitude = {};
	float fAttenuation = {};
	float fDuration = {};
	float fTime = {};

	//
	SEarthQuake() {}

	SEarthQuake( float _fAmplitude, float _fAttenuation, float _fDuration )
		: fAmplitude( _fAmplitude )
		, fAttenuation( _fAttenuation )
		, fDuration( _fDuration )
		, fTime( 0 )
	{
	}
};
} // namespace NCamera

class CCamera : public ICamera, protected NInput::CGMORegContainer
{
	OBJECT_NOCOPY_METHODS( CCamera )

	bool bMayaHandleType = {};
	// projection transform params
	float fScreenWidth = {};
	float fScreenHeight = {};
	float fNearClipPlane = {};
	float fFarClipPlane = {};
	bool bPerspectiveTransform = {};
	// transform matrices, built from data above
	SHMatrix matView; // view matrix
	SHMatrix matProj; // projective matrix
	CTransformStack transformStack;
	//
	//CDGPtr<NCamera::CCameraPlacement> pScriptMutator;
	CDGPtr<NCamera::CCameraBasicMouseMutator> pMouseMutator;
	CDGPtr<CScriptMoviesMutatorHolder> pScriptMutatorsHolder;
	bool bUseMovieMutator = {};
	bool bWasUpdated = {};           // camera was updated from input
	bool bWasUpdatedExternally = {}; // camera was updated from external
	bool bIsMovieFinished = {};      // the movie segment has reached it`s finish
	bool bIsPlayingFinal = {};       // the current movie segment is the final one
	std::list<NCamera::SEarthQuake> earthquakes;
	NTimer::STime timeLastUpdate = {};
	//
	void MsgMouseRotation( const SGameMessage &msg, bool bBegin );
	void SetupDefaultMouseMutator();

public:
	CCamera();

	// scroll, rotate, apply limits and rebuild matrices
	void Update() override;
	//
	void SetLimits( const NCamera::ELimitsType eLimitsType, const NCamera::SCameraLimits &limits ) override;
	void GetLimits( const NCamera::ELimitsType eLimitsType, NCamera::SCameraLimits *pLimits ) const override;
	void SetAnchorLimits( const CTRect<float> &vLimits ) override;
	// set camera's anchor
	void SetAnchor( const CVec3 &_vAnchor ) override;
	//
	void ResetToDefault() override;
	void SetPlacement( const float fDist, const float fPitch, const float fYaw ) override;
	void SetDistance( const float fDist ) override;
	float GetDistance() const override;
	void SwitchAutoPositioning( const bool bAllowAutoPositioning ) override;
	void SwitchManualScrolling( const std::string &szLocker, const bool bManualOn ) override;

	const CVec3 GetListener() const override;

	void GetPlacement( float *pfDist, float *pfPitch, float *pfYaw ) override;

	void GetCameraParams( float *pfNear, float *pfFar, float *pfFOV ) const override
	{
		( *pfNear ) = fNearClipPlane;
		( *pfFar ) = fFarClipPlane;
		( *pfFOV ) = GetFOV();
	}

	void GetCameraState( CVec3 *pvPos, float *pfYaw, float *pfPitch, float *pfRoll, float *pfFOV ) const override;
	void SetCameraState(
			const CVec3 &vPos, const float fYaw, const float fPitch, const float fRoll, const float fFOV ) override;

	const CVec3 GetPos() const override;
	const CVec3 GetAnchor() const override;

	// script movies engine
	void SetScriptMutatorsHolder( CScriptMoviesMutatorHolder *_pScriptMutatorsHolder ) override
	{
		pScriptMutatorsHolder = _pScriptMutatorsHolder;
	}

	CScriptMoviesMutatorHolder *GetScriptMutatorsHolder() override
	{
		return pScriptMutatorsHolder;
	}

	bool IsScriptHolderActive() const
	{
		return ( ( pScriptMutatorsHolder != 0 ) && ( !pScriptMutatorsHolder->IsStopped() ) );
	}

	void FinishMovie( bool bSmoothEnd = false ) override;
	bool ValidateScriptPlacement( NCamera::CCameraPlacement *pPlacement ) const;
	void ConformMouseMutator2ScriptMutator() override;

	// border scrolling
	void ResetScrolling() override;
	void SetScrollSpeedX( const float _fSpeed ) override;
	void SetScrollSpeedY( const float _fSpeed ) override;

	// rotation
	void SetYawSpeed( const float fSpeed ) override;

	// set projection transforms
	void SetPerspectiveTransform( float fWidth, float fHeight, float fNear, float fFar ) override;
	void SetOrthographicTransform( float fWidth, float fHeight, float fNear, float fFar ) override;

	float GetYaw() const override;
	void SetYaw( float fYaw ) override;
	float GetPitch() const override;
	void SetPitch( float fPitch ) override;
	float GetFOV() const override;
	void SetFOV( float fFOV ) override;

	float GetRoll() const
	{
		return DEF_ROLL;
	}

	void SetRoll( float _Roll ) {}

	void SetHandleType( bool _bMayaHandleType ) override;

	bool GetHandleType() override
	{
		return bMayaHandleType;
	}

	//
	// get view volume planes
	// get transform matrices: result transform, projection, viewport
	const SHMatrix &GetViewMatrix() const override
	{
		return matView;
	}

	const SHMatrix &GetProjectiveMatrix() const override
	{
		return matProj;
	}

	class CTransformStack &GetTransform() override
	{
		return transformStack;
	}

	// projective ray
	void GetProjectiveRay( CVec3 *pvOrig, CVec3 *pvDir, const CVec2 &vScreenPos ) const override;
	void GetProjectiveRayPoints( CVec3 *pvNear, CVec3 *pvFar, const CVec2 &vScreenPos ) const override;
	// msg processing
	bool ProcessEvent( const SGameMessage &msg ) override;

	// updates checking
	bool WasUpdated() override
	{
		return bWasUpdated;
	}

	// earthquakes
	void AddEarthquake( const CVec3 &vPos, float fPower ) override;
	void ClearEarthquakes() override;

	void SetCameraPosition( const CQuat &quat ) override {}

	const CQuat GetCameraPosition() override
	{
		return CQuat( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	int operator&( IBinSaver &saver ) override;
};
