// The mesh accessors, against what granny2.dll was measured to do.
//
// These six read fields off a converted granny_mesh and nothing else, so the
// fixtures here are meshes built in memory rather than files. That is not a
// shortcut around the real question, it is the only way to ask it: all 6,031
// meshes in the retail pak carry a 32-bit index array and no 16-bit one, so the
// corpus cannot say what happens with a 16-bit array, with both, with neither,
// or with a null topology.
//
// Every expectation below was read off granny2.dll, driven through ctypes with
// these same structures passed in as arguments. Where the DLL's rule had a
// plausible alternative, the alternative is named in the test that rules it out.
//
// Agreement on real data is a separate measurement and lives in
// scripts/port/gr2diff.py, which walks both implementations over the corpus.
// It reported no difference in any of the six over all 6,031 meshes.

#include "Structures.h"

#include <gr2/granny.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

using namespace NGr2;

namespace
{

//! A mesh and everything it points at, so that one object owns the fixture.
struct SFixture
{
	SMesh mesh{};
	SVertexData vertexData{};
	STriTopology topology{};
	SDataTypeDefinition vertexType{};
	std::vector<uint8_t> vertices;
	std::vector<int32_t> indices32;
	std::vector<uint16_t> indices16;

	granny_mesh *Mesh() { return reinterpret_cast<granny_mesh *>( &mesh ); }
};

//! A mesh with a vertex buffer and whichever index arrays are asked for.
//!
//! A count of 0 leaves that array absent, pointer and count both. The tests
//! that need the two to disagree set the fields themselves afterwards.
std::unique_ptr<SFixture> Make( int32_t nVertices, int32_t nIndices32, int32_t nIndices16 )
{
	auto p = std::make_unique<SFixture>();

	p->vertices.assign( static_cast<size_t>( nVertices ) * 32u, 0u );
	p->vertexData.pVertexType = &p->vertexType;
	p->vertexData.nVertexCount = nVertices;
	p->vertexData.pVertices = p->vertices.empty() ? nullptr : p->vertices.data();

	if ( nIndices32 > 0 )
	{
		p->indices32.assign( static_cast<size_t>( nIndices32 ), 1 );
		p->topology.nIndexCount = nIndices32;
		p->topology.pIndices = p->indices32.data();
	}
	if ( nIndices16 > 0 )
	{
		p->indices16.assign( static_cast<size_t>( nIndices16 ), 2u );
		p->topology.nIndex16Count = nIndices16;
		p->topology.pIndices16 = p->indices16.data();
	}

	p->mesh.pPrimaryVertexData = &p->vertexData;
	p->mesh.pPrimaryTopology = &p->topology;
	return p;
}

}

// --- vertices --------------------------------------------------------------

TEST( MeshVertices, ReportsTheVertexDataVerbatim )
{
	auto p = Make( 11, 6, 0 );

	EXPECT_EQ( 11, GrannyGetMeshVertexCount( p->Mesh() ) );
	EXPECT_EQ( static_cast<void *>( p->vertices.data() ), GrannyGetMeshVertices( p->Mesh() ) );
	EXPECT_EQ( reinterpret_cast<granny_data_type_definition *>( &p->vertexType ),
	           GrannyGetMeshVertexType( p->Mesh() ) );
}

TEST( MeshVertices, NullVertexDataIsZeroAndNull )
{
	auto p = Make( 11, 6, 0 );
	p->mesh.pPrimaryVertexData = nullptr;

	EXPECT_EQ( 0, GrannyGetMeshVertexCount( p->Mesh() ) );
	EXPECT_EQ( nullptr, GrannyGetMeshVertices( p->Mesh() ) );
	EXPECT_EQ( nullptr, GrannyGetMeshVertexType( p->Mesh() ) );
}

TEST( MeshVertices, EachFieldIsReadIndependently )
{
	// A null Vertices or a null VertexType does not suppress the other two. The
	// alternative rule, treating vertex data as absent unless it is complete,
	// was ruled out against the DLL.
	auto p = Make( 11, 6, 0 );
	p->vertexData.pVertices = nullptr;

	EXPECT_EQ( 11, GrannyGetMeshVertexCount( p->Mesh() ) );
	EXPECT_EQ( nullptr, GrannyGetMeshVertices( p->Mesh() ) );
	EXPECT_NE( nullptr, GrannyGetMeshVertexType( p->Mesh() ) );

	auto q = Make( 11, 6, 0 );
	q->vertexData.pVertexType = nullptr;

	EXPECT_EQ( 11, GrannyGetMeshVertexCount( q->Mesh() ) );
	EXPECT_NE( nullptr, GrannyGetMeshVertices( q->Mesh() ) );
	EXPECT_EQ( nullptr, GrannyGetMeshVertexType( q->Mesh() ) );
}

TEST( MeshVertices, TopologyDoesNotAffectThem )
{
	auto p = Make( 11, 6, 0 );
	p->mesh.pPrimaryTopology = nullptr;

	EXPECT_EQ( 11, GrannyGetMeshVertexCount( p->Mesh() ) );
	EXPECT_NE( nullptr, GrannyGetMeshVertices( p->Mesh() ) );
	EXPECT_NE( nullptr, GrannyGetMeshVertexType( p->Mesh() ) );
}

