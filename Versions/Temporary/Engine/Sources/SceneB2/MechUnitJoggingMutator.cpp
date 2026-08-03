#include "stdafx.h"

#include "vendor/granny/include/granny.h"
#include "MechUnitJoggingMutator.h"
#include "Main/GameTimer.h"
#include "System/Commands.h"

static bool s_bJogTweakMode = false;
static float s_fJogPeriod1 = FP_2PI;
static float s_fJogPhaze1 = 0;
static float s_fJogAmp1 = 1;
static float s_fJogPeriod2 = FP_2PI;
static float s_fJogPhaze2 = 0;
static float s_fJogAmp2 = 1;

static float GetJoggingWaveValue( const float fDeltaTime, const float fPeriod, const float fAmp, const float fPhaze )
{
	// Each wave is optional; a disabled wave must not suppress the other one.
	if ( fPeriod == 0.0f )
		return 0.0f;

	const float fAngle = fmod( fDeltaTime * FP_2PI / fPeriod, FP_2PI ) + fPhaze;
	return fAmp * NMath::Cos( fAngle );
}

float CMechUnitJoggingMutator::SJoggingParams2::GetValue( const NTimer::STime nDeltaTime ) const
{
	// Jogging periods are authored in seconds, while the game timer uses milliseconds.
	const float fDeltaTime = (float)nDeltaTime / 1000.0f;
	if ( s_bJogTweakMode )
		return GetJoggingWaveValue( fDeltaTime, s_fJogPeriod1, s_fJogAmp1, s_fJogPhaze1 ) +
			GetJoggingWaveValue( fDeltaTime, s_fJogPeriod2, s_fJogAmp2, s_fJogPhaze2 );
	else
		return GetJoggingWaveValue( fDeltaTime, fPeriod1, fAmp1, fPhaze1 ) +
			GetJoggingWaveValue( fDeltaTime, fPeriod2, fAmp2, fPhaze2 );
}

void CMechUnitJoggingMutator::Setup( const int _nBasisBoneIndex, const SJoggingParams &_joggingX, const SJoggingParams &_joggingY )
{
	joggingX.fAmp1		= _joggingX.fAmp1;
	joggingX.fAmp2		= _joggingX.fAmp2;
	joggingX.fPeriod1 = _joggingX.fPeriod1;
	joggingX.fPeriod2 = _joggingX.fPeriod2;
	joggingX.fPhaze1	= _joggingX.fPhaze1;
	joggingX.fPhaze2	= _joggingX.fPhaze2;

	joggingY.fAmp1		= _joggingY.fAmp1;
	joggingY.fAmp2		= _joggingY.fAmp2;
	joggingY.fPeriod1 = _joggingY.fPeriod1;
	joggingY.fPeriod2 = _joggingY.fPeriod2;
	joggingY.fPhaze1	= _joggingY.fPhaze1;
	joggingY.fPhaze2	= _joggingY.fPhaze2;

	nStartTime = GameTimer()->GetSegmentTime();
	nStopTime = GameTimer()->GetSegmentTime();

	nBasisBoneIndex = _nBasisBoneIndex;
}

bool CMechUnitJoggingMutator::NeedUpdate()
{
	IGameTimer *pTimer = GameTimer();
	return !bStopped && pTimer->GetPauseType() == -1 && pTimer->GetSegmentTime() > nStartTime ;
}

void CMechUnitJoggingMutator::MutateSkeletonPose( granny_local_pose *pPose )
{
	const NTimer::STime nDeltaTime = bStopped ? nStopTime - nStartTime : GameTimer()->GetSegmentTime() - nStartTime;

	const float fValueX = joggingX.GetValue( nDeltaTime );
	const float fValueY = s_bJogTweakMode ? 0 : joggingY.GetValue( nDeltaTime );
//	const float fValueZ = joggingZ.GetValue( nDeltaTime );

	CQuat qResult( ToRadian(fValueX), V3_AXIS_X );
	const CQuat qY( ToRadian(fValueY), V3_AXIS_Y );
	qResult *= qY;

	granny_transform *pRootTransform = GrannyGetLocalPoseTransform( pPose, nBasisBoneIndex );
	CQuat qBoneOrientation;
	memcpy( &qBoneOrientation, &(pRootTransform->Orientation), sizeof( float ) * 4 );
	qBoneOrientation *= qResult;
	memcpy( &(pRootTransform->Orientation), &qBoneOrientation, sizeof( float ) * 4 );
	CVec3 vOldBonePos, vBonePos;
	memcpy( &vOldBonePos, &(pRootTransform->Position), sizeof( float ) * 3 );
	SHMatrix mTrans( qResult );
	mTrans.RotateVector( &vBonePos, vOldBonePos );
	memcpy( &(pRootTransform->Position), &vBonePos, sizeof( float ) * 3 );
	pRootTransform->Flags |= GrannyHasPosition;
	pRootTransform->Flags |= GrannyHasOrientation;
}

void CMechUnitJoggingMutator::Play()
{
	if ( bStopped )
	{
		nStartTime += (GameTimer()->GetSegmentTime()-nStopTime);
		bStopped = false;
	}
}

void CMechUnitJoggingMutator::Stop()
{
	if ( !bStopped )
	{
		nStopTime = GameTimer()->GetSegmentTime();
		bStopped = true;
	}
}

int CMechUnitJoggingMutator::operator&( IBinSaver &saver )
{	
	saver.Add( 1, &joggingX );
	saver.Add( 2, &joggingY );
	saver.Add( 3, &nStartTime );
	saver.Add( 4, &nStopTime );
	saver.Add( 5, &nBasisBoneIndex );
	saver.Add( 6, &bStopped );

	return 0;
}

START_REGISTER( MechUnitJoggingMutator )

REGISTER_VAR_EX( "jog_tweak_mode", NGlobal::VarBoolHandler, &s_bJogTweakMode, false, STORAGE_NONE );
REGISTER_VAR_EX( "jog_period1", NGlobal::VarFloatHandler, &s_fJogPeriod1, FP_2PI, STORAGE_NONE );
REGISTER_VAR_EX( "jog_phaze1", NGlobal::VarFloatHandler, &s_fJogPhaze1, 0, STORAGE_NONE );
REGISTER_VAR_EX( "jog_amp1", NGlobal::VarFloatHandler, &s_fJogAmp1, 1, STORAGE_NONE );
REGISTER_VAR_EX( "jog_period2", NGlobal::VarFloatHandler, &s_fJogPeriod2, FP_2PI, STORAGE_NONE );
REGISTER_VAR_EX( "jog_phaze2", NGlobal::VarFloatHandler, &s_fJogPhaze2, 0, STORAGE_NONE );
REGISTER_VAR_EX( "jog_amp2", NGlobal::VarFloatHandler, &s_fJogAmp2, 1, STORAGE_NONE );

FINISH_REGISTER


REGISTER_SAVELOAD_CLASS( 0x15095B00, CMechUnitJoggingMutator )


