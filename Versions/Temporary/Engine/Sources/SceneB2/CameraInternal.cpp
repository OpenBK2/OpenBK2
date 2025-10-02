#include "StdAfx.h"

#include <float.h>
#include "CameraInternal.h"
#include "../Misc/StrProc.h"
#include "../System/Commands.h"
#include "Cursor.h"
#include "../3Dmotor/Gfx.h"
#include "CameraGameMouseMutator.h"
#include "CameraMayaMouseMutator.h"
#include "CameraScriptMutators.h"
#include "../Main/GameTimer.h"
#include "Scene.h"

BASIC_REGISTER_CLASS( ICamera );
REGISTER_SAVELOAD_CLASS( 0x1006D300, CCamera )

static float s_fQuakeMinRadius = 8.0f * VIS_TILE_SIZE;
static float s_fQuakeMaxRadius = 200.0f * VIS_TILE_SIZE;
static float s_fQuakeAttenuation = 5.0f;
static float s_fQuakePeriod = 8.0f;
static float s_fQuakeDuration = 1000.0f;
static float s_fQuakePowerCoeff = 1.0f;
static float s_fValidateTime = 3000.0f;
static float s_fValidateDifference = 0.0001f;

CCamera::CCamera()
	: bMayaHandleType( false ),
	bIsMovieFinished( false ),
	bUseMovieMutator( false ),
	bIsPlayingFinal( false )
{
	SetupDefaultMouseMutator();
	//
	bWasUpdated = true;
	bWasUpdatedExternally = true;
	bPerspectiveTransform = true;
	//
	Identity( &matView );
	Identity( &matProj );
	transformStack.Clear();
	transformStack.Make( matProj );
	transformStack.SetCamera( matView );
	//
	AddObserver( "camera_mouse_rotate_start", &CCamera::MsgMouseRotation, true );
	AddObserver( "camera_mouse_rotate_finish", &CCamera::MsgMouseRotation, false );
	AddObserver( "camera_control_on", &CCamera::MsgMouseRotation, true );
	AddObserver( "camera_control_off", &CCamera::MsgMouseRotation, false );
}

void CCamera::SetHandleType(	bool _bMayaHandleType )
{
	bMayaHandleType = _bMayaHandleType;
	if ( bMayaHandleType )
	{
		pMouseMutator = new NCamera::CCameraMayaMouseMutator();
	}
	else
	{
		pMouseMutator = new NCamera::CCameraGameMouseMutator();
	}
}

void CCamera::SetupDefaultMouseMutator()
{
	SetHandleType( NGlobal::GetVar("camera_mode").GetString() == L"camera_mode_maya" );
}

void CCamera::MsgMouseRotation( const SGameMessage &msg, bool bBegin )
{
	if ( bBegin )
	{
//		Cursor()->Show( false );
		const CVec2 vPos = Cursor()->GetPos();
		Cursor()->SetBounds( vPos.x, vPos.y, vPos.x + 1, vPos.y + 1 );
	}
	else
	{
//		Cursor()->Show( true );
		const CVec2 vSize = NGfx::GetScreenRect();
		Cursor()->SetBounds( 0, 0, vSize.x, vSize.y );
	}
}

void CCamera::ResetToDefault()
{
	if ( pMouseMutator )
	{
		pMouseMutator->ResetToDefault();
		const CVec3 &vAnchor = pMouseMutator->GetAnchor();
		pMouseMutator->SetAnchor( CVec3(vAnchor.x, vAnchor.y, 0) );
	}
	bWasUpdatedExternally = true;
}

void CCamera::SetLimits( const NCamera::ELimitsType eLimitsType, const NCamera::SCameraLimits &limits )
{
	if ( pMouseMutator )
		pMouseMutator->SetLimits( eLimitsType, limits );

	bWasUpdatedExternally = true;
}

void CCamera::GetLimits( const NCamera::ELimitsType eLimitsType, NCamera::SCameraLimits *pLimits ) const
{
	if ( pMouseMutator )
		pMouseMutator->GetLimits( eLimitsType, pLimits );
}

