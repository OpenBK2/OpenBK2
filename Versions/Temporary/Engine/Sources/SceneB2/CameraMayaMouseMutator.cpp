#include "StdAfx.h"

#include "CameraMayaMouseMutator.h"
#include "../System/Commands.h"

namespace NCamera
{
	static float s_fMayaCameraScrollXSensetivity = 1;
	static float s_fMayaCameraScrollYSensetivity = 1;
	static float s_fMayaCameraPitchSensetivity = 1;
	static float s_fMayaCameraYawSensetivity = 1;
	static float s_fMayaCameraZoomSensetivity = 1;
	static bool s_bMayaFreeCamera = false;
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	float CCameraMayaMouseMutator::GetPitchDelta() { return GetPitchMouseDelta() + GetPitchKeyboardDelta(); }
	float CCameraMayaMouseMutator::GetYawDelta() { return GetYawMouseDelta() + GetYawKeyboardDelta(); }
	float CCameraMayaMouseMutator::GetForwardDelta() { return GetForwardMouseDelta() + GetForwardKeyboardDelta(); }
	float CCameraMayaMouseMutator::GetStrafeDelta() { return GetStrafeMouseDelta() + GetStrafeKeyboardDelta(); }
	float CCameraMayaMouseMutator::GetZoomDelta() { return GetZoomMouseDelta() + GetZoomKeyboardDelta(); }
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CCameraMayaMouseMutator::Recalc()
	{
		const DWORD currTime = GetTickCount();
		const float fTimeDiff = GetTimeLastUpdate() < currTime ? currTime - GetTimeLastUpdate() : 0;
		SetTimeLastUpdate( currTime );
		CVec3 vDir;

		const float fYawOld = fYaw;
		const float fPitchOld = fPitch;
		const CVec3 vPositionOld = vPosition;

		if ( !IsCameraLocked() )
		{
			// mouse camera control
			const float fPitchDelta = GetPitchDelta() * s_fMayaCameraPitchSensetivity;
			const float fYawDelta = GetYawDelta() * s_fMayaCameraYawSensetivity;
			// increment pitch
			const float fAddPitch = ToRadian( 180.0f );
			fPitch = IncrementValue( fPitch - fAddPitch, fPitchDelta, GetPitchLimit(), fTimeDiff, HasAutopositioning(), s_bMayaFreeCamera ) + fAddPitch;
			fPitch = fmod( fPitch, FP_2PI );
			// increment yaw
			fYaw = IncrementValue( fYaw, fYawDelta, GetYawLimit(), fTimeDiff, HasAutopositioning(), s_bMayaFreeCamera );
			fYaw = fmod( fYaw, FP_2PI );
			// DebugTrace( "fPitch = %2.3f, fYaw = %2.3f", fPitch, fYaw );
			// calculate camera's rotation
			const CQuat quat = CQuat( fYaw, V3_AXIS_Z ) * CQuat( fPitch, V3_AXIS_X );
			// acquire translation deltas
			const float fSpeedCoeff = GetDistance() / GetDistanceLimit().fAve;
			const float fFwdDelta = GetForwardDelta() * fSpeedCoeff * s_fMayaCameraScrollYSensetivity;
			const float fFwd = fFwdDelta == 0 ? GetScrollSpeedY() * fTimeDiff : fFwdDelta * 50;
			const float fStrafeDelta = GetStrafeDelta() * fSpeedCoeff * s_fMayaCameraScrollXSensetivity;
			const float fStrafe = fStrafeDelta == 0 ? GetScrollSpeedX() * fTimeDiff : fStrafeDelta * 50;
			// calculate anchor translation
			CVec3 vAxisX, vAxisY;
			// forward
			CVec3 vAnchor = GetAnchor();
			quat.GetYAxis( &vAxisY );
			Normalize( &vAxisY );
			vAnchor += vAxisY * fFwd;
			// strafe
			quat.GetXAxis( &vAxisX );
			Normalize( &vAxisX );
			vAnchor += vAxisX * fStrafe;

			SetAnchor( vAnchor );
			// increment zoom
			const float fZoomDelta = GetZoomDelta() * s_fMayaCameraZoomSensetivity;
			SetDistance( IncrementValue( GetDistance(), fZoomDelta*fSpeedCoeff, GetDistanceLimit(), fTimeDiff, HasAutopositioning(), s_bMayaFreeCamera ) );
			// calculate new camera position
			quat.GetZAxis( &vDir );
			vPosition = vAnchor - vDir*GetDistance();
		}
		else
		{
			const CQuat quat = CQuat( fYaw, V3_AXIS_Z ) * CQuat( fPitch, V3_AXIS_X );
			quat.GetZAxis( &vDir );
			vPosition = GetAnchor() - vDir*GetDistance();
			
			ClearSliders();
		}

		SetWasUpdated( ( fabs( fYawOld - fYaw ) > EPS_VALUE ) || 
			( fabs( fPitchOld - fPitch ) > EPS_VALUE ) ||
			( fabs2( vPositionOld - vPosition ) > EPS_VALUE ) );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int CCameraMayaMouseMutator::operator&( IBinSaver &saver )
	{
		saver.Add( 1, static_cast<CCameraBasicMouseMutator *>(this) );
		return 0;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
using namespace NCamera;
REGISTER_SAVELOAD_CLASS( 0x101ACBC1, CCameraMayaMouseMutator )

START_REGISTER( MayaCameraMouseMutatorCommands )
REGISTER_VAR_EX( "maya_camera_scroll_x_sensetivity", NGlobal::VarFloatHandler, &s_fMayaCameraScrollXSensetivity, 1, STORAGE_NONE )
REGISTER_VAR_EX( "maya_camera_scroll_y_sensetivity", NGlobal::VarFloatHandler, &s_fMayaCameraScrollYSensetivity, 1, STORAGE_NONE )
REGISTER_VAR_EX( "maya_camera_pitch_sensetivity", NGlobal::VarFloatHandler, &s_fMayaCameraPitchSensetivity, 1, STORAGE_NONE )
REGISTER_VAR_EX( "maya_camera_yaw_sensetivity", NGlobal::VarFloatHandler, &s_fMayaCameraYawSensetivity, 1, STORAGE_NONE )
REGISTER_VAR_EX( "maya_camera_zoom_sensetivity", NGlobal::VarFloatHandler, &s_fMayaCameraZoomSensetivity, 1, STORAGE_NONE )
REGISTER_VAR_EX( "maya_free_camera", NGlobal::VarBoolHandler, &s_bMayaFreeCamera, false, STORAGE_NONE )
FINISH_REGISTER


