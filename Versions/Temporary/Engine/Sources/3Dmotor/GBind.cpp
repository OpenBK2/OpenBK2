#include "stdafx.h"
#include "vendor/granny/include/granny.h"
#include "GBind.h"
#include "3DLib/Transform.h"

namespace NGScene
{

// CBind

CBind::CBind( CFuncBase<NAnimation::SGrannySkeletonPose> *_pAnimation, const NDb::SSkeleton *pSkeleton, int _nModelInFile ) 
	: pGlobal(0), pAnimation(_pAnimation)
{
	skeletonH.pSkeleton = pSkeleton;
	skeletonH.nModelInFile = _nModelInFile;
}

CBind::CBind( CFuncBase<NAnimation::SGrannySkeletonPose> *_pAnimation, const NAnimation::SGrannySkeletonHandle &_skeletonH )
	: pGlobal(0), pAnimation(_pAnimation), skeletonH(_skeletonH)
{
}

void CBind::Recalc()
{
	const granny_local_pose *pPose = pAnimation->GetValue().pPose;
	const float *pGlobalTransform = pAnimation->GetValue().poseGlobal;
	int nBones = GrannyGetLocalPoseBoneCount( pPose );
	if ( !IsValid( pSkeletonFileLoader ) )
	{
		pSkeletonFileLoader = NAnimation::GetSkeletonFileInfo( skeletonH.pSkeleton );
	}
	pSkeletonFileLoader.Refresh();
	ASSERT( IsValid(pSkeletonFileLoader->GetValue()) );
	const granny_skeleton *pSkel = NAnimation::GetSkeleton( pSkeletonFileLoader->GetValue(), skeletonH.nModelInFile );
	if ( !pSkel || pSkel->BoneCount != nBones )
	{
		value.resize( nBones );
		for ( int i = 0; i < nBones; ++i )		
			Identity( &value[i] );
		Updated();
		return;
	}

	value.resize( nBones );

	if ( !pGlobal )
		pGlobal = GrannyNewWorldPose( nBones );

	GrannyBuildWorldPose( pSkel, 0, nBones, pPose, pGlobalTransform, pGlobal );

	for ( int i = 0; i < nBones; ++i )
	{
		granny_real32 *pMatrix = GrannyGetWorldPoseComposite4x4( pGlobal, i );
		value[i].Set( 
			pMatrix[0], pMatrix[4], pMatrix[8], pMatrix[12], 
			pMatrix[1], pMatrix[5], pMatrix[9], pMatrix[13],
			pMatrix[2], pMatrix[6], pMatrix[10], pMatrix[14],
			pMatrix[3], pMatrix[7], pMatrix[11], pMatrix[15]
		);
	}
}

CBind::~CBind()
{
	if ( pGlobal )
		GrannyFreeWorldPose( pGlobal );
}

// CAnimatedBound

CAnimatedBound::CAnimatedBound( const SBound &_bv, CFuncBase<NAnimation::SGrannySkeletonPose> *_pAnimation ) 
	: bv(_bv), pAnimation(_pAnimation) 
{
	Zero( value );
	Zero( prevValue );
}

static bool operator==( const SSphere &a, const SSphere &b )
{
	return memcmp( &a, &b, sizeof(a) ) == 0;
}
bool CAnimatedBound::NeedUpdate() 
{ 
	// discretisation step
	const float F_DISCR_STEP = 1;
	if ( !pAnimation.Refresh() )
		return false;
	const NAnimation::SGrannySkeletonPose &pose = pAnimation->GetValue();
	const SHMatrix *pGlobal = (const SHMatrix*)pose.poseGlobal;
	SHMatrix pos;
	Transpose( &pos, *pGlobal );
	CVec3 ptCenter;
	float fRadius = sqrt( CalcRadius2( bv, pos ) );
	pos.RotateHVector( &ptCenter, bv.s.ptCenter );
	// sphere discretisaion
	SSphere newBV;
	DiscretisizeBoundSphere( &newBV, ptCenter, fRadius, F_DISCR_STEP );
	if ( newBV == prevValue )
		return false;
	prevValue = newBV;
	value.SphereInit( prevValue.ptCenter, prevValue.fRadius );
	return true;
}

void CAnimatedBound::Recalc()
{
}


void DiscretisizeBoundSphere( SSphere *pResult, const CVec3 &ptCenter, const float fRadius, const float fDiscrStep )
{
	// position discretisation
	CVec3 vDiscrCenter;
	vDiscrCenter.x = Float2Int( ptCenter.x * (1 / fDiscrStep) ) * fDiscrStep;
	vDiscrCenter.y = Float2Int( ptCenter.y * (1 / fDiscrStep) ) * fDiscrStep;
	vDiscrCenter.z = Float2Int( ptCenter.z * (1 / fDiscrStep) ) * fDiscrStep;
	// radius should be larger because of position shift
	float fDiscrRadius = fRadius + fabs( ptCenter - vDiscrCenter );
	// radius discretisation with bias to max
	fDiscrRadius = Float2Int( fDiscrRadius * ( 1 / fDiscrStep ) + 0.5f ) * fDiscrStep;
	pResult->ptCenter = vDiscrCenter;
	pResult->fRadius = fDiscrRadius;
}


} // namespace
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x12041162, CBind )
REGISTER_SAVELOAD_CLASS( 0x2013BC80, CAnimatedBound )

