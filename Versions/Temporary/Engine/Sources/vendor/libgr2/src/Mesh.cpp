// Geometry queries.
//
// Two one-liners over the converted structures, and both were measured against
// granny2.dll rather than assumed, because both had a plausible alternative.

#include <gr2/granny.h>

#include "Structures.h"
#include "Trace.h"

using namespace NGr2;

extern "C"
{

GR2_API( granny_int32x ) GrannyGetMeshTriangleGroupCount( granny_mesh const *Mesh )
{
	GR2_TRACE( "Mesh={}", Mesh );

	if ( Mesh == 0 )
	{
		return 0;
	}

	// The topology's group count, and nothing cleverer: measured equal in all 381
	// meshes of a first sample and in every mesh of the corpus sweep since.
	// GrannyGetMeshTriangleGroups hands back that same array, by pointer.
	const SMesh *pMesh = reinterpret_cast<const SMesh *>( Mesh );
	return pMesh->pPrimaryTopology != nullptr
	           ? static_cast<granny_int32x>( pMesh->pPrimaryTopology->nGroupCount )
	           : 0;
}

GR2_API( bool ) GrannyMeshIsRigid( granny_mesh const *Mesh )
{
	GR2_TRACE( "Mesh={}", Mesh );

	if ( Mesh == 0 )
	{
		return false;
	}

	// A mesh bound to at most one bone needs no skinning.
	//
	// There is a second reading, that a mesh is rigid when its vertices carry no
	// bone weights, and the two were measured against each other over 5,628
	// meshes: they never disagreed, and neither ever disagreed with the DLL. This
	// one is chosen because it does not need the vertex type converted, and
	// because it is what the engine's own guard just above the call assumes,
	// ConvertWeightsFromGrannyEx rejecting BoneBindingCount <= 0 and then reading
	// BoneBindings[0].
	//
	// The measured spread: 5,863 meshes with one binding, all rigid; 160 with two
	// to 108 bindings, none rigid. No shipped mesh has none.
	const SMesh *pMesh = reinterpret_cast<const SMesh *>( Mesh );
	return pMesh->nBoneBindingCount <= 1;
}

}