void CCamera::SetAnchorLimits( const CTRect<float> &vLimits )
{
	if ( pMouseMutator )
		pMouseMutator->SetAnchorLimits( vLimits );
}

void CCamera::SwitchAutoPositioning( const bool bAllowAutoPositioning )
{ 
	if ( pMouseMutator )
		pMouseMutator->SwitchAutoPositioning( bAllowAutoPositioning );
}

void CCamera::SwitchManualScrolling( const string &szLocker, const bool bManualOn )
{
	if ( pMouseMutator )
		pMouseMutator->SwitchManualScrolling( szLocker, bManualOn );
}

const CVec3 CCamera::GetListener() const
{
	if ( IsScriptHolderActive() )
		return pScriptMutatorsHolder->GetPos();
	else if ( pMouseMutator )
		return pMouseMutator->GetListener();

	return VNULL3;
}

void CCamera::SetPerspectiveTransform( float _fWidth, float _fHeight, float _fNear, float _fFar )
{
	fScreenWidth = _fWidth;
	fScreenHeight = _fHeight;
	fNearClipPlane = _fNear;
	fFarClipPlane = _fFar;
	bPerspectiveTransform = true;
	bWasUpdatedExternally = true;
}

float CCamera::GetYaw() const
{
	if ( IsScriptHolderActive() )
		return pScriptMutatorsHolder->GetYaw();
	else if ( pMouseMutator )
		return pMouseMutator->GetYaw();
	else
		return DEF_YAW;
}

float CCamera::GetPitch() const
{
	if ( IsScriptHolderActive() )
		return pScriptMutatorsHolder->GetPitch();
	else if ( pMouseMutator )
		return pMouseMutator->GetPitch();
	else
		return DEF_PITCH;
}

float CCamera::GetFOV() const
{
	if ( IsScriptHolderActive() )
		return pScriptMutatorsHolder->GetFOV();
	else if ( pMouseMutator )
		return pMouseMutator->GetFOV();
	else
		return DEF_FOV;
}

void CCamera::SetYaw( float _fYaw )
{
	if ( pMouseMutator )
		pMouseMutator->SetYaw( _fYaw );
	bWasUpdatedExternally = true;
}

void CCamera::SetPitch( float _fPitch )
{
	if ( pMouseMutator )
		pMouseMutator->SetPitch( _fPitch );
	bWasUpdatedExternally = true;
}

void CCamera::SetFOV( float _fFOV )
{
	if ( pMouseMutator )
		pMouseMutator->SetFOV( _fFOV );
	bWasUpdatedExternally = true;
}

void CCamera::SetOrthographicTransform( float _fWidth, float _fHeight, float _fNear, float _fFar )
{
	bPerspectiveTransform = false;
	fScreenWidth = _fWidth;
	fScreenHeight = _fHeight;
	fNearClipPlane = _fNear;
	fFarClipPlane = _fFar;
	bWasUpdatedExternally = true;
}

void CCamera::GetProjectiveRay( CVec3 *pvOrig, CVec3 *pvDir, const CVec2 &vScreenPos ) const
{
	MakeProjectiveRay( pvDir, pvOrig, transformStack, CVec2(fScreenWidth, fScreenHeight), vScreenPos );
	if ( fabs2(*pvDir) < 0.000001 || _finite(pvDir->x) == 0 || _finite(pvDir->y) == 0 || _finite(pvDir->z) == 0 )
	{
		*pvDir = VNULL3;
		return;
	}
	Normalize( pvDir );
}

void CCamera::GetProjectiveRayPoints( CVec3 *pvNear, CVec3 *pvFar, const CVec2 &vScreenPos ) const
{
	CVec3 vOrig, vDir;
	GetProjectiveRay( &vOrig, &vDir, vScreenPos );
	if ( vDir == VNULL3 )
	{
		*pvNear = vOrig;
		*pvFar = vOrig;
	}
	else
	{
		*pvNear = vOrig + vDir * fNearClipPlane;
		*pvFar = vOrig + vDir * fFarClipPlane;
	}
}

