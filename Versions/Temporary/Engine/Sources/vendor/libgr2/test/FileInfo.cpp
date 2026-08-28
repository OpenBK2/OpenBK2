// GrannyGetFileInfo: the conversion from what the file holds to what the engine
// reads.
//
// This is where the engine crashed against the stub. GObjectInfo.cpp reaches
// pData->Models[0] the moment a geometry resource loads, so a null here is a
// dereferenced null two lines later.
//
// The fixtures are hand authored, which for this entry point means authoring a
// type tree as well as an object: a GR2 describes its own structures, so the
// only way to build a file whose root is a granny_file_info is to write the type
// definitions that say so. CTypedFile below does that, and the result is a real
// exercise of the walker rather than a mock of it.
//
// What is not here is agreement with granny2.dll on real data, which is the
// measurement that matters and needs both the DLL and the corpus. That was run
// separately over the retail Oodle1 geometry and skeletons, comparing names,
// counts, parent indices, transforms, vertex strides, triangle groups, bone
// bindings and mesh pointer identity; the commit that added this file records
// the number.

#include "MinimalGr2.h"

#include "Convert.h"
#include "File.h"
#include "Structures.h"
#include "TypeTree.h"

#include <gr2/granny.h>

#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace NGr2Test;

namespace
{

//! A file with a hand authored type tree, so the walker has something to walk.
//!
//! Section 0 holds objects, section 1 strings, section 2 type definitions. That
//! is not how a real file is arranged, and deliberately so: nothing in the reader
//! may assume a particular section holds a particular kind of thing.
class CTypedFile
{
public:
	static constexpr uint32_t OBJECTS = 0;
	static constexpr uint32_t STRINGS = 1;
	static constexpr uint32_t TYPES = 2;

	//! Add a string, and hand back where it landed.
	uint32_t AddString( const std::string &s )
	{
		const uint32_t nAt = static_cast<uint32_t>( m_Strings.size() );
		m_Strings.insert( m_Strings.end(), s.begin(), s.end() );
		m_Strings.push_back( 0 );
		return nAt;
	}

	//! Reserve nBytes of object space, zeroed, and hand back where it starts.
	uint32_t AddObject( uint32_t nBytes )
	{
		const uint32_t nAt = static_cast<uint32_t>( m_Objects.size() );
		m_Objects.resize( m_Objects.size() + nBytes, 0 );
		return nAt;
	}

	void PutU32( uint32_t nAt, uint32_t nValue )
	{
		memcpy( m_Objects.data() + nAt, &nValue, sizeof( nValue ) );
	}

	void PutI32( uint32_t nAt, int32_t nValue )
	{
		memcpy( m_Objects.data() + nAt, &nValue, sizeof( nValue ) );
	}

	void PutReal32( uint32_t nAt, float fValue )
	{
		memcpy( m_Objects.data() + nAt, &fValue, sizeof( fValue ) );
	}

	//! One member of a type definition. Written in order; End is added for you.
	struct SMemberSpec
	{
		const char *pszName;
		uint32_t nType;
		int32_t nArrayWidth;
		//! Where the referenced type is, in the types section. 0 means none, which
		//! is why no type is ever placed at offset 0.
		uint32_t nReferenceType;
	};

	//! Write a type definition, and hand back where it starts.
	uint32_t AddType( const std::vector<SMemberSpec> &members )
	{
		if ( m_Types.empty() )
		{
			// Nothing at offset 0, so that 0 can mean "no referenced type".
			m_Types.resize( 32, 0 );
		}
		const uint32_t nAt = static_cast<uint32_t>( m_Types.size() );
		m_Types.resize( m_Types.size() + 32 * ( members.size() + 1 ), 0 );

		for ( size_t i = 0; i < members.size(); ++i )
		{
			const uint32_t nEntry = nAt + static_cast<uint32_t>( i ) * 32;
			PutTypeU32( nEntry, members[i].nType );
			PutTypeI32( nEntry + 12, members[i].nArrayWidth );

			m_Fixups.push_back( SFixup{ TYPES, nEntry + 4, STRINGS,
			                            AddString( members[i].pszName ) } );
			if ( members[i].nReferenceType != 0 )
			{
				m_Fixups.push_back(
					SFixup{ TYPES, nEntry + 8, TYPES, members[i].nReferenceType } );
			}
		}
		// The trailing entry is zeroed, and End is 0.
		return nAt;
	}

