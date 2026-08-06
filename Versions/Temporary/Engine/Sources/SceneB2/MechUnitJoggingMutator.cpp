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
float CMechUnitJoggingMutator::SJoggingParams2::GetValue( const NTimer::STime nDeltaTime ) const
{
	if ( s_bJogTweakMode )
	{
		if ( (s_fJogPeriod1 == 0) || (s_fJogPeriod2 == 0) )
			return 0;
		const float fAngle1 = fmod( (float)nDeltaTime*FP_2PI/s_fJogPeriod1, FP_2PI ) + s_fJogPhaze1;
		const float fAngle2 = fmod( (float)nDeltaTime*FP_2PI/s_fJogPeriod2, FP_2PI ) + s_fJogPhaze2;
		return s_fJogAmp1 * NMath::Cos( fAngle1 ) + s_fJogAmp2 * NMath::Cos( fAngle2 );
	}
	else
	{
		if ( (fPeriod1 == 0) || (fPeriod2 == 0) )
			return 0;
		const float fAngle1 = fmod( (float)nDeltaTime*FP_2PI/fPeriod1, FP_2PI ) + fPhaze1;
		const float fAngle2 = fmod( (float)nDeltaTime*FP_2PI/fPeriod2, FP_2PI ) + fPhaze2;
		return fAmp1 * NMath::Cos( fAngle1 ) + fAmp2 * NMath::Cos( fAngle2 );
	}
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

void CMechUnitJoggingMutator::SetupPropellers(
	ISkeletonAnimator *pAnimator,
	const std::vector<std::string> &propellerObjects,
	const std::vector<CVec3> &speedsRad )
{
	propellerBoneIndices.clear();
	propellerSpeedsRad.clear();
	bPropellersEnabled = false;

	CDynamicCast<IGetBone> pGetBone = pAnimator;
	if ( !pGetBone )
		return;

	// Pair the configuration by index and keep only names present in this model's skeleton.
	const int nPropellerCount = (int)(std::min)( propellerObjects.size(), speedsRad.size() );
	for ( int i = 0; i < nPropellerCount; ++i )
	{
		const int nBoneIndex = pGetBone->GetBoneIndex( propellerObjects[i].c_str() );
		if ( nBoneIndex < 0 )
			continue;

		propellerBoneIndices.push_back( nBoneIndex );
		propellerSpeedsRad.push_back( speedsRad[i] );
	}

	// Game time advances every rendered frame, unlike fixed-step segment time.
	nPropellerStartTime = GameTimer()->GetGameTime();
	bPropellersEnabled = !propellerBoneIndices.empty();
}

void CMechUnitJoggingMutator::SetPropellersEnabled( const bool bEnabled )
{
	bPropellersEnabled = bEnabled && !propellerBoneIndices.empty();
}

bool CMechUnitJoggingMutator::NeedUpdate()
{
	IGameTimer *pTimer = GameTimer();
	const bool bJoggingNeedsUpdate = !bStopped && pTimer->GetSegmentTime() > nStartTime;
	const bool bPropellersNeedUpdate = bPropellersEnabled && !propellerBoneIndices.empty() &&
		pTimer->GetGameTime() > nPropellerStartTime;
	return pTimer->GetPauseType() == -1 && ( bJoggingNeedsUpdate || bPropellersNeedUpdate );
}

void CMechUnitJoggingMutator::MutateSkeletonPose( granny_local_pose *pPose )
{
	if ( nBasisBoneIndex >= 0 )
	{
		const NTimer::STime nDeltaTime = bStopped ? nStopTime - nStartTime : GameTimer()->GetSegmentTime() - nStartTime;

		const float fValueX = joggingX.GetValue( nDeltaTime );
		const float fValueY = s_bJogTweakMode ? 0 : joggingY.GetValue( nDeltaTime );
//		const float fValueZ = joggingZ.GetValue( nDeltaTime );

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

	if ( bPropellersEnabled )
	{
		const NTimer::STime nDeltaTime = GameTimer()->GetGameTime() - nPropellerStartTime;
		const float fSeconds = (float)nDeltaTime / 1000.0f;
		for ( int i = 0; i < (int)propellerBoneIndices.size(); ++i )
		{
			const CVec3 &vSpeed = propellerSpeedsRad[i];
			CQuat qPropeller;
			// Vector components are local X/Y/Z Euler angular velocities in radians per second.
			qPropeller.FromEulerAngles( fmod( vSpeed.z * fSeconds, FP_2PI ),
				fmod( vSpeed.y * fSeconds, FP_2PI ), fmod( vSpeed.x * fSeconds, FP_2PI ) );
			granny_transform *pTransform = GrannyGetLocalPoseTransform( pPose, propellerBoneIndices[i] );
			CQuat qBoneOrientation;
			memcpy( &qBoneOrientation, &(pTransform->Orientation), sizeof( float ) * 4 );
			qBoneOrientation *= qPropeller;
			memcpy( &(pTransform->Orientation), &qBoneOrientation, sizeof( float ) * 4 );
			pTransform->Flags |= GrannyHasOrientation;
		}
	}
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
	saver.Add( 7, &propellerBoneIndices );
	saver.Add( 8, &propellerSpeedsRad );
	saver.Add( 9, &nPropellerStartTime );
	saver.Add( 10, &bPropellersEnabled );

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