void CCamera::Update()
{
	NCamera::CCameraPlacement placement;

	if ( IsScriptHolderActive() )
	{
		if ( bIsPlayingFinal && pScriptMutatorsHolder->IsFinished() )
		{
			pScriptMutatorsHolder->Stop();
			bIsPlayingFinal = false;
		}
		pScriptMutatorsHolder.Refresh();
		placement = pScriptMutatorsHolder->GetValue();
	}
	else if ( pMouseMutator )
	{
		pMouseMutator.Refresh();
		placement = pMouseMutator->GetValue();
	}
	else
		return;

	const float fYaw = placement.fYaw;
	const float fPitch = placement.fPitch;
	const float fFOV = placement.fFOV;
	const float fRoll = placement.fRoll + FP_PI;	// Mutators must hold fRoll in [-PI, PI).

	CVec3 vPosition = placement.vPosition;
	float fTimeDiff = timeLastUpdate;
	timeLastUpdate = GameTimer()->GetAbsTime();
	fTimeDiff = float( timeLastUpdate ) > fTimeDiff ? float( timeLastUpdate ) - fTimeDiff : 0;

	if ( !earthquakes.empty() ) 
	{
		float fVal = 0;
		for ( list<NCamera::SEarthQuake>::iterator it = earthquakes.begin(); it != earthquakes.end(); )
		{
			const float fTime = it->fTime / 1000.0f;
			fVal += exp( -fTime*it->fAttenuation ) * cos( s_fQuakePeriod*FP_2PI*fTime ) * it->fAmplitude;
			it->fTime += fTimeDiff;
			if ( it->fTime >= it->fDuration )
				it = earthquakes.erase( it );
			else
				++it;
		}
		vPosition.z += fVal;
	}

	const CQuat quat = CQuat( fYaw, V3_AXIS_Z ) * CQuat( fPitch, V3_AXIS_X );
	CVec3 vDir;
	quat.GetZAxis( &vDir );

	// build view matrix
	const int nPitch = Float2Int( floor(fPitch/FP_PI) );
	if ( nPitch%2 == 0 )
	{
		float fpTang = atan2( vDir.z, sqrt(sqr(vDir.x) + sqr(vDir.y)) );
		float fpRisk = atan2( vDir.y, vDir.x ) - FP_PI2;
		MakeMatrix( &matView, fpTang, fpRisk, fRoll, vPosition );
	}
	else
		MakeMatrix( &matView, vPosition, vDir );

	// build projection matrix
	transformStack.Clear();
	if ( bPerspectiveTransform )
		transformStack.MakeProjective( CVec2(fScreenWidth, fScreenHeight), fFOV, fNearClipPlane, fFarClipPlane );
	else
		transformStack.MakeParallel( fScreenWidth, fScreenHeight, fNearClipPlane, fFarClipPlane );

	matProj = transformStack.Get().forward;
	transformStack.SetCamera( GetViewMatrix() );

	bWasUpdated = bWasUpdatedExternally || pMouseMutator->WasUpdated();
	bWasUpdatedExternally = false;
}

void CCamera::SetAnchor( const CVec3 &_vAnchor )
{
	if ( pMouseMutator )
		pMouseMutator->SetAnchor( _vAnchor );
	bWasUpdatedExternally = true;
}

void CCamera::SetPlacement( const float _fDist, const float _fPitch, const float _fYaw )
{
	if ( pMouseMutator )
		pMouseMutator->SetPlacement( _fDist, ToRadian(270.0f - _fPitch), ToRadian(_fYaw) );
	bWasUpdatedExternally = true;
}

void CCamera::SetDistance( const float _fDist )
{
	if ( pMouseMutator )
		pMouseMutator->SetDistance( _fDist );
	bWasUpdatedExternally = true;
}