	//! A pointer at nFrom in the objects section, leading to nTo.
	void Point( uint32_t nFrom, uint32_t nToSection, uint32_t nToOffset )
	{
		m_Fixups.push_back( SFixup{ OBJECTS, nFrom, nToSection, nToOffset } );
	}

	//! A string member: writes the fixup that makes it point at the text.
	void PointAtString( uint32_t nFrom, const std::string &s )
	{
		m_Fixups.push_back( SFixup{ OBJECTS, nFrom, STRINGS, AddString( s ) } );
	}

	void SetRoot( uint32_t nTypeOffset, uint32_t nObjectOffset )
	{
		m_nRootType = nTypeOffset;
		m_nRootObject = nObjectOffset;
	}

	//! Assemble, and load through the public entry point.
	granny_file *Load()
	{
		CHeaderShapedFile file( 3 );
		file.SetSectionData( OBJECTS, m_Objects );
		file.SetSectionData( STRINGS, m_Strings );
		file.SetSectionData( TYPES, m_Types );
		for ( const SFixup &fixup : m_Fixups )
		{
			file.AddPointerFixup( fixup.nFromSection, fixup.nFromOffset, fixup.nToSection,
			                      fixup.nToOffset );
		}
		file.SetRootObject( OBJECTS, m_nRootObject );
		file.SetU32( OFF_ROOT_OBJECT_TYPE_SECTION, TYPES );
		file.SetU32( OFF_ROOT_OBJECT_TYPE_OFFSET, m_nRootType );

		m_Bytes = file.Bytes();
		return GrannyReadEntireFileFromMemory( static_cast<granny_int32x>( m_Bytes.size() ),
		                                       m_Bytes.data() );
	}

private:
	struct SFixup
	{
		uint32_t nFromSection;
		uint32_t nFromOffset;
		uint32_t nToSection;
		uint32_t nToOffset;
	};

	void PutTypeU32( uint32_t nAt, uint32_t nValue )
	{
		memcpy( m_Types.data() + nAt, &nValue, sizeof( nValue ) );
	}
	void PutTypeI32( uint32_t nAt, int32_t nValue )
	{
		memcpy( m_Types.data() + nAt, &nValue, sizeof( nValue ) );
	}

	std::vector<uint8_t> m_Objects;
	std::vector<uint8_t> m_Strings;
	std::vector<uint8_t> m_Types;
	std::vector<SFixup> m_Fixups;
	std::vector<uint8_t> m_Bytes;
	uint32_t m_nRootType = 0;
	uint32_t m_nRootObject = 0;
};

// The member type numbers the fixtures use, from granny_member_type.
constexpr uint32_t T_INLINE = 1;
constexpr uint32_t T_REFERENCE = 2;
constexpr uint32_t T_REFERENCE_TO_ARRAY = 3;
constexpr uint32_t T_ARRAY_OF_REFERENCES = 4;
constexpr uint32_t T_STRING = 8;
constexpr uint32_t T_TRANSFORM = 9;
constexpr uint32_t T_REAL32 = 10;
constexpr uint32_t T_INT32 = 19;

const NGr2::SFileInfo *Info( granny_file *pFile )
{
	return reinterpret_cast<const NGr2::SFileInfo *>( GrannyGetFileInfo( pFile ) );
}

}

TEST( FileInfo, RefusesNullAndSurvivesAFileWithNoTypeTree )
{
	EXPECT_EQ( nullptr, GrannyGetFileInfo( nullptr ) );

	// A structurally valid file whose root object type points at an empty
	// section, which is what the header-shaped fixture is. There is nothing to
	// walk, so there is nothing to hand back.
	const CHeaderShapedFile bare;
	granny_file *pFile = GrannyReadEntireFileFromMemory(
		static_cast<granny_int32x>( bare.Bytes().size() ), bare.Bytes().data() );
	ASSERT_NE( nullptr, pFile );
	EXPECT_EQ( nullptr, GrannyGetFileInfo( pFile ) );
	GrannyFreeFile( pFile );
}