// --- indices ---------------------------------------------------------------

TEST( MeshIndices, ThirtyTwoBitArray )
{
	// The only shape any shipped file has.
	auto p = Make( 11, 6, 0 );

	EXPECT_EQ( 6, GrannyGetMeshIndexCount( p->Mesh() ) );
	EXPECT_EQ( static_cast<void *>( p->indices32.data() ), GrannyGetMeshIndices( p->Mesh() ) );
	EXPECT_EQ( 4, GrannyGetMeshBytesPerIndex( p->Mesh() ) );
}

TEST( MeshIndices, SixteenBitArray )
{
	auto p = Make( 11, 0, 3 );

	EXPECT_EQ( 3, GrannyGetMeshIndexCount( p->Mesh() ) );
	EXPECT_EQ( static_cast<void *>( p->indices16.data() ), GrannyGetMeshIndices( p->Mesh() ) );
	EXPECT_EQ( 2, GrannyGetMeshBytesPerIndex( p->Mesh() ) );
}

TEST( MeshIndices, ThirtyTwoBitWinsWhenBothArePresent )
{
	auto p = Make( 11, 6, 3 );

	EXPECT_EQ( 6, GrannyGetMeshIndexCount( p->Mesh() ) );
	EXPECT_EQ( static_cast<void *>( p->indices32.data() ), GrannyGetMeshIndices( p->Mesh() ) );
	EXPECT_EQ( 4, GrannyGetMeshBytesPerIndex( p->Mesh() ) );
}

TEST( MeshIndices, NeitherArrayIsZeroWidth )
{
	// Not four. A mesh with no indices reports a width of 0, which is the one
	// place the DLL does not fall back on the 32-bit default.
	auto p = Make( 11, 0, 0 );

	EXPECT_EQ( 0, GrannyGetMeshIndexCount( p->Mesh() ) );
	EXPECT_EQ( nullptr, GrannyGetMeshIndices( p->Mesh() ) );
	EXPECT_EQ( 0, GrannyGetMeshBytesPerIndex( p->Mesh() ) );
}

TEST( MeshIndices, NullTopologyIsZeroWidth )
{
	auto p = Make( 11, 6, 0 );
	p->mesh.pPrimaryTopology = nullptr;

	EXPECT_EQ( 0, GrannyGetMeshIndexCount( p->Mesh() ) );
	EXPECT_EQ( nullptr, GrannyGetMeshIndices( p->Mesh() ) );
	EXPECT_EQ( 0, GrannyGetMeshBytesPerIndex( p->Mesh() ) );
}

TEST( MeshIndices, TheCountDecidesAndNotThePointer )
{
	// A non-zero IndexCount takes the 32-bit branch whatever Indices holds, so
	// the width is 4 and the pointer comes back null rather than being repaired
	// from Indices16. This is what rules out "pick whichever pointer is set".
	auto p = Make( 11, 3, 0 );
	p->topology.pIndices = nullptr;

	EXPECT_EQ( 3, GrannyGetMeshIndexCount( p->Mesh() ) );
	EXPECT_EQ( nullptr, GrannyGetMeshIndices( p->Mesh() ) );
	EXPECT_EQ( 4, GrannyGetMeshBytesPerIndex( p->Mesh() ) );
}

TEST( MeshIndices, AZeroCountIgnoresANonNullPointer )
{
	// The other half of the same rule.
	auto p = Make( 11, 6, 0 );
	p->topology.nIndexCount = 0;

	EXPECT_EQ( 0, GrannyGetMeshIndexCount( p->Mesh() ) );
	EXPECT_EQ( nullptr, GrannyGetMeshIndices( p->Mesh() ) );
	EXPECT_EQ( 0, GrannyGetMeshBytesPerIndex( p->Mesh() ) );
}

TEST( MeshIndices, AThirtyTwoBitCountBeatsAPresentSixteenBitArray )
{
	// IndexCount says 6 with no array behind it, Indices16 is real. The 32-bit
	// branch still wins, which is the sharpest form of "the count decides".
	auto p = Make( 11, 0, 3 );
	p->topology.nIndexCount = 6;

	EXPECT_EQ( 6, GrannyGetMeshIndexCount( p->Mesh() ) );
	EXPECT_EQ( nullptr, GrannyGetMeshIndices( p->Mesh() ) );
	EXPECT_EQ( 4, GrannyGetMeshBytesPerIndex( p->Mesh() ) );
}

// --- defensive -------------------------------------------------------------

TEST( MeshAccessors, NullMesh )
{
	// The DLL faults on this; every entry point in this library answers instead,
	// the way GrannyGetMeshTriangleGroupCount and GrannyMeshIsRigid beside them
	// already do.
	EXPECT_EQ( 0, GrannyGetMeshVertexCount( nullptr ) );
	EXPECT_EQ( nullptr, GrannyGetMeshVertices( nullptr ) );
	EXPECT_EQ( nullptr, GrannyGetMeshVertexType( nullptr ) );
	EXPECT_EQ( 0, GrannyGetMeshIndexCount( nullptr ) );
	EXPECT_EQ( nullptr, GrannyGetMeshIndices( nullptr ) );
	EXPECT_EQ( 0, GrannyGetMeshBytesPerIndex( nullptr ) );
}
