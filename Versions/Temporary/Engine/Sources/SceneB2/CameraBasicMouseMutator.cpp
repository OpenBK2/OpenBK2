#include "StdAfx.h"

#include "CameraBasicMouseMutator.h"

namespace NCamera
{

static inline float CyclicClamp( const float fValue, const float fMin, const float fMax )
{
	if ( fValue < fMin )
		return fMax + fmod( fValue - fMin, fMax - fMin );
	else
		return fMin + fmod( fValue - fMin, fMax - fMin );
} 

float IncrementValue( float fCurrValue, float fAdd, const SLimit &limit, float fTimeDiff, const bool bAutoPositioning, bool bFreeCamera )
{
	fTimeDiff *= 0.001f;
	float fResult = 0;
	if ( fAdd != 0 ) // manual
	{
		fResult = fCurrValue + fAdd * limit.fManualSpeed;
		if ( limit.fMax > limit.fMin && !bFreeCamera ) 
			return limit.bCyclic ? CyclicClamp( fResult, limit.fMin, limit.fMax ) : Clamp( fResult, limit.fMin, limit.fMax );
		else
			return fResult;
	}
	else if ( bAutoPositioning && !bFreeCamera && limit.fMax > limit.fMin ) // automatic
	{
		if ( fCurrValue > limit.fAve ) 
		{
			fResult = fCurrValue - fTimeDiff * limit.fAutoSpeed;
			return limit.bCyclic ? CyclicClamp( fResult, limit.fAve, limit.fMax ) : Clamp( fResult, limit.fAve, limit.fMax );
		}
		else
		{
			fResult = fCurrValue + fTimeDiff * limit.fAutoSpeed;
			return limit.bCyclic ? CyclicClamp( fResult, limit.fMin, limit.fAve ) : Clamp( fResult, limit.fMin, limit.fAve );
		}
	}
	else
		return fCurrValue;
}

CCameraBasicMouseMutator::CCameraBasicMouseMutator()
	:	vAnchor( VNULL3 ),
	sliderFwd( "camera_forward" ),
	sliderStrafe( "camera_strafe" ),
	sliderZoom( "camera_zoom" ),
	sliderPitch( "camera_pitch" ),
	sliderYaw( "camera_yaw" ),
	sliderMouseForward( "camera_mouse_forward" ),
	sliderMouseStrafe( "camera_mouse_strafe" ),
	sliderMousePitch( "camera_mouse_pitch" ),
	sliderMouseYaw( "camera_mouse_yaw" ),
	sliderMouseZoom( "camera_mouse_zoom" ),
	fScrollSpeedX( 0 ),
	fScrollSpeedY( 0 ),
	fYawSpeed( 0.0f )
{
	//
	rcAnchorLimit.SetEmpty();

	distanceLimit.SetLimit( 10, 1000, 170, 0, 50, false );
	pitchLimit.SetLimit( ToRadian(180.0f + 90.0f - 89.0f), ToRadian(180.0f + 90.0f - 89.0f), ToRadian(180.0f + 90.0f - 50.0f), 0, ToRadian(180.0f + 20.0f), false );
	yawLimit.SetLimit( ToRadian(-135.0f), ToRadian(-135.0f), ToRadian(45.0f), 0, ToRadian(30.0f), false );
	//
	ResetToDefault();
	//
	bAutoPositioning = NGlobal::GetVar( "disable_auto_camera", 0 ) == 0;
	bWasUpdated = true;
}

void CCameraBasicMouseMutator::ResetToDefault()
{
	fDistance = distanceLimit.fAve;
	fPitch = pitchLimit.fAve + ToRadian( 180.0f );
	fYaw = yawLimit.fAve;
}

void CCameraBasicMouseMutator::SetLimits( const NCamera::ELimitsType eLimitsType, const NCamera::SCameraLimits &limits )
{
	switch ( eLimitsType ) 
	{
		case NCamera::CAMERA_LIMITS_YAW:
		{
			yawLimit.SetLimit( ToRadian(limits.fMin),
												ToRadian(limits.fMax),
												ToRadian(limits.fAve),
												ToRadian(limits.fAutoSpeed),
												ToRadian(limits.fManualSpeed),
												limits.bCyclic );
			fYaw = yawLimit.fAve;
			break;
		}
		//
		case NCamera::CAMERA_LIMITS_PITCH:
		{
			pitchLimit.SetLimit( ToRadian(90.0f - limits.fMax),
													ToRadian(90.0f - limits.fMin),
													ToRadian(90.0f - limits.fAve),
													ToRadian(limits.fAutoSpeed),
													ToRadian(limits.fManualSpeed),
													limits.bCyclic );
			fPitch = pitchLimit.fAve + ToRadian( 180.0f );
			break;
		}
		//
		case NCamera::CAMERA_LIMITS_DISTANCE:
		{
			distanceLimit.SetLimit( limits.fMin,
															limits.fMax,
															limits.fAve,
															limits.fAutoSpeed,
															limits.fManualSpeed,
															limits.bCyclic );
			fDistance = distanceLimit.fAve;
			break;
		}
		//
		default:
		{
			NI_ASSERT( false, StrFmt("Unknown limits type %d", eLimitsType) );
		}
	}
}

void CCameraBasicMouseMutator::GetLimits( const NCamera::ELimitsType eLimitsType, NCamera::SCameraLimits *pLimits )
{
	switch ( eLimitsType )
	{
		case NCamera::CAMERA_LIMITS_YAW:
		{
			pLimits->fMin = ToDegree( yawLimit.fMin );
			pLimits->fMax = ToDegree( yawLimit.fMax );
			pLimits->fAve = ToDegree( yawLimit.fAve );
			pLimits->fAutoSpeed = ToDegree( yawLimit.fAutoSpeed );
			pLimits->fManualSpeed = ToDegree( yawLimit.fManualSpeed );
			pLimits->bCyclic = yawLimit.bCyclic;
			//
			break;
		}
		//
		case NCamera::CAMERA_LIMITS_PITCH:
		{
			pLimits->fMin = 90.0f - ToDegree( pitchLimit.fMax );
			pLimits->fMax = 90.0f - ToDegree( pitchLimit.fMin );
			pLimits->fAve = 90.0f - ToDegree( pitchLimit.fAve );
			pLimits->fAutoSpeed = ToDegree( pitchLimit.fAutoSpeed );
			pLimits->fManualSpeed = ToDegree( pitchLimit.fManualSpeed );
			pLimits->bCyclic = pitchLimit.bCyclic;
			//
			break;
		}
		//
		case NCamera::CAMERA_LIMITS_DISTANCE:
		{
			pLimits->fMin = distanceLimit.fMin;
			pLimits->fMax = distanceLimit.fMax;
			pLimits->fAve = distanceLimit.fAve;
			pLimits->fAutoSpeed = distanceLimit.fAutoSpeed;
			pLimits->fManualSpeed = distanceLimit.fManualSpeed;
			pLimits->bCyclic = distanceLimit.bCyclic;
			//
			break;
		}
	}
}

void CCameraBasicMouseMutator::SetAnchorLimits( const CTRect<float> &vLimits )
{
	rcAnchorLimit = vLimits;
}

void CCameraBasicMouseMutator::SwitchAutoPositioning( const bool bAllowAutoPositioning )
{ 
	if ( NGlobal::GetVar( "disable_auto_camera", 0 ) == 0 )
		bAutoPositioning = bAllowAutoPositioning; 
}

void CCameraBasicMouseMutator::SwitchManualScrolling( const string &szLocker, const bool bManualOn )
{
	if ( bManualOn )
		manualLockers.erase( szLocker );
	else
		manualLockers[szLocker] = true;
}

const CVec3 CCameraBasicMouseMutator::GetListener() const
{
	const float fScreenWidth = fDistance * tan( ToRadian(fFOV/2.0f) );
	const float fHeight = fDistance * atan( ToRadian(fFOV/2.0f) );

	CVec3 vDir( vAnchor - vPosition );
	vDir.z = 0;
	Normalize( &vDir );

	CVec3 vPos( vAnchor - vDir * fScreenWidth * 0.5 );
	vPos.z = fHeight / 2;
	return vPos;
}

bool CCameraBasicMouseMutator::ProcessEvent( const SGameMessage &msg )
{
	if ( CGMORegContainer::ProcessEvent( msg, this ) == false )
	{
		sliderFwd.ProcessEvent( msg );
		sliderStrafe.ProcessEvent( msg );
		sliderZoom.ProcessEvent( msg );
		sliderPitch.ProcessEvent( msg );
		sliderYaw.ProcessEvent( msg );
		sliderMousePitch.ProcessEvent( msg );
		sliderMouseYaw.ProcessEvent( msg );
		sliderMouseZoom.ProcessEvent( msg );
		sliderMouseForward.ProcessEvent( msg );
		sliderMouseStrafe.ProcessEvent( msg );
	}
	return false;
}

void CCameraBasicMouseMutator::ClearSliders()
{
	sliderMousePitch.GetDelta();
	sliderMouseYaw.GetDelta();
	sliderPitch.GetDelta();
	sliderYaw.GetDelta();
	sliderFwd.GetDelta();
	sliderMouseForward.GetDelta();
	sliderStrafe.GetDelta();
	sliderMouseStrafe.GetDelta();
	sliderMouseZoom.GetDelta();
	sliderZoom.GetDelta();
}

int CCameraBasicMouseMutator::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CCameraPlacement *>(this) );
	saver.Add( 2, &vAnchor );
	saver.Add( 3, &fDistance );
	saver.Add( 4, &bAutoPositioning );
	saver.Add( 5, &manualLockers );
	saver.Add( 6, &rcAnchorLimit );
	saver.Add( 7, &distanceLimit );
	saver.Add( 8, &yawLimit );
	saver.Add( 9, &pitchLimit );
	saver.Add( 10, &fYawSpeed );
	//
	if ( saver.IsReading() )
	{
		fScrollSpeedX = 0;
		fScrollSpeedY = 0;
		bWasUpdated = true;
		bWasUpdatedExternally = true;
		timeLastUpdate = 0;
	}
	//
	return 0;
}

}


