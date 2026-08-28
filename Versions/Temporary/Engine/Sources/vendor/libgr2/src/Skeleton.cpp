// The skeleton, and looking a bone up by name.
//
// M2. The engine resolves locators and attachment points by name, so this is on
// the path of every unit that carries a turret, a barrel or an effect emitter.
//
// The comparison is a plain byte comparison against the bone names in the file.
// Those are ASCII in this game's data, which is what keeps this independent of
// the process locale.

#include <gr2/granny.h>

#include "Trace.h"

extern "C"
{

GR2_API( bool ) GrannyFindBoneByName( granny_skeleton const *Skeleton, char const *BoneName,
                                      granny_int32x *BoneIndex )
{
	GR2_STUB( "Skeleton={} BoneName={} BoneIndex={}", Skeleton, BoneName, BoneIndex );
	return false;
}

}