float CCamera::GetDistance() const
{
	if ( IsScriptHolderActive() )
		return pScriptMutatorsHolder->GetDistance();
	else if ( pMouseMutator )
		return pMouseMutator->GetDistance();
	else
		return 1.0f;
}

void CCamera::GetPlacement( float *pfDist, float *pfPitch, float *pfYaw )
{
	if ( IsScriptHolderActive() )
	{
		pScriptMutatorsHolder->GetPlacement( pfDist, pfPitch, pfYaw );
		(*pfPitch) = 270.0f - ToDegree( *pfPitch );
		(*pfYaw) = ToDegree( *pfYaw );
	}
	else if ( pMouseMutator )
	{
		pMouseMutator->GetPlacement( pfDist, pfPitch, pfYaw );
		(*pfPitch) = 270.0f - ToDegree( *pfPitch );
		(*pfYaw) = ToDegree( *pfYaw );
	}
}

void CCamera::GetCameraState( CVec3 *pvPos, float *pfYaw, float *pfPitch, float *pfRoll, float *pfFOV ) const
{
	if ( IsScriptHolderActive() )
	{
		NCamera::CCameraPlacement placement = pScriptMutatorsHolder->GetValue();

		(*pvPos) = placement.vPosition;
		(*pfYaw) = placement.fYaw;
		(*pfPitch) = placement.fPitch;
		(*pfRoll) = placement.fRoll;
		(*pfFOV) = placement.fFOV;
	}
	if ( pMouseMutator )
	{
		NCamera::CCameraPlacement *pPlacement = static_cast<NCamera::CCameraPlacement *>( pMouseMutator );
		if ( !pPlacement )
			return;

		(*pvPos) = pPlacement->vPosition;
		(*pfYaw) = pPlacement->fYaw;
		(*pfPitch) = pPlacement->fPitch;
		(*pfRoll) = pPlacement->fRoll;
		(*pfFOV) = pPlacement->fFOV;
	}
}

void CCamera::SetCameraState( const CVec3 &vPos, const float fYaw, const float fPitch, const float fRoll, const float fFOV )
{
	if ( pMouseMutator )
	{
		NCamera::CCameraPlacement *pPlacement = static_cast<NCamera::CCameraPlacement *>( pMouseMutator );
		if ( !pPlacement )
			return;

		pPlacement->vPosition = vPos;
		pPlacement->fYaw = fYaw;
		pPlacement->fPitch = fPitch;
		pPlacement->fRoll = fRoll;
		pPlacement->fFOV = fFOV;

		bWasUpdatedExternally = true;
	}
}

const CVec3 CCamera::GetAnchor() const
{
	if ( IsScriptHolderActive() )
		return pScriptMutatorsHolder->GetAnchor();
	else if ( pMouseMutator )
		return pMouseMutator->GetAnchor();
	else
		return VNULL3;
}

const CVec3 CCamera::GetPos() const
{
	if ( IsScriptHolderActive() )
		return pScriptMutatorsHolder->GetPos();
	else if ( pMouseMutator )
		return pMouseMutator->GetPos();
	else
		return VNULL3;
}


void CCamera::ResetScrolling()
{
	if ( pMouseMutator )
		pMouseMutator->ResetScrolling();
}

void CCamera::SetScrollSpeedX( const float _fSpeed )
{
	if ( pMouseMutator )
		pMouseMutator->SetScrollSpeedX( _fSpeed );
}

void CCamera::SetScrollSpeedY( const float _fSpeed )	
{
	if ( pMouseMutator )
		pMouseMutator->SetScrollSpeedY( _fSpeed );
}

void CCamera::SetYawSpeed( const float fSpeed )
{
	if ( pMouseMutator )
		pMouseMutator->SetYawSpeed( fSpeed );
}

