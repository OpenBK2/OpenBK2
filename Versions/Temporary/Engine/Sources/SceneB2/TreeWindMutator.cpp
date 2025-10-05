#include "stdafx.h"

#include "System/Time.h"
#include "TreeWindMutator.h"
#include "WindController.h"
#include "System/FastMath.h"
#include "Scene.h"

#define TIME_COEFF 1

void CTreeWindMutator::Setup( ISkeletonAnimator *pAnimator, const CVec3 &_vPos3, const std::vector<std::string> &leafNames )
{
	vPos.x = _vPos3.x;
	vPos.y = _vPos3.y;

	lastUpdateTime = 0;
	fMagnitude = 0.0f;

	CDynamicCast<NAnimation::IGetBone> pGetBone = pAnimator;
	for ( std::vector<std::string>::const_iterator it = leafNames.begin(); it != leafNames.end(); ++it )
		leafBones.push_back( pGetBone->GetBoneIndex( it->c_str() ) );
}

void CTreeWindMutator::MutateSkeletonPose( granny_local_pose *pPose )
{
	NTimer::STime curTime = Singleton<IGameTimer>()->GetGameTime();

	if ( curTime > lastUpdateTime ) 
	{
		lastUpdateTime = curTime;
		fMagnitude = Scene()->GetWindController()->GetWindCoeff( vPos );
	}

	if ( fMagnitude == 0.0f )
		return;

	const float fIntensity = Scene()->GetWindController()->GetWindIntensity();
	const float fMaxTurn = ( fIntensity + 1.0f ) * 0.002f;

	const float fCoeff = vPos.x + curTime * 0.001f;
	const float fCoeffSin = NMath::Sin( fCoeff / fIntensity );
	const float fCoeffSin2 = NMath::Sin( fCoeff + fCoeff );
	const float fCoeffCos = NMath::Cos( fCoeff / fIntensity );
	const float fCoeffSinRnd = NMath::Sin( fCoeff * 1000 / fIntensity );

	float fCX, fCY;
	Scene()->GetWindController()->GetDirCoeff( &fCX, &fCY );

	//CQuat qRot( fMagnitude * fCoeffSin2 * fMaxTurn, fCoeffSin, fCoeffCos, 0.0f, true );		// "Random" direction wave
	CQuat qRot( fMagnitude * ( 2 + fCoeffSin2 ) * fMaxTurn * 0.5f, fCX, fCY, 0.0f, true );				// Wind direction wave
	CQuat qInverseRot( qRot );
	qInverseRot.UnitInverse();
	CQuat qRndRot( fMagnitude * fCoeffSinRnd * fMaxTurn * 0.3f, fCoeffCos, fCoeffSin, 2.0f, true );

	granny_transform *pTransform = GrannyGetLocalPoseTransform( pPose, 0 );
	TransformRootBone( pTransform, qRot );
	for ( std::vector<int>::const_iterator it = leafBones.begin(); it != leafBones.end(); ++it )
	{
		if ( *it == -1 )
			continue;
		pTransform = GrannyGetLocalPoseTransform( pPose, *it );
		TransformLeafBone( pTransform, qInverseRot * qRndRot );
	}
}

void CTreeWindMutator::TransformRootBone( granny_transform *pTransform, const CQuat &qRot )
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

void CTreeWindMutator::TransformLeafBone( granny_transform *pTransform, const CQuat &qRot )
{
	if ( !pTransform )
		return;
	CQuat qBoneOrientation( CVec4( pTransform->Orientation[0], pTransform->Orientation[1], pTransform->Orientation[2], pTransform->Orientation[3] ) );
	qBoneOrientation *= qRot;
	const CVec4 &vBoneOrientation = qBoneOrientation.GetInternalVector();
	pTransform->Orientation[0] = vBoneOrientation.x;
	pTransform->Orientation[1] = vBoneOrientation.y;
	pTransform->Orientation[2] = vBoneOrientation.z;
	pTransform->Orientation[3] = vBoneOrientation.w;
	pTransform->Flags |= GrannyHasOrientation;
}

bool CTreeWindMutator::NeedUpdate()
{
	NTimer::STime curTime = Singleton<IGameTimer>()->GetGameTime();

	if ( curTime <= lastUpdateTime ) 
		return false;

	lastUpdateTime = curTime;
	fMagnitude = Scene()->GetWindController()->GetWindCoeff( vPos );

	if ( fMagnitude == 0.0f )
		return false;

	return true;
}

int CTreeWindMutator::operator&( IBinSaver &saver )
{
	saver.Add( 1, &leafBones );	
	saver.Add( 2, &vPos );
	saver.Add( 3, &lastUpdateTime );
	saver.Add( 4, &fMagnitude );

	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x19132B40, CTreeWindMutator)


