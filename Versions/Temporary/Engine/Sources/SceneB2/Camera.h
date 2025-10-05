#pragma once

#include "SceneB2_export.h"

#include "System/Dg.h"
#include "Stats_B2_M1/DBMapInfo.h"

#define DEF_YAW 0.0f
#define DEF_PITCH 45.0f
#define DEF_FOV 26.0f
#define DEF_ROLL 0.0f

namespace NDb
{
	struct SCameraLimits;
	//
};
struct SGameMessage;
class CScriptMoviesMutatorHolder;

namespace NCamera
{
	enum ELimitsType
	{
		CAMERA_LIMITS_YAW,
		CAMERA_LIMITS_PITCH,
		CAMERA_LIMITS_DISTANCE
	};
	enum EMode
	{
		CAM_MODE_VIEW,
		CAM_MODE_MOVIE
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SCameraLimits
	{
		float fMin;
		float fMax;
		float fAve;
		float fAutoSpeed;
		float fManualSpeed;

		bool bCyclic;

		SCameraLimits() 
			: fMin( 0.0f ), fMax( 0.0f ), fAve( 0.0f ), fAutoSpeed( 0.0f ), fManualSpeed( 0.0f ), bCyclic( false )
		{
		}
		SCameraLimits( const float _fMin, const float _fMax, const float _fAve, const float _fAutoSpeed, const float _fManualSpeed )
			: fMin( _fMin ), fMax( _fMax ), fAve( _fAve ), fAutoSpeed( _fAutoSpeed ), fManualSpeed( _fManualSpeed ), bCyclic( false )
		{
		}
		SCameraLimits( const float _fMin, const float _fMax, const float _fAve, const float _fAutoSpeed, const float _fManualSpeed, const bool _bCyclic )
			: fMin( _fMin ), fMax( _fMax ), fAve( _fAve ), fAutoSpeed( _fAutoSpeed ), fManualSpeed( _fManualSpeed ), bCyclic( _bCyclic )
		{
		}
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CCameraPlacement : public NDb::SScriptCameraPlacement, public CVersioningBase
	{
	public:
		ZDATA_( NDb::SScriptCameraPlacement )
		//
		//	fYaw - in Degrees
		//	fPitch - in Degrees
		//	fFOV - in Degrees
		//
		float fRoll;
		ZEND int operator&( IBinSaver &f ) { f.Add(1,( NDb::SScriptCameraPlacement *)this); f.Add(2,&fRoll); return 0; }

		CCameraPlacement()
		{
			szName = "";
			vPosition = VNULL3;
			fYaw = DEF_YAW;
			fPitch = DEF_PITCH;
			fFOV = DEF_FOV;
			fRoll = DEF_ROLL;
		}
		CCameraPlacement( const CVec3 &_vAnchor, float _fYaw, float _fPitch, float _fFOV, float _fRoll )
		{
			szName = "";
			vPosition = _vAnchor;
			fYaw = _fYaw;
			fPitch = _fPitch;
			fFOV = _fFOV;
			fRoll = _fRoll;
		}

		// from anchor to eye
		const CVec3 GetDirection() const
		{
			return CVec3( sin(ToRadian(this->fYaw))*cos(ToRadian(this->fPitch)),
										-cos(ToRadian(this->fYaw))*cos(ToRadian(this->fPitch)), 
										sin(ToRadian(this->fPitch)) );
		}
		const float GetDistance() const
		{
			const CVec3 vDir = GetDirection();
			NI_VERIFY( fabs(vDir.z) > FP_EPSILON, "Unable to calculate camera distance", return 0.0f	);
			return fabs( vPosition.z / vDir.z );
		}
		const CVec3 GetAnchor() const { return vPosition - GetDirection()*GetDistance(); }

		const float& GetFOV() const { return fFOV; }
		//const CVec3 GetPos() const { return vPosition; }

		//const float GetYaw() const { return ToRadian(fYaw); }
		//const float GetPitch() const { return ToRadian(fPitch); }
		//const float GetRoll() const { return fRoll; }

		virtual bool ProcessEvent( const SGameMessage &msg ) { return false; }
		const CCameraPlacement &GetValue() const { return *this; }
	};
};

struct ICamera : public CObjectBase
{
	enum { tidTypeID = 0x3014EC01 };
	