bool CCamera::ProcessEvent( const SGameMessage &msg )
{
	if ( !CGMORegContainer::ProcessEvent(msg, this) )
	{
		if ( IsScriptHolderActive() )
		{
			NCamera::CCameraPlacement placement = pScriptMutatorsHolder->GetValue();
			if ( placement.ProcessEvent(msg) )
				return true;
		}
		else if ( pMouseMutator && pMouseMutator->ProcessEvent(msg) )
			return true;
	}
	return false;
}

void CCamera::AddEarthquake( const CVec3 &vPos, const float fPower )
{
	if ( fPower <= 0 ) 
		return;

	const CVec3 &vAnchor = GetAnchor();
	const float fDist = fabs( vPos.x - vAnchor.x, vPos.y - vAnchor.y );
	if ( fDist < s_fQuakeMaxRadius )
	{
		float fCoeff = fPower;
		if ( fDist > s_fQuakeMinRadius )
			fCoeff *= ( 1.0f - ( fDist - s_fQuakeMinRadius ) / ( s_fQuakeMaxRadius - s_fQuakeMinRadius ) );
		earthquakes.push_back( NCamera::SEarthQuake(fPower*fCoeff*s_fQuakePowerCoeff, s_fQuakeAttenuation, s_fQuakeDuration) );
	}
}

void CCamera::ClearEarthquakes()
{
	earthquakes.clear();
}

void CCamera::ConformMouseMutator2ScriptMutator()
{
	NI_VERIFY( pScriptMutatorsHolder, "Trying to conform to zero mutator holder", return )
	pScriptMutatorsHolder->Recalc();

	NCamera::CCameraPlacement placement = pScriptMutatorsHolder->GetValue();

	pMouseMutator->SetYaw( placement.fYaw );
	pMouseMutator->SetPitch( placement.fPitch );
	pMouseMutator->SetFOV( placement.fFOV );

	const float fFakePitch = ToRadian( 270.0f ) - placement.fPitch;
	const CVec3 vDir( sin(placement.fYaw)*cos(fFakePitch),
										-cos(placement.fYaw)*cos(fFakePitch), 
										sin(fFakePitch) );

	const float fDistance = fabs( placement.vPosition.z / vDir.z );
	pMouseMutator->SetDistance( fDistance );
	CVec3 vAnchor = placement.vPosition - vDir * fDistance;
	//AI2Vis( &vAnchor );
	pMouseMutator->SetAnchor( vAnchor );
	//pMouseMutator->SetWasUpdated( false );
}

bool CCamera::ValidateScriptPlacement( NCamera::CCameraPlacement *pPlacement ) const
{
	NCamera::SCameraLimits distLimits, pitchLimits, yawLimits;
	GetLimits( NCamera::CAMERA_LIMITS_YAW, &yawLimits );
	GetLimits( NCamera::CAMERA_LIMITS_PITCH, &pitchLimits );
	GetLimits( NCamera::CAMERA_LIMITS_DISTANCE, &distLimits );

	const float fRealYaw = pPlacement->fYaw;
	const float fRealPitch = pPlacement->fPitch;
	const float fRealFOV = pPlacement->GetFOV();
	const float fRealDistance = pPlacement->GetDistance();
	const CVec3 vRealPosition = pPlacement->vPosition;

	const float fValidYaw = fRealYaw;	// limits not used
	const float fValidPitch = Clamp( fRealPitch, pitchLimits.fMin, pitchLimits.fMax );
	const float fValidFOV = DEF_FOV;
	const float fValidDistance = Clamp( fRealDistance, distLimits.fMin, distLimits.fMax );
	NCamera::CCameraPlacement validPlacement;
	validPlacement.fYaw = fValidYaw;
	validPlacement.fPitch = fValidPitch;
	validPlacement.fFOV = fValidFOV;
	const CVec3 vValidAnchor = pPlacement->GetAnchor();
	const CVec3 vValidPosition = vValidAnchor + validPlacement.GetDirection()*fValidDistance;
	validPlacement.vPosition = vValidPosition;

	// rPlacement is already valid
	if ( fabs(fRealYaw - fValidYaw) < s_fValidateDifference &&
			 fabs(fRealPitch - fValidPitch) < s_fValidateDifference &&
			 fabs(fRealFOV - fValidFOV) < s_fValidateDifference &&
			 fabs(fRealDistance - fValidDistance) < s_fValidateDifference )
	{
		return false;
	}

	(*pPlacement) = validPlacement;
	return true;
}

