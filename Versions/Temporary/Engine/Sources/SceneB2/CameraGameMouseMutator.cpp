#include "stdafx.h"

#include "CameraGameMouseMutator.h"
#include "System/Commands.h"
#include "Scene.h"

#include "port/time.h"

#include <cstdint>

namespace NCamera
{
	static bool s_bGameCameraMousePitchInvert = false;
	static float s_fGameCameraMouseSensetivity = 0.5f;
	static bool s_bGameCameraMouseZoomInvert = false;
	static int s_nSmoothMouseZoom = 0;
	static float s_fSmoothZoomSpeed = 4.0f;
	static bool s_bGameFreeCamera = false;
	static bool s_bGameCameraTrackHeights = false;
	static bool s_bGameCameraSelectYawPitch = true;
	//
	static float s_fGameCameraMousePitchSensetivity = 1;
	static float s_fGameCameraKeyboardPitchSensetivity = 1;
	static float s_fGameCameraMouseYawSensetivity = 1;
	static float s_fGameCameraKeyboardYawSensetivity = 1;
	static float s_fGameCameraMouseForwardSensetivity = 1;
	static float s_fGameCameraKeyboardForwardSensetivity = 1;
	static float s_fGameCameraMouseStrafeSensetivity = 1;
	static float s_fGameCameraKeyboardStrafeSensetivity = 1;
	static float s_fGameCameraMouseZoomSensetivity = 1;
	static float s_fGameCameraKeyboardZoomSensetivity = 1;
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CCameraGameMouseMutator::CCameraGameMouseMutator()
	{
		SetTimeLastUpdate( GetCurrentTimeMilliseconds() );
	}
	void CCameraGameMouseMutator::SetDistance( float fDistance )
	{
		// Explicit placements and camera resets cancel any pending wheel movement.
		CCameraBasicMouseMutator::SetDistance( fDistance );
		bSmoothZoomActive = false;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	float CCameraGameMouseMutator::GetPitchDelta() 
	{ 
		return GetPitchMouseDelta() * s_fGameCameraMousePitchSensetivity + 
					GetPitchKeyboardDelta() * s_fGameCameraKeyboardPitchSensetivity; 
	}
	float CCameraGameMouseMutator::GetYawDelta() 
	{ 
		return GetYawMouseDelta() * s_fGameCameraMouseYawSensetivity + 
					GetYawKeyboardDelta() * s_fGameCameraKeyboardYawSensetivity; 
	}
	float CCameraGameMouseMutator::GetForwardDelta() 
	{ 
		return GetForwardMouseDelta() * s_fGameCameraMouseForwardSensetivity + 
					GetForwardKeyboardDelta() * s_fGameCameraKeyboardForwardSensetivity; 
	}
	float CCameraGameMouseMutator::GetStrafeDelta() 
	{ 
		return GetStrafeMouseDelta() * s_fGameCameraMouseStrafeSensetivity + 
					GetStrafeKeyboardDelta() * s_fGameCameraKeyboardStrafeSensetivity; 
	}
	float CCameraGameMouseMutator::GetZoomDistance( float fTimeDiff, float fRealSensetivity )
	{
		const float fMouseDelta = GetZoomMouseDelta() * s_fGameCameraMouseZoomSensetivity;
		const float fKeyboardDelta = GetZoomKeyboardDelta() * s_fGameCameraKeyboardZoomSensetivity;
		const float fZoomSensitivity = s_bGameCameraMouseZoomInvert ? -fRealSensetivity : fRealSensetivity;
		const float fSpeedCoeff = GetDistance() / GetDistanceLimit().fAve;
		if ( s_nSmoothMouseZoom == 0 )
		{
			bSmoothZoomActive = false;
			return IncrementValue( GetDistance(), (fMouseDelta + fKeyboardDelta) * fZoomSensitivity * fSpeedCoeff,
				GetDistanceLimit(), fTimeDiff, HasAutopositioning(), s_bGameFreeCamera );
		}

		if ( !bSmoothZoomActive )
			fZoomTargetDistance = GetDistance();
		if ( fMouseDelta != 0 )
		{
			// Accumulate wheel steps at the target distance, so smoothing speed does not
			// change the requested zoom. Clamp the target to avoid queued overscroll.
			const float fTargetSpeedCoeff = fZoomTargetDistance / GetDistanceLimit().fAve;
			fZoomTargetDistance = IncrementValue( fZoomTargetDistance, fMouseDelta * fZoomSensitivity * fTargetSpeedCoeff,
				GetDistanceLimit(), 0, false, s_bGameFreeCamera );
			bSmoothZoomActive = true;
		}

		// Keyboard zoom remains immediate; move the wheel target by the same input.
		const float fKeyboardZoom = fKeyboardDelta * fZoomSensitivity * fSpeedCoeff;
		if ( bSmoothZoomActive && fKeyboardZoom != 0 )
			fZoomTargetDistance = IncrementValue( fZoomTargetDistance, fKeyboardZoom,
				GetDistanceLimit(), 0, false, s_bGameFreeCamera );
		float fDistance = IncrementValue( GetDistance(), fKeyboardZoom, GetDistanceLimit(), fTimeDiff,
			HasAutopositioning() && !bSmoothZoomActive, s_bGameFreeCamera );
		if ( bSmoothZoomActive )
		{
			// Exponential easing uses real elapsed milliseconds, independent of game
			// speed and frame rate. At speed 1, about 95% of the step takes 250 ms.
			const float fSmoothSpeed = std::isfinite( s_fSmoothZoomSpeed ) && s_fSmoothZoomSpeed > 0
				? s_fSmoothZoomSpeed : 1.0f;
			const float fBlend = -std::expm1( -0.012f * fSmoothSpeed * fTimeDiff );
			fDistance += (fZoomTargetDistance - fDistance) * fBlend;
			if ( fabs( fZoomTargetDistance - fDistance ) < 0.001f )
			{
				fDistance = fZoomTargetDistance;
				bSmoothZoomActive = false;
			}
		}
		return fDistance;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CCameraGameMouseMutator::Recalc()
	{
		const uint32_t currTime = GetCurrentTimeMilliseconds();
		const float fTimeDiff = GetTimeLastUpdate() < currTime ? currTime - GetTimeLastUpdate() : 0;
		SetTimeLastUpdate( currTime );
		CVec3 vDir;

		const float fYawOld = fYaw;
		const float fPitchOld = fPitch;
		const CVec3 vPositionOld = vPosition;

		if ( !IsCameraLocked() )
		{
			const float fRealSensetivity = s_fGameCameraMouseSensetivity < 0.5f ? 
				s_fGameCameraMouseSensetivity + 0.5f : 2.0f * s_fGameCameraMouseSensetivity;
			// mouse camera control
			float fPitchDelta = GetPitchDelta() * fRealSensetivity;
			float fYawDelta = GetYawDelta() * fRealSensetivity;
			// select yaw or pitch
			if ( s_bGameCameraSelectYawPitch )
			{
				if ( fabs(fPitchDelta) > fabs(fYawDelta) )
					fYawDelta = 0;
				else
					fPitchDelta = 0;
			}
			//
			if ( s_bGameCameraMousePitchInvert )
				fPitchDelta = -fPitchDelta;
			// increment pitch
			const float fAddPitch = ToRadian( 180.0f );
			fPitch = IncrementValue( fPitch - fAddPitch, fPitchDelta, GetPitchLimit(), fTimeDiff, HasAutopositioning(), s_bGameFreeCamera ) + fAddPitch;
			fPitch = fmod( fPitch, FP_2PI );
			// increment yaw
			fYaw = IncrementValue( fYaw, fYawDelta, GetYawLimit(), fTimeDiff, HasAutopositioning(), s_bGameFreeCamera );
			fYaw = fmod( fYaw, FP_2PI );
			// DebugTrace( "fPitch = %2.3f, fYaw = %2.3f", fPitch, fYaw );
			// calculate camera's rotation
			const CQuat quat = CQuat( fYaw, V3_AXIS_Z ) * CQuat( fPitch, V3_AXIS_X );
			// acquire translation deltas
			const float fSpeedCoeff = GetDistance() / GetDistanceLimit().fAve;
			const float fFwdDelta = GetForwardDelta() * fSpeedCoeff;
			float fFwd = fFwdDelta == 0 ? GetScrollSpeedY() * fTimeDiff : fFwdDelta * 50;
			const float fStrafeDelta = GetStrafeDelta() * fSpeedCoeff;
			float fStrafe = fStrafeDelta == 0 ? GetScrollSpeedX() * fTimeDiff : fStrafeDelta * 50;

			fFwd *= fRealSensetivity;
			fStrafe *= fRealSensetivity;

			// calculate anchor translation
			CVec3 vAxisX, vAxisY;
			// forward
			CVec3 vGameAnchor = GetAnchor();
	//		quat.GetYAxis( &vAxisY );
	//		Normalize( &vAxisY );
	//		vAnchor += vAxisY * fFwd;
	//		// strafe
	//		quat.GetXAxis( &vAxisX );
	//		Normalize( &vAxisX );
	//		vAnchor += vAxisX * fStrafe;

			quat.GetZAxis( &vAxisY );
			vAxisY.z = 0;
			Normalize( &vAxisY );
			vGameAnchor += vAxisY * fFwd;
			// strafe
			quat.GetXAxis( &vAxisX );
			vAxisX.z = 0;
			Normalize( &vAxisX );
			vGameAnchor += vAxisX * fStrafe;
			// bound anchor
			if ( !GetAnchorLimit().IsEmpty() ) 
			{
				const CTRect<float> rcAnchorLimit = GetAnchorLimit();
				vGameAnchor.x = Clamp( vGameAnchor.x, rcAnchorLimit.minx, rcAnchorLimit.maxx );
				vGameAnchor.y = Clamp( vGameAnchor.y, rcAnchorLimit.miny, rcAnchorLimit.maxy );
			}
			SetAnchor( vGameAnchor );
			// increment zoom
			CCameraBasicMouseMutator::SetDistance( GetZoomDistance( fTimeDiff, fRealSensetivity ) );
			// calculate new camera position
			quat.GetZAxis( &vDir );
			vPosition = vGameAnchor - vDir*GetDistance();
			if ( s_bGameCameraTrackHeights && !NGlobal::GetVar( "temp.script_movie", false ) )
				vPosition.z += AI2Vis( Scene()->GetZ( Vis2AI( vPosition.x ), Vis2AI( vPosition.y ) ) );
		}
		else
		{
			bSmoothZoomActive = false;
			// increment yaw
			float fYawDelta = GetYawSpeed() * fTimeDiff * 0.0001f;
//			fYaw = IncrementValue( fYaw, fYawDelta, GetYawLimit(), fTimeDiff, HasAutopositioning(), s_bGameFreeCamera );
			fYaw += fYawDelta;
			fYaw = fmod( fYaw, FP_2PI );
			
			const CQuat quat = CQuat( fYaw, V3_AXIS_Z ) * CQuat( fPitch, V3_AXIS_X );
			quat.GetZAxis( &vDir );
			vPosition = GetAnchor() - vDir*GetDistance();
			if ( s_bGameCameraTrackHeights && !NGlobal::GetVar( "temp.script_movie", false ) )
				vPosition.z += AI2Vis( Scene()->GetZ( Vis2AI( vPosition.x ), Vis2AI( vPosition.y ) ) );
			
			ClearSliders();
		}

		SetWasUpdated( ( fabs( fYawOld - fYaw ) > EPS_VALUE ) || 
									( fabs( fPitchOld - fPitch ) > EPS_VALUE ) ||
									( fabs2( vPositionOld - vPosition ) > EPS_VALUE ) );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int CCameraGameMouseMutator::operator&( IBinSaver &saver )
	{
		saver.Add( 1, static_cast<CCameraBasicMouseMutator *>(this) );
		if ( saver.IsReading() )
		{
			bSmoothZoomActive = false;
			SetTimeLastUpdate( GetCurrentTimeMilliseconds() );
		}
		return 0;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
using namespace NCamera;
REGISTER_SAVELOAD_CLASS( SCENEB2, 0x101ACBC0, CCameraGameMouseMutator )

START_REGISTER( GameCameraMouseMutatorCommands )
REGISTER_VAR_EX( "game_camera_mouse_pitch_invert", NGlobal::VarBoolHandler, &s_bGameCameraMousePitchInvert, false, STORAGE_USER );
REGISTER_VAR_EX( "game_camera_mouse_zoom_invert", NGlobal::VarBoolHandler, &s_bGameCameraMouseZoomInvert, false, STORAGE_USER );
REGISTER_VAR_EX( "smooth_mouse_zoom", NGlobal::VarIntHandler, &s_nSmoothMouseZoom, 0, STORAGE_USER );
REGISTER_VAR_EX( "smooth_zoom_speed", NGlobal::VarFloatHandler, &s_fSmoothZoomSpeed, 2.0f, STORAGE_USER );
REGISTER_VAR_EX( "game_camera_mouse_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraMouseSensetivity, 0.5f, STORAGE_USER );
REGISTER_VAR_EX( "game_free_camera", NGlobal::VarBoolHandler, &s_bGameFreeCamera, false, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_track_heights", NGlobal::VarBoolHandler, &s_bGameCameraTrackHeights, false, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_select_yaw_pitch", NGlobal::VarBoolHandler, &s_bGameCameraSelectYawPitch, true, STORAGE_NONE );
//
REGISTER_VAR_EX( "game_camera_mouse_pitch_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraMousePitchSensetivity, 1, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_keyboard_pitch_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraKeyboardPitchSensetivity, 1, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_mouse_yaw_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraMouseYawSensetivity, 1, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_keyboard_yaw_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraKeyboardYawSensetivity, 1, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_mouse_forward_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraMouseForwardSensetivity, 1, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_keyboard_forward_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraKeyboardForwardSensetivity, 1, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_mouse_strafe_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraMouseStrafeSensetivity, 1, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_keyboard_strafe_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraKeyboardStrafeSensetivity, 1, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_mouse_zoom_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraMouseZoomSensetivity, 1, STORAGE_NONE );
REGISTER_VAR_EX( "game_camera_keyboard_zoom_sensetivity", NGlobal::VarFloatHandler, &s_fGameCameraKeyboardZoomSensetivity, 1, STORAGE_NONE );

FINISH_REGISTER