	virtual void Update() = 0;
	//
	virtual void SetLimits( const NCamera::ELimitsType eLimitsType, const NCamera::SCameraLimits &limits ) = 0;
	virtual void GetLimits( const NCamera::ELimitsType eLimitsType, NCamera::SCameraLimits *pLimits ) const = 0;
	virtual void SetAnchorLimits( const CTRect<float> &vLimits ) = 0;
	// get/set camera's anchor
	virtual void SetAnchor( const CVec3 &_vAnchor ) = 0;
	virtual const CVec3 GetAnchor() const = 0;
	virtual const CVec3 GetPos() const = 0;
	virtual void SetYaw( float fYaw ) = 0;
	virtual float GetYaw() const = 0;
	virtual void SetPitch( float fPitch ) = 0;
	virtual float GetPitch() const = 0;
	virtual void SetFOV( float fFOV ) = 0;
	virtual float GetFOV() const = 0;
	//
	virtual void SetHandleType( bool bMayaHandleType ) = 0;
	virtual bool GetHandleType() = 0;
	// 
	virtual void SetPlacement( const float fDist, const float fPitch, const float fYaw ) = 0;
	virtual void SetDistance( const float fDist ) = 0;
	virtual float GetDistance() const = 0;
	virtual void GetPlacement( float *pfDist, float *pfPitch, float *pfYaw ) = 0;
	virtual void ResetToDefault() = 0;

	virtual void GetCameraState( CVec3 *pvPos, float *pfYaw, float *pfPitch, float *pfRoll, float *pfFOV )	const {};
	virtual void SetCameraState( const CVec3 &vPos, const float fYaw, const float fPitch, const float fRoll, const float fFOV ) {};

	// for script movies engine
	virtual void FinishMovie( bool bSmoothEnd = false ) = 0;

	virtual void ConformMouseMutator2ScriptMutator() = 0;

	virtual void SetScriptMutatorsHolder( CScriptMoviesMutatorHolder *_pScriptMutatorsHolder ) = 0;
	virtual CScriptMoviesMutatorHolder* GetScriptMutatorsHolder() = 0;
	//virtual void MovieSetTime( float fTime ) = 0;
	//virtual void MovieSetSpeed( float fSpeed ) = 0;
	//virtual void MoviePlay() = 0;
	//virtual void MovieStop() = 0;
	//virtual void MoviePause() = 0;
	//virtual void MovieSetLoopMode( bool bLoopPlayback ) = 0;

	// transformations
	virtual void SetPerspectiveTransform( float fWidth, float fHeight, float fNear, float fFar ) = 0;
	virtual void SetOrthographicTransform( float fWidth, float fHeight, float fNear, float fFar ) = 0;
	virtual void GetCameraParams( float *pfNear, float *pfFar, float *pfFOV ) const = 0;
	virtual void SwitchAutoPositioning( const bool bAllowAutoPositioning ) = 0;
	// we need a named locker, for example finishing a script can't to permit a moving, 
	// locked by a mouse selector and vice versa
	virtual void SwitchManualScrolling( const std::string &szLocker, const bool bManualOn ) = 0;

	// for listener positioning
	virtual const CVec3 GetListener() const { return GetPos(); } 
	// border scrolling
	virtual void ResetScrolling() = 0;
	virtual void SetScrollSpeedX( const float _fSpeed ) = 0;
	virtual void SetScrollSpeedY( const float _fSpeed ) = 0;
	// rotation
	virtual void SetYawSpeed( const float fSpeed ) = 0;
	// matrices
	virtual const SHMatrix& GetViewMatrix() const = 0;
	virtual const SHMatrix& GetProjectiveMatrix() const = 0;
	virtual class CTransformStack& GetTransform() = 0;
	// projective ray
	virtual void GetProjectiveRay( CVec3 *pvOrig, CVec3 *pvDir, const CVec2 &vScreenPos ) const = 0;
	virtual void GetProjectiveRayPoints( CVec3 *pvNear, CVec3 *pvFar, const CVec2 &vScreenPos ) const = 0;
	//
	virtual bool ProcessEvent( const SGameMessage &msg ) = 0;
	//
	virtual bool WasUpdated() = 0;
	// earthquakes
	virtual void AddEarthquake( const CVec3 &vPos, float fPower ) = 0;
	virtual void ClearEarthquakes() = 0;

	// for globe camera
	virtual void SetCameraPosition( const CQuat &quat ) = 0;
	virtual const CQuat GetCameraPosition() = 0;

};

SCENEB2_EXPORT ICamera* CreateCamera();
inline ICamera* Camera() { return Singleton<ICamera>(); }


