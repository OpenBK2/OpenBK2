#include "StdAfx.h"

#include "TreeFallingMutator.h"
#include "../System/FastMath.h"
#include "../Misc/Win32Random.h"
#include "Scene.h"

void CTreeFallingMutator::Setup( ISkeletonAnimator *pAnimator, const CVec2 &vDir, float _fEndAngle, const CQuat &qRot,
																 const vector<string> &leafNames, int _nEffectID, const CVec3 &vPos,
																 float _fEffectHeight, float fFallCycles, int nFallDuration, NTimer::STime timeStart )
{
	fEndAngle = _fEndAngle;
	CDynamicCast<NAnimation::IGetBone> pGetBone = pAnimator;
	for ( vector<string>::const_iterator it = leafNames.begin(); it != leafNames.end(); ++it )
	{
		leafBones.push_back();
		leafBones.back().nBoneIndex = pGetBone->GetBoneIndex( it->c_str() );
		NI_ASSERT( leafBones.back().nBoneIndex != -1, StrFmt( "Wrong bone name: %s", it->c_str() ) );
		leafBones.back().fMaxRotAngle = NWin32Random::Random( -1.0f, 1.0f ) * FP_PI * 0.25f;
		leafBones.back().fNormalRotAngle = fEndAngle + NWin32Random::Random( -1.0f, 1.0f ) * FP_PI * 0.125f;
		if ( leafBones.back().fNormalRotAngle > FP_PI * 0.5f )
			leafBones.back().fNormalRotAngle = FP_PI * 0.5f;
	}
	pTimer = Singleton<IGameTimer>();
	nStartTime = timeStart;
	SHMatrix mTrans;
	qRot.DecompReversedEulerMatrix( &mTrans );
	mTrans.RotateVector( &vRotAxis, CVec3( -vDir.y, vDir.x, 0 ) );
	vEffectRotAxis.Set( -vDir.y, vDir.x, 0 );
	pAnimator->SetSpecialMutator( this );
	
	nEffectID = _nEffectID;
	vTreePos = vPos;
	fEffectHeight = _fEffectHeight;
	
	fCycles = fFallCycles;
	fAnimLength = nFallDuration;
}

float CTreeFallingMutator::GetCoeffForTime( int nTime )
{
	if ( bFinished )
		return 1.0f;
	const float fTimeCoeff = ( FP_PI * fCycles * 0.5f ) / fAnimLength;
	float fCoeff = ( fAnimLength - nTime ) / fAnimLength;
	if ( fCoeff < 0.0f )
	{
		bFinished = true;
		fCoeff = 0.0f;
	}
	fCoeff *= abs( fCoeff );
	float fResult = abs( NMath::Cos( nTime * fTimeCoeff ) );
	return 1.0 - fResult * fCoeff;
}

void CTreeFallingMutator::TransformRootBone( granny_transform *pTransform, const CQuat &qRot )
{
	CQuat qBoneOrientation( CVec4( pTransform->Orientation[0], pTransform->Orientation[1], pTransform->Orientation[2], pTransform->Orientation[3] ) );
	qBoneOrientation *= qRot;
	const CVec4 &vBoneOrientation = qBoneOrientation.GetInternalVector();
	pTransform->Orientation[0] = vBoneOrientation.x;
	pTransform->Orientation[1] = vBoneOrientation.y;
	pTransform->Orientation[2] = vBoneOrientation.z;
	pTransform->Orientation[3] = vBoneOrientation.w;
	CVec3 vOldBonePos( pTransform->Position[0], pTransform->Position[1], pTransform->Position[2] );
	CVec3 vBonePos;
	qRot.Rotate( &vBonePos, vOldBonePos );
	pTransform->Position[0] = vBonePos.x;
	pTransform->Position[1] = vBonePos.y;
	pTransform->Position[2] = vBonePos.z;
	pTransform->Flags |= GrannyHasPosition;
	pTransform->Flags |= GrannyHasOrientation;
}

void CTreeFallingMutator::TransformLeafBone( granny_transform *pTransform, const CQuat &qRot )
{
	CQuat qBoneOrientation( CVec4( pTransform->Orientation[0], pTransform->Orientation[1], pTransform->Orientation[2], pTransform->Orientation[3] ) );
	qBoneOrientation *= qRot;
	const CVec4 &vBoneOrientation = qBoneOrientation.GetInternalVector();
	pTransform->Orientation[0] = vBoneOrientation.x;
	pTransform->Orientation[1] = vBoneOrientation.y;
	pTransform->Orientation[2] = vBoneOrientation.z;
	pTransform->Orientation[3] = vBoneOrientation.w;
	pTransform->Flags |= GrannyHasOrientation;
}

void CTreeFallingMutator::MutateSkeletonPose( granny_local_pose *pPose )
{	
	float fCoeff = GetCoeffForTime( pTimer->GetGameTime() - nStartTime );

	CQuat qRot( fCoeff * fEndAngle, vRotAxis );
	CQuat qEffectRot( fCoeff * fEndAngle, vEffectRotAxis );

	granny_transform *pTransform = GrannyGetLocalPoseTransform( pPose, 0 );
	TransformRootBone( pTransform, qRot );
	for ( list<SLeafMutatorData>::const_iterator it = leafBones.begin(); it != leafBones.end(); ++it )
	{
		if ( it->nBoneIndex == -1 )
			continue;
		CQuat qInverseRot( -fCoeff * it->fNormalRotAngle, vRotAxis );
		CQuat qRndRot( fCoeff * it->fMaxRotAngle, 0.0f, 0.0f, 1.0f );
		pTransform = GrannyGetLocalPoseTransform( pPose, it->nBoneIndex );
		TransformLeafBone( pTransform, qInverseRot * qRndRot );
	}
	
	if ( !bFinished && nEffectID != -1 )
	{
		CVec3 vTransformedEffect;
		CVec3 vEffectPoint( 0, 0, fEffectHeight );
		qEffectRot.Rotate( &vTransformedEffect, vEffectPoint );
		vTransformedEffect += vTreePos;
		Scene()->MoveObject( nEffectID, vTransformedEffect, QNULL );
	}
}

int CTreeFallingMutator::operator&( IBinSaver &saver )
{
	saver.Add( 1, &nRootBoneIdx );
	saver.Add( 2, &fEndAngle );
	saver.Add( 3, &nStartTime );
	saver.Add( 4, &vRotAxis );
	saver.Add( 5, &leafBones );
	saver.Add( 6, &bFinished );
	saver.Add( 7, &nEffectID );
	saver.Add( 8, &vTreePos );
	saver.Add( 9, &fEffectHeight );
	saver.Add( 10, &vEffectRotAxis );
	saver.Add( 11, &fCycles );
	saver.Add( 12, &fAnimLength );
	saver.Add( 13, &pTimer );

	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x12094B80, CTreeFallingMutator )

