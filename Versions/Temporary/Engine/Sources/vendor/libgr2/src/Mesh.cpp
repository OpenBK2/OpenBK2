// Queries about a mesh that the engine cannot answer by reading the structure.
//
// M2. Both of these are small, and both gate a branch the engine takes on every
// model it loads: how many triangle groups to iterate, and whether the mesh is
// rigid or has to go through the skinning path.
//
// Note that only the CObjectInfo loader goes through these. Two further readers,
// aiObjectLoader for collision geometry and TerraTools for terrain and debris,
// walk granny_mesh themselves, so geometry is not covered until the structure
// layout is right as well as these two functions.

#include <gr2/granny.h>

#include "Trace.h"

extern "C"
{

GR2_API( granny_int32x ) GrannyGetMeshTriangleGroupCount( granny_mesh const *Mesh )
{
	GR2_STUB( "Mesh={}", Mesh );
	return 0;
}

GR2_API( bool ) GrannyMeshIsRigid( granny_mesh const *Mesh )
{
	GR2_STUB( "Mesh={}", Mesh );
	return false;
}

}
