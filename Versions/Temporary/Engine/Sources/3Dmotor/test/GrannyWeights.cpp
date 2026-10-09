// Exercise the engine's real Granny conversion with compact in-memory vertices.
#include "3Dmotor/stdafx.h"
#include "3Dmotor/GObjectInfo.h"
#include "vendor/granny/include/granny.h"

#include <gtest/gtest.h>

namespace
{
struct SSkinnedMesh
{
	granny_data_type_definition type[3] = {};
	granny_bone bones[4] = {};
	granny_bone_binding bindings[4] = {};
	granny_skeleton skeleton = {};
	granny_vertex_data vertices = {};
	granny_mesh mesh = {};
	std::vector<granny_uint8> bytes;
	int width;

	explicit SSkinnedMesh( int nWidth ) : bytes( 4 * nWidth ), width( nWidth )
	{
		const char *names[] = { "root", "arm", "leg", "head" };
		for ( int i = 0; i < 4; ++i )
		{
			bones[i].Name = names[i];
			bones[i].ParentIndex = -1;
			// Reverse the binding order to check that indices are remapped by name.
			bindings[i].BoneName = names[3 - i];
		}
		skeleton.Name = "weight test";
		skeleton.BoneCount = 4;
		skeleton.Bones = bones;
		type[0].Type = GrannyNormalUInt8Member;
		type[0].Name = GrannyVertexBoneWeightsName;
		type[0].ArrayWidth = width;
		type[1].Type = GrannyUInt8Member;
		type[1].Name = GrannyVertexBoneIndicesName;
		type[1].ArrayWidth = width;
		vertices.VertexType = type;
		vertices.VertexCount = 2;
		vertices.Vertices = bytes.data();
		mesh.PrimaryVertexData = &vertices;
		mesh.BoneBindingCount = 4;
		mesh.BoneBindings = bindings;
	}

	granny_uint8 &Weight( int vertex, int influence ) { return bytes[vertex * 2 * width + influence]; }
	granny_uint8 &Index( int vertex, int influence ) { return bytes[vertex * 2 * width + width + influence]; }
	const char *Convert( std::vector<NGScene::SVertexWeight> *result )
	{
		return NGScene::ConvertWeightsFromGrannyEx( &skeleton, &mesh, 0, result, vertices.VertexCount );
	}
};

TEST( GrannyWeights, ConvertsOneThroughFourInfluencesAndClearsUnusedSlots )
{
	for ( int width = 1; width <= 4; ++width )
	{
		SCOPED_TRACE( width );
		SSkinnedMesh source( width );
		std::vector<NGScene::SVertexWeight> result( 2 );
		for ( int vertex = 0; vertex < 2; ++vertex )
		{
			for ( int j = 0; j < 4; ++j )
			{
				// Reused destinations must not preserve old influences.
				result[vertex].fWeights[j] = 1.0f;
				result[vertex].cBoneIndices[j] = 255;
			}
			for ( int j = 0; j < width; ++j )
			{
				source.Weight( vertex, j ) = static_cast<granny_uint8>( j + 1 == width ? 255 - (width - 1) * 40 : 40 );
				source.Index( vertex, j ) = static_cast<granny_uint8>( (j + vertex) % 4 );
			}
		}
		ASSERT_EQ( nullptr, source.Convert( &result ) );
		ASSERT_EQ( 2u, result.size() );
		for ( int vertex = 0; vertex < 2; ++vertex )
			for ( int j = 0; j < 4; ++j )
			{
				EXPECT_FLOAT_EQ( j < width ? source.Weight( vertex, j ) / 255.0f : 0.0f, result[vertex].fWeights[j] );
				EXPECT_EQ( j < width ? 3 - source.Index( vertex, j ) : 0, result[vertex].cBoneIndices[j] );
			}
	}
}

TEST( GrannyWeights, IgnoresOutOfRangeIndicesOnZeroWeightInfluences )
{
	SSkinnedMesh source( 4 );
	for ( int vertex = 0; vertex < 2; ++vertex )
	{
		source.Weight( vertex, 0 ) = 255;
		for ( int j = 1; j < 4; ++j )
			source.Index( vertex, j ) = 255;
	}
	std::vector<NGScene::SVertexWeight> result;
	ASSERT_EQ( nullptr, source.Convert( &result ) );
	for ( const auto &vertex : result )
	{
		EXPECT_FLOAT_EQ( 1.0f, vertex.fWeights[0] );
		EXPECT_EQ( 3, vertex.cBoneIndices[0] );
		for ( int j = 1; j < 4; ++j )
		{
			EXPECT_FLOAT_EQ( 0.0f, vertex.fWeights[j] );
			EXPECT_EQ( 0, vertex.cBoneIndices[j] );
		}
	}
}

TEST( GrannyWeights, RejectsOutOfRangeActiveBindingWithoutPublishingIt )
{
	SSkinnedMesh source( 1 );
	for ( const int badIndex : { 4, 255 } )
	{
		source.Weight( 0, 0 ) = 255;
		source.Index( 0, 0 ) = static_cast<granny_uint8>( badIndex );
		std::vector<NGScene::SVertexWeight> result;
		EXPECT_NE( nullptr, source.Convert( &result ) );
		ASSERT_EQ( 2u, result.size() );
		EXPECT_EQ( 0, result[0].cBoneIndices[0] );
		EXPECT_FLOAT_EQ( 0.0f, result[0].fWeights[0] );
	}
}

TEST( GrannyWeights, RejectsMissingSkeletonBoneWithoutPublishingPastEndIndex )
{
	SSkinnedMesh source( 1 );
	source.bindings[0].BoneName = "missing";
	source.Weight( 0, 0 ) = 255;
	std::vector<NGScene::SVertexWeight> result;
	// GrannyFindBoneByName writes BoneCount on a miss; that is not a valid index.
	EXPECT_NE( nullptr, source.Convert( &result ) );
	ASSERT_EQ( 2u, result.size() );
	EXPECT_EQ( 0, result[0].cBoneIndices[0] );
	EXPECT_FLOAT_EQ( 0.0f, result[0].fWeights[0] );
}
}