void CCamera::FinishMovie( bool bSmoothEnd /* = false	*/)
{
	if ( !pScriptMutatorsHolder )
		return;

	pScriptMutatorsHolder.Refresh();
	NCamera::CCameraPlacement realPlacement = pScriptMutatorsHolder->GetValue();
	realPlacement.fYaw = ToDegree( realPlacement.fYaw );
	realPlacement.fPitch = 270.0f - ToDegree( realPlacement.fPitch );
	NCamera::CCameraPlacement finalPlacement = realPlacement;

	if ( ValidateScriptPlacement(&finalPlacement) )
	{
		NDb::SScriptMovies finalMovie;
		{
			finalMovie.scriptCameraPlacements.clear();

			NDb::SScriptCameraPlacement startCamPlacement = static_cast<NDb::SScriptCameraPlacement>( realPlacement );
			NDb::SScriptCameraPlacement finishCamPlacement = static_cast<NDb::SScriptCameraPlacement>( finalPlacement );

			Vis2AI( &startCamPlacement.vPosition );
			Vis2AI( &finishCamPlacement.vPosition );

			finalMovie.scriptCameraPlacements.push_back( startCamPlacement );
			finalMovie.scriptCameraPlacements.push_back( finishCamPlacement );
		}

		{
			finalMovie.scriptMovieSequences.clear();

			NDb::SScriptMovieSequence finalSequence;
			finalSequence.posKeys.clear();
			finalSequence.followKeys.clear();

			NDb::SScriptMovieKeyPos startPos;
			startPos.fStartTime = 0;
			startPos.nPositionIndex = 0;

			NDb::SScriptMovieKeyPos finishPos;
			finishPos.fStartTime = s_fValidateTime / 1000.0f;	// time to validate camera placement
			finishPos.nPositionIndex = 1;

			finalSequence.posKeys.push_back( startPos );
			finalSequence.posKeys.push_back( finishPos );

			finalMovie.scriptMovieSequences.push_back( finalSequence );
		}

		CCSTime *pTimer = Scene()->GetGameTimer();

		CScriptMoviesMutatorHolder *pMoviesHolder = new CScriptMoviesMutatorHolder( finalMovie, 0, pTimer );
		pMoviesHolder->SetLoopMode( false );
		pMoviesHolder->Play();
		SetScriptMutatorsHolder( pMoviesHolder );

		SetPlacement( finalPlacement.GetDistance(), finalPlacement.fPitch, finalPlacement.fYaw );
		SetAnchor( finalPlacement.GetAnchor() );
		SetFOV( finalPlacement.GetFOV() );
		SetRoll( 0.0f );

		bIsPlayingFinal = true;
	}
	else
	{
		SetPlacement( finalPlacement.GetDistance(), finalPlacement.fPitch, finalPlacement.fYaw );
		SetAnchor( finalPlacement.GetAnchor() );
		SetFOV( finalPlacement.GetFOV() );
		SetRoll( 0.0f );

		if ( CScriptMoviesMutatorHolder *pMoviesHolder = GetScriptMutatorsHolder() )
		{
			pMoviesHolder->Stop();
		}
	}

	return;
}

