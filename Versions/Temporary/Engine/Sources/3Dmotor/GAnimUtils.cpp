#include "StdAfx.h"
#include "vendor/Granny/include/granny.h"
#include "GAnimUtils.h"

namespace NAnimation
{

// CAddBoneFilter

void CAddBoneFilter::Recalc()
{
	pAnimation.Refresh();
	granny_local_pose *pPose = pAnimation->GetValue().pPose;
	int nBones = GrannyGetLocalPoseBoneCount( pPose );
	if ( nAddBone >= nBones )
		return;
	if ( !IsValid( pSkeletonFileLoader ) )
	{
		pSkeletonFileLoader = NAnimation::GetSkeletonFileInfo( skelHandle.pSkeleton );
	}
	pSkeletonFileLoader.Refresh();
	const bool bValid = IsValid( pSkeletonFileLoader->GetValue() );
	ASSERT( bValid );
	const granny_skeleton *pSkeleton = GetSkeleton( pSkeletonFileLoader->GetValue(), skelHandle.nModelInFile );
	if ( !pSkeleton || pSkeleton->BoneCount != nBones )
		return;

	if ( !pGlobal )
		pGlobal = GrannyNewWorldPose( nAddBone + 1 );
	GrannyBuildWorldPose( pSkeleton, 0, nAddBone + 1, pPose, pAnimation->GetValue().poseGlobal, pGlobal );

	granny_real32 *pMatrix = GrannyGetWorldPose4x4( pGlobal, nAddBone );
	value.forward.Set( 
		pMatrix[0], pMatrix[4], pMatrix[8], pMatrix[12], 
		pMatrix[1], pMatrix[5], pMatrix[9], pMatrix[13],
		pMatrix[2], pMatrix[6], pMatrix[10], pMatrix[14],
		pMatrix[3], pMatrix[7], pMatrix[11], pMatrix[15]
	);
	value.backward.HomogeneousInverse( value.forward );
}

CAddBoneFilter::~CAddBoneFilter()
{
	if ( pGlobal )
		GrannyFreeWorldPose( pGlobal );
}

}
using namespace NAnimation;
REGISTER_SAVELOAD_CLASS( 0x10441190, CAddBoneFilter )

