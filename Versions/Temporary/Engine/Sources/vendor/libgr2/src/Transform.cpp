// granny_transform, the position, orientation and scale-shear triple a bone's
// local pose is expressed in.
//
// M3. Both of these are pure arithmetic on that structure, so neither can be
// written before the structure has its real layout, and both are wanted by the
// engine's own procedural bone mutators rather than only by pose evaluation.
//
// Two conventions collide here and are worth stating once: Granny stores
// quaternions x, y, z, w while glm::quat takes w first, and there are three
// matrix layouts in play across this library and the engine. Convert at one
// boundary, not at each use.

#include <gr2/granny.h>

#include "Trace.h"

extern "C"
{

GR2_API( void ) GrannyMakeIdentity( granny_transform *Result )
{
	GR2_STUB( "Result={}", Result );
}

GR2_API( void ) GrannyPostMultiplyBy( granny_transform *Transform,
                                      granny_transform const *PostMult )
{
	GR2_STUB( "Transform={} PostMult={}", Transform, PostMult );
}

}