TEST( FileInfo, ReadsAMinimalRoot )
{
	// The smallest thing that is a granny_file_info: a name and nothing else.
	CTypedFile file;
	const uint32_t nType = file.AddType( { { "FromFileName", T_STRING, 1, 0 } } );
	const uint32_t nRoot = file.AddObject( 4 );
	file.PointAtString( nRoot, "J:/Complete/Something/1.mb" );
	file.SetRoot( nType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );

	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );
	ASSERT_NE( nullptr, pInfo->pszFromFileName );
	EXPECT_STREQ( "J:/Complete/Something/1.mb", pInfo->pszFromFileName );
	EXPECT_EQ( 0, pInfo->nModelCount );
	EXPECT_EQ( nullptr, pInfo->ppModels );
	GrannyFreeFile( pFile );
}

TEST( FileInfo, IsBuiltOnceAndKept )
{
	// The engine calls it per resource load and compares what comes back, so a
	// second call has to be the same object rather than a second conversion.
	CTypedFile file;
	const uint32_t nType = file.AddType( { { "FromFileName", T_STRING, 1, 0 } } );
	const uint32_t nRoot = file.AddObject( 4 );
	file.PointAtString( nRoot, "once" );
	file.SetRoot( nType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pFirst = Info( pFile );
	ASSERT_NE( nullptr, pFirst );
	EXPECT_EQ( pFirst, Info( pFile ) );
	EXPECT_EQ( pFirst, Info( pFile ) );
	GrannyFreeFile( pFile );
}

TEST( FileInfo, ConvertsABoneFromTheFilesOwnNames )
{
	// The version conversion, in the one structure where it bites. The file says
	// Transform, InverseWorldTransform, LightInfo, CameraInfo; granny211.h says
	// LocalTransform, InverseWorld4x4, LODError. Nothing matches by name, so this
	// is the mapping being exercised rather than a coincidence.
	CTypedFile file;
	const uint32_t nBoneType = file.AddType( {
		{ "Name", T_STRING, 1, 0 },
		{ "ParentIndex", T_INT32, 1, 0 },
		{ "Transform", T_TRANSFORM, 1, 0 },
		{ "InverseWorldTransform", T_REAL32, 16, 0 },
		{ "LightInfo", T_REFERENCE, 1, 0 },
		{ "CameraInfo", T_REFERENCE, 1, 0 },
	} );
	const uint32_t nSkeletonType = file.AddType( {
		{ "Name", T_STRING, 1, 0 },
		{ "Bones", T_REFERENCE_TO_ARRAY, 1, nBoneType },
	} );
	const uint32_t nRootType = file.AddType( {
		{ "Skeletons", T_ARRAY_OF_REFERENCES, 1, nSkeletonType },
	} );

	// Two bones, laid out end to end, which is what ReferenceToArray means.
	const uint32_t nBoneSize = 4 + 4 + 68 + 64 + 4 + 4;
	const uint32_t nBones = file.AddObject( nBoneSize * 2 );
	file.PointAtString( nBones, "Root" );
	file.PutI32( nBones + 4, -1 );
	file.PutU32( nBones + 8, 3 );              // Transform.Flags
	file.PutReal32( nBones + 12, 1.5f );       // Transform.Position[0]
	file.PutReal32( nBones + 76, 7.25f );      // InverseWorldTransform[0]
	file.PointAtString( nBones + nBoneSize, "joint1" );
	file.PutI32( nBones + nBoneSize + 4, 0 );
	file.PutReal32( nBones + nBoneSize + 76 + 60, 9.5f );  // InverseWorldTransform[15]

	const uint32_t nSkeleton = file.AddObject( 4 + 8 );
	file.PointAtString( nSkeleton, "Skel" );
	file.PutI32( nSkeleton + 4, 2 );
	file.Point( nSkeleton + 8, CTypedFile::OBJECTS, nBones );

	const uint32_t nRoot = file.AddObject( 8 );
	file.PutI32( nRoot, 1 );
	const uint32_t nSlots = file.AddObject( 4 );
	file.Point( nRoot + 4, CTypedFile::OBJECTS, nSlots );
	file.Point( nSlots, CTypedFile::OBJECTS, nSkeleton );
	file.SetRoot( nRootType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );

	ASSERT_EQ( 1, pInfo->nSkeletonCount );
	const NGr2::SSkeleton *pSkeleton = pInfo->ppSkeletons[0];
	ASSERT_NE( nullptr, pSkeleton );
	EXPECT_STREQ( "Skel", pSkeleton->pszName );
	// Absent from the file, and granny2.dll fills it with 0.
	EXPECT_EQ( 0, pSkeleton->nLODType );
	ASSERT_EQ( 2, pSkeleton->nBoneCount );

	EXPECT_STREQ( "Root", pSkeleton->pBones[0].pszName );
	EXPECT_EQ( -1, pSkeleton->pBones[0].nParentIndex );
	EXPECT_EQ( 3u, pSkeleton->pBones[0].LocalTransform.nFlags ) << "Transform became "
	                                                               "LocalTransform";
	EXPECT_FLOAT_EQ( 1.5f, pSkeleton->pBones[0].LocalTransform.Position[0] );
	EXPECT_FLOAT_EQ( 7.25f, pSkeleton->pBones[0].InverseWorld4x4[0] )
		<< "InverseWorldTransform became InverseWorld4x4";
	// Absent from the file. 0.0, measured out of granny2.dll over 60 files; a
	// first reading of it said 1.0, which was InverseWorld4x4[15] misread.
	EXPECT_FLOAT_EQ( 0.0f, pSkeleton->pBones[0].fLODError );

	EXPECT_STREQ( "joint1", pSkeleton->pBones[1].pszName );
	EXPECT_EQ( 0, pSkeleton->pBones[1].nParentIndex );
	EXPECT_FLOAT_EQ( 9.5f, pSkeleton->pBones[1].InverseWorld4x4[15] )
		<< "the second bone is one whole bone further on";

	GrannyFreeFile( pFile );
}

TEST( FileInfo, GivesOneFileObjectExactlyOneAddress )
{
	// FindFirstAppropriateModel in GObjectInfo.cpp finds a mesh's model by
	// comparing MeshBindings[i].Mesh against a pointer it already holds, so a
	// mesh reached twice has to be one object. Converting it twice would compile,
	// run, and quietly never find a model.
	CTypedFile file;
	const uint32_t nMeshType = file.AddType( { { "Name", T_STRING, 1, 0 } } );
	const uint32_t nBindingType =
		file.AddType( { { "Mesh", T_REFERENCE, 1, nMeshType } } );
	const uint32_t nModelType = file.AddType( {
		{ "Name", T_STRING, 1, 0 },
		{ "MeshBindings", T_REFERENCE_TO_ARRAY, 1, nBindingType },
	} );
	const uint32_t nRootType = file.AddType( {
		{ "Meshes", T_ARRAY_OF_REFERENCES, 1, nMeshType },
		{ "Models", T_ARRAY_OF_REFERENCES, 1, nModelType },
	} );

	const uint32_t nMesh = file.AddObject( 4 );
	file.PointAtString( nMesh, "body" );

	// Two bindings, both pointing at the same mesh.
	const uint32_t nBindings = file.AddObject( 8 );
	file.Point( nBindings, CTypedFile::OBJECTS, nMesh );
	file.Point( nBindings + 4, CTypedFile::OBJECTS, nMesh );

	const uint32_t nModel = file.AddObject( 4 + 8 );
	file.PointAtString( nModel, "model" );
	file.PutI32( nModel + 4, 2 );
	file.Point( nModel + 8, CTypedFile::OBJECTS, nBindings );

	const uint32_t nRoot = file.AddObject( 16 );
	const uint32_t nMeshSlots = file.AddObject( 4 );
	const uint32_t nModelSlots = file.AddObject( 4 );
	file.PutI32( nRoot, 1 );
	file.Point( nRoot + 4, CTypedFile::OBJECTS, nMeshSlots );
	file.PutI32( nRoot + 8, 1 );
	file.Point( nRoot + 12, CTypedFile::OBJECTS, nModelSlots );
	file.Point( nMeshSlots, CTypedFile::OBJECTS, nMesh );
	file.Point( nModelSlots, CTypedFile::OBJECTS, nModel );
	file.SetRoot( nRootType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );

	ASSERT_EQ( 1, pInfo->nMeshCount );
	ASSERT_EQ( 1, pInfo->nModelCount );
	const NGr2::SModel *pModel = pInfo->ppModels[0];
	ASSERT_NE( nullptr, pModel );
	ASSERT_EQ( 2, pModel->nMeshBindingCount );

	EXPECT_EQ( pInfo->ppMeshes[0], pModel->pMeshBindings[0].pMesh )
		<< "the model's mesh is the file's mesh, by address";
	EXPECT_EQ( pModel->pMeshBindings[0].pMesh, pModel->pMeshBindings[1].pMesh )
		<< "and both bindings name the same one";
	EXPECT_STREQ( "body", pModel->pMeshBindings[0].pMesh->pszName );

	GrannyFreeFile( pFile );
}

TEST( FileInfo, MembersTheFileDoesNotHaveTakeTheirDefaults )
{
	// A file whose skeleton carries neither LODType nor ExtendedData, which is
	// every file this game ships. Reading a member by name means an absent one is
	// simply not found, and the default stands.
	CTypedFile file;
	const uint32_t nSkeletonType = file.AddType( { { "Name", T_STRING, 1, 0 } } );
	const uint32_t nRootType =
		file.AddType( { { "Skeletons", T_ARRAY_OF_REFERENCES, 1, nSkeletonType } } );

	const uint32_t nSkeleton = file.AddObject( 4 );
	file.PointAtString( nSkeleton, "bare" );
	const uint32_t nRoot = file.AddObject( 8 );
	const uint32_t nSlots = file.AddObject( 4 );
	file.PutI32( nRoot, 1 );
	file.Point( nRoot + 4, CTypedFile::OBJECTS, nSlots );
	file.Point( nSlots, CTypedFile::OBJECTS, nSkeleton );
	file.SetRoot( nRootType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );
	ASSERT_EQ( 1, pInfo->nSkeletonCount );

	EXPECT_STREQ( "bare", pInfo->ppSkeletons[0]->pszName );
	EXPECT_EQ( 0, pInfo->ppSkeletons[0]->nLODType );
	EXPECT_EQ( 0, pInfo->ppSkeletons[0]->nBoneCount );
	EXPECT_EQ( nullptr, pInfo->ppSkeletons[0]->pBones );
	EXPECT_EQ( nullptr, pInfo->ppSkeletons[0]->ExtendedData.pType );
	GrannyFreeFile( pFile );
}

TEST( FileInfo, ATransformTheFileLacksComesBackAsIdentity )
{
	// Rather than as zeros, which would collapse every model it multiplied.
	CTypedFile file;
	const uint32_t nModelType = file.AddType( { { "Name", T_STRING, 1, 0 } } );
	const uint32_t nRootType =
		file.AddType( { { "Models", T_ARRAY_OF_REFERENCES, 1, nModelType } } );

	const uint32_t nModel = file.AddObject( 4 );
	file.PointAtString( nModel, "m" );
	const uint32_t nRoot = file.AddObject( 8 );
	const uint32_t nSlots = file.AddObject( 4 );
	file.PutI32( nRoot, 1 );
	file.Point( nRoot + 4, CTypedFile::OBJECTS, nSlots );
	file.Point( nSlots, CTypedFile::OBJECTS, nModel );
	file.SetRoot( nRootType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );
	ASSERT_EQ( 1, pInfo->nModelCount );

	const NGr2::STransform &t = pInfo->ppModels[0]->InitialPlacement;
	EXPECT_FLOAT_EQ( 1.0f, t.Orientation[3] );
	EXPECT_FLOAT_EQ( 1.0f, t.ScaleShear[0][0] );
	EXPECT_FLOAT_EQ( 1.0f, t.ScaleShear[1][1] );
	EXPECT_FLOAT_EQ( 1.0f, t.ScaleShear[2][2] );
	EXPECT_FLOAT_EQ( 0.0f, t.ScaleShear[0][1] );
	GrannyFreeFile( pFile );
}

TEST( TypeTree, MemberSizesAreTheOnesGrannyReports )
{
	// GrannyGetTotalObjectSize of a vertex type is the stride the engine steps
	// the file's own vertex bytes with, so these sizes are not bookkeeping: get
	// one wrong and every vertex after the first is read from the wrong place.
	CTypedFile file;
	const uint32_t nVertexType = file.AddType( {
		{ "Position", T_REAL32, 3, 0 },
		{ "Normal", T_REAL32, 3, 0 },
		{ "TextureCoordinates0", T_REAL32, 2, 0 },
	} );
	const uint32_t nDataType = file.AddType( {
		{ "VertexType", T_INLINE, 1, nVertexType },
	} );
	const uint32_t nRootType = file.AddType( {
		{ "FromFileName", T_STRING, 1, 0 },
		{ "Inline", T_INLINE, 1, nDataType },
	} );

	const uint32_t nRoot = file.AddObject( 4 + 32 );
	file.PointAtString( nRoot, "sizes" );
	file.SetRoot( nRootType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );

	const std::vector<NGr2::SMember> *pMembers =
		NGr2::ReadType( *pFile, NGr2::SReference{ CTypedFile::TYPES, nVertexType } );
	ASSERT_NE( nullptr, pMembers );
	ASSERT_EQ( 3u, pMembers->size() );
	EXPECT_EQ( 12u, ( *pMembers )[0].nSize ) << "three floats";
	EXPECT_EQ( 0u, ( *pMembers )[0].nOffset );
	EXPECT_EQ( 12u, ( *pMembers )[1].nOffset );
	EXPECT_EQ( 8u, ( *pMembers )[2].nSize ) << "two floats";
	EXPECT_EQ( 24u, ( *pMembers )[2].nOffset );
	EXPECT_EQ( 32u,
	           NGr2::DiskObjectSize( *pFile, NGr2::SReference{ CTypedFile::TYPES,
	                                                           nVertexType } ) );

	// An inline member is as big as the type it names, recursively.
	EXPECT_EQ( 32u, NGr2::DiskObjectSize(
						*pFile, NGr2::SReference{ CTypedFile::TYPES, nDataType } ) );
	EXPECT_EQ( 36u, NGr2::DiskObjectSize(
						*pFile, NGr2::SReference{ CTypedFile::TYPES, nRootType } ) );

	GrannyFreeFile( pFile );
}

TEST( TypeTree, RefusesATypeWithNoEndMarker )
{
	// The loop that reads members stops at End or at the section's end, and a
	// type definition running off the end of its section is the second case.
	CHeaderShapedFile file;
	std::vector<uint8_t> types( 64, 0 );
	// A member whose type is real and whose End marker is past the section.
	types[0] = 10;
	file.SetSectionData( 2, types );
	file.SetU32( OFF_ROOT_OBJECT_TYPE_SECTION, 2 );
	file.SetU32( OFF_ROOT_OBJECT_TYPE_OFFSET, 60 );

	granny_file *pFile = GrannyReadEntireFileFromMemory(
		static_cast<granny_int32x>( file.Bytes().size() ), file.Bytes().data() );
	ASSERT_NE( nullptr, pFile );
	EXPECT_EQ( nullptr, GrannyGetFileInfo( pFile ) );
	GrannyFreeFile( pFile );
}

TEST( TypeTree, SizeEntryPointsSurviveNull )
{
	// Both are called straight from engine code with whatever it has.
	EXPECT_EQ( 0, GrannyGetMemberTypeSize( nullptr ) );
	EXPECT_EQ( 0, GrannyGetTotalObjectSize( nullptr ) );
}
