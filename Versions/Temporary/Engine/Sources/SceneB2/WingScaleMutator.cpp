#include "stdafx.h"

#include "vendor/Granny/include/granny.h"
#include "WingScaleMutator.h"

bool CWingScaleMutator::Setup( ISkeletonAnimator *pAnimator, const string &szScaledWingPrefix, const string &szStaticWingName )
{
	CDynamicCast<NAnimation::IGetBone> pGetBone = pAnimator;
	if ( pGetBone )
	{
		vector<string> names;
		pGetBone->GetBoneNames( &names );
		for ( vector<string>::const_iterator it = names.begin(); it != names.end(); ++it )
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
	for ( vector<int>::const_iterator it = scaledWings.begin(); it != scaledWings.end(); ++it )
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

REGISTER_SAVELOAD_CLASS( 0x3119AB00, CWingScaleMutator );


