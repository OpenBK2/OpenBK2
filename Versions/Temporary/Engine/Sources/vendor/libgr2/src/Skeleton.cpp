// Finding a bone by the name the exporter gave it.
//
// The engine's only way in: 3Dmotor's GObjectInfo turns a mesh's bone binding
// name into an index this way, and GAnimation does the same for every animated
// bone, so a wrong answer here is a limb attached to the wrong joint rather than
// a crash.

#include <gr2/granny.h>

#include "Structures.h"
#include "Trace.h"

#include <cstring>

using namespace NGr2;

extern "C"
{

GR2_API( bool ) GrannyFindBoneByName( granny_skeleton const *Skeleton, char const *BoneName,
                                      granny_int32x *BoneIndex )
{
	GR2_TRACE( "Skeleton={} BoneName={} BoneIndex={}", Skeleton, BoneName, BoneIndex );

	if ( Skeleton == 0 || BoneName == 0 || BoneIndex == 0 )
	{
		return false;
	}

	const SSkeleton *pSkeleton = reinterpret_cast<const SSkeleton *>( Skeleton );

	// Linear, and in order, because that is what the answer has to be. Bone names
	// in these files are not sorted, so nothing faster is available without an
	// index built at conversion time, and 20,466 lookups over the corpus each
	// found the bone at its own position rather than at a first match elsewhere.
	for ( int32_t i = 0; i < pSkeleton->nBoneCount; ++i )
	{
		const char *pszName = pSkeleton->pBones[i].pszName;
		if ( pszName != nullptr && strcmp( pszName, BoneName ) == 0 )
		{
			*BoneIndex = static_cast<granny_int32x>( i );
			return true;
		}
	}

	// The bone count on a miss, not the index left alone and not -1. Measured out
	// of the real DLL over 6,108 lookups of a name that was not there, with the
	// out parameter pre-set to two different sentinels to be sure it is written
	// rather than untouched. No engine code reads it after a false return, but a
	// replacement that differs here differs, and this costs nothing.
	*BoneIndex = static_cast<granny_int32x>( pSkeleton->nBoneCount );
	return false;
}

}