int CCamera::operator&( IBinSaver &saver )
{
	saver.Add( 7, &matView );
	saver.Add( 14, &matProj );
	saver.Add( 16, &fScreenWidth );
	saver.Add( 17, &fScreenHeight );
	saver.Add( 18, &fNearClipPlane );
	saver.Add( 19, &fFarClipPlane );
	//saver.Add( 20, &pScriptMutator );
	saver.Add( 22, &pMouseMutator );
	saver.Add( 23, &earthquakes );
	saver.Add( 24, &timeLastUpdate );
	saver.Add( 25, &pScriptMutatorsHolder );
	saver.Add( 26, &bUseMovieMutator );
	saver.Add( 27, &bIsMovieFinished );
	saver.Add( 28, &bIsPlayingFinal );
	if ( saver.IsReading() )
	{
		bWasUpdated = true;
		bWasUpdatedExternally = true;
		if ( pMouseMutator == 0 )
			SetupDefaultMouseMutator();
		bPerspectiveTransform = true;
	}
	return 0;
}

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

ICamera* CreateCamera()
{
	return new CCamera();
}

// set_camera_pitch_limits min max ave asp msp
static void SetCameraLimits( NCamera::ELimitsType eType, const vector<wstring> &szParams )
{
	if ( szParams.size() != 5 ) 
	{
		csSystem << "this call requires 5 params: min max ave auto_speed manual_speed" << endl;
		return;
	}
	//
	NCamera::SCameraLimits limits = NCamera::SCameraLimits( 
		NStr::ToFloat( NStr::ToMBCS(szParams[0]) ), 
		NStr::ToFloat( NStr::ToMBCS(szParams[1]) ),
		NStr::ToFloat( NStr::ToMBCS(szParams[2]) ), 
		NStr::ToFloat( NStr::ToMBCS(szParams[3]) ),
		NStr::ToFloat( NStr::ToMBCS(szParams[4]) ), false );
	//
	Camera()->SetLimits( eType, limits );
}

static void CmdSetCameraPitchLimits( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	SetCameraLimits( NCamera::CAMERA_LIMITS_PITCH, paramsSet );
}

static void CmdSetCameraYawLimits( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	SetCameraLimits( NCamera::CAMERA_LIMITS_YAW, paramsSet );
}

static void CmdSetCameraDistanceLimits( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	SetCameraLimits( NCamera::CAMERA_LIMITS_DISTANCE, paramsSet );
}

static void CmdResetCameraToDefault( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	Camera()->ResetToDefault();
}

START_REGISTER( CameraCommands )

REGISTER_CMD( "camera_pitch_limits", CmdSetCameraPitchLimits )
REGISTER_CMD( "camera_yaw_limits", CmdSetCameraYawLimits )
REGISTER_CMD( "camera_distance_limits", CmdSetCameraDistanceLimits )
REGISTER_CMD( "camera_reset", CmdResetCameraToDefault )

REGISTER_VAR_EX( "camera_quake_min_radius", NGlobal::VarFloatHandler, &s_fQuakeMinRadius, 8.0f*VIS_TILE_SIZE, STORAGE_NONE );
REGISTER_VAR_EX( "camera_quake_max_radius", NGlobal::VarFloatHandler, &s_fQuakeMaxRadius, 200.0f*VIS_TILE_SIZE, STORAGE_NONE );
REGISTER_VAR_EX( "camera_quake_attenuation", NGlobal::VarFloatHandler, &s_fQuakeAttenuation, 5.0f, STORAGE_NONE );
REGISTER_VAR_EX( "camera_quake_period", NGlobal::VarFloatHandler, &s_fQuakePeriod, 8.0f, STORAGE_NONE );
REGISTER_VAR_EX( "camera_quake_duration", NGlobal::VarFloatHandler, &s_fQuakeDuration, 1000.0f, STORAGE_NONE );
REGISTER_VAR_EX( "camera_quake_power_coeff", NGlobal::VarFloatHandler, &s_fQuakePowerCoeff, 1.0f, STORAGE_NONE );
REGISTER_VAR_EX( "camera_validate_time", NGlobal::VarFloatHandler, &s_fValidateTime, 1000.0f, STORAGE_NONE );
REGISTER_VAR_EX( "camera_validate_diff", NGlobal::VarFloatHandler, &s_fValidateDifference, 0.0001f, STORAGE_NONE );

FINISH_REGISTER


