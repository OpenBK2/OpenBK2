#include "stdafx.h"

#include "vendor/granny/include/granny.h"
#include "WingScaleMutator.h"

#include "Misc/StrProc.h"

#include "SceneB2_export.h"

// strnicmp is an MSVC CRT extension. The names being matched are Granny bone names,
// which are ASCII, so ASCII_tolower answers the same question strnicmp did.
//
// strnicmp stopped at the terminator, so a name shorter than the prefix never
// matched; the size test below is that.
static bool StartsWithNoCase( const std::string &szName, const std::string &szPrefix )
{
	if ( szName.size() < szPrefix.size() )
	{
		return false;
	}
	for ( std::string::size_type i = 0; i < szPrefix.size(); ++i )
	{
		if ( NStr::ASCII_tolower( szName[i] ) != NStr::ASCII_tolower( szPrefix[i] ) )
		{
			return false;
		}
	}
	return true;
}

bool CWingScaleMutator::Setup( ISkeletonAnimator *pAnimator, const std::string &szScaledWingPrefix, const std::string &szStaticWingName )
{
	CDynamicCast<NAnimation::IGetBone> pGetBone = pAnimator;
	if ( pGetBone )
	{
		std::vector<std::string> names;
		pGetBone->GetBoneNames( &names );
		for ( std::vector<std::string>::const_iterator it = names.begin(); it != names.end(); ++it )
		{
			if ( StartsWithNoCase( *it, szScaledWingPrefix ) )
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


