// Geometry queries.
//
// One-liners over the converted structures, every one measured against
// granny2.dll rather than assumed, because every one had a plausible
// alternative.

#include <gr2/granny.h>

#include "Structures.h"
#include "Trace.h"

using namespace NGr2;

namespace
{

//! Which index array a mesh's topology offers, and how wide its entries are.
//!
//! Granny picks on the *count*, not on the pointer, and prefers the 32-bit
//! array. Measured against the DLL with meshes built in memory, because no
//! shipped file separates the two: all 6,031 meshes in the retail pak carry a
//! 32-bit array and no 16-bit one, so the corpus cannot distinguish these rules.
//!
//!   IndexCount   Indices   Index16Count  ->  count      pointer     bytes
//!   6            set       0                 6          Indices     4
//!   0            null      3                 3          Indices16   2
//!   6            set       3                 6          Indices     4
//!   0            null      0                 0          null        0
//!   3            null      0                 3          null        4
//!   0            set       0                 0          null        0
//!   6            null      3                 6          null        4
//!
//! The last three are what say it is the count: a non-zero IndexCount takes the
//! 32-bit branch and hands back whatever Indices holds, null included, and a
//! zero IndexCount ignores a non-null Indices entirely.
struct SIndexArray
{
	int32_t nCount = 0;
	void *pIndices = nullptr;
	int32_t nBytesPerIndex = 0;
};

SIndexArray IndexArray( const granny_mesh *Mesh )
{
	SIndexArray out;
	if ( Mesh == nullptr )
	{
		return out;
	}
	const STriTopology *pTopology = reinterpret_cast<const SMesh *>( Mesh )->pPrimaryTopology;
	if ( pTopology == nullptr )
	{
		return out;
	}
	if ( pTopology->nIndexCount != 0 )
	{
		out.nCount = pTopology->nIndexCount;
		out.pIndices = pTopology->pIndices;
		out.nBytesPerIndex = 4;
	}
	else if ( pTopology->nIndex16Count != 0 )
	{
		out.nCount = pTopology->nIndex16Count;
		out.pIndices = pTopology->pIndices16;
		out.nBytesPerIndex = 2;
	}
	return out;
}

//! A mesh's primary vertex data, or null. The three vertex accessors all read
//! it and all answer 0 or null when it is absent, measured the same way.
const SVertexData *VertexData( const granny_mesh *Mesh )
{
	return Mesh != nullptr ? reinterpret_cast<const SMesh *>( Mesh )->pPrimaryVertexData : nullptr;
}

}

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

// The vertex side. TraceModel in the map editor walks these to ray-cast against
// a model, stepping the vertex buffer at the stride GrannyGetTotalObjectSize
// gives for the type this returns.

GR2_API( granny_int32x ) GrannyGetMeshVertexCount( granny_mesh const *Mesh )
{
	GR2_TRACE( "Mesh={}", Mesh );

	const SVertexData *pData = VertexData( Mesh );
	return pData != nullptr ? static_cast<granny_int32x>( pData->nVertexCount ) : 0;
}

GR2_API( void * ) GrannyGetMeshVertices( granny_mesh const *Mesh )
{
	GR2_TRACE( "Mesh={}", Mesh );

	const SVertexData *pData = VertexData( Mesh );
	return pData != nullptr ? pData->pVertices : nullptr;
}

GR2_API( granny_data_type_definition * ) GrannyGetMeshVertexType( granny_mesh const *Mesh )
{
	GR2_TRACE( "Mesh={}", Mesh );

	const SVertexData *pData = VertexData( Mesh );
	return pData != nullptr
	           ? reinterpret_cast<granny_data_type_definition *>( pData->pVertexType )
	           : nullptr;
}

// The index side. See IndexArray above for which of the topology's two arrays
// each of these reports and why.

GR2_API( granny_int32x ) GrannyGetMeshIndexCount( granny_mesh const *Mesh )
{
	GR2_TRACE( "Mesh={}", Mesh );

	return static_cast<granny_int32x>( IndexArray( Mesh ).nCount );
}

GR2_API( void * ) GrannyGetMeshIndices( granny_mesh const *Mesh )
{
	GR2_TRACE( "Mesh={}", Mesh );

	return IndexArray( Mesh ).pIndices;
}

GR2_API( granny_int32x ) GrannyGetMeshBytesPerIndex( granny_mesh const *Mesh )
{
	GR2_TRACE( "Mesh={}", Mesh );

	return static_cast<granny_int32x>( IndexArray( Mesh ).nBytesPerIndex );
}

}
