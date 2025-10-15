#include "stdafx.h"

#include "vendor/granny/include/granny.h"
#include "WingScaleMutator.h"

#include "SceneB2_export.h"

bool CWingScaleMutator::Setup( ISkeletonAnimator *pAnimator, const std::string &szScaledWingPrefix, const std::string &szStaticWingName )
{
	CDynamicCast<NAnimation::IGetBone> pGetBone = pAnimator;
	if ( pGetBone )
	{
		std::vector<std::string> names;
		pGetBone->GetBoneNames( &names );
		for ( std::vector<std::string>::const_iterator it = names.begin(); it != names.end(); ++it )
		{
			if ( strnicmp( it->c_str(), szScaledWingPrefix.c_str(), szScaledWingPrefix.length() ) == 0 )
				scaledWings.push_back( pGetBone->GetBoneIndex( it->c_str() ) );
		}
		nStaticWing = pGetBone->GetBoneIndex( szStaticWingName.c_str() );
		if ( !scaledWings.empty() && nStaticWing != -1 )
			pAnimator->SetSpecialMutator( this );
	}
	return !scaledWings.empty() && nStaticWing != -1;
}

void CWingScaleMutator::MutateSkeletonPose( granny_local_pose *pPose )
{
	for ( std::vector<int>::const_iterator it = scaledWings.begin(); it != scaledWings.end(); ++it )
	{
		granny_transform *pRootTransform = GrannyGetLocalPoseTransform( pPose, *it );
		pRootTransform->ScaleShear[0][0] = fScale;
		pRootTransform->Flags |= GrannyHasScaleShear;
	}

	granny_transform *pRootTransform = GrannyGetLocalPoseTransform( pPose, nStaticWing );
	if ( !bShowStatic )
	{
		pRootTransform->ScaleShear[0][0] = 0.0f;
		pRootTransform->ScaleShear[1][1] = 0.0f;
		pRootTransform->ScaleShear[2][2] = 0.0f;
		pRootTransform->Flags |= GrannyHasScaleShear;
	}
	else
	{
		pRootTransform->ScaleShear[0][0] = 1.0f;
		pRootTransform->ScaleShear[1][1] = 1.0f;
		pRootTransform->ScaleShear[2][2] = 1.0f;
		pRootTransform->Flags |= GrannyHasScaleShear;
	}
}

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x3119AB00, CWingScaleMutator );


