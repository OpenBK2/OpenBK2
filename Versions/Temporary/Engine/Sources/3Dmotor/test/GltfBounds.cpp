// Stored geometry bounds must survive loading even when a model has different
// bounds. This exercises the DLL's real XML and binary serializers without a GPU.
#include "3Dmotor/stdafx.h"
#include "3Dmotor/DBScene.h"
#include "3Dmotor/GltfFormat.h"
#include "System/XmlSaver.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <gtest/gtest.h>

namespace
{
class CCountingVFS : public NVFS::IVFS
{
	OBJECT_NOCOPY_METHODS( CCountingVFS );
public:
	CObj<NVFS::IVFS> source;
	int accesses = 0;
	CDataStream *OpenFile( const std::string &path ) override
	{ ++accesses; return source->OpenFile(path); }
	bool DoesFileExist( const std::string &path ) override
	{ ++accesses; return source->DoesFileExist(path); }
	bool GetFileStats( NVFS::SFileStats *stats, const std::string &path ) override
	{ ++accesses; return source->GetFileStats(stats, path); }
	void GetAllFileNames( std::vector<std::string> *names, const std::string &folder ) override
	{ ++accesses; source->GetAllFileNames(names, folder); }
};

class GltfBounds : public testing::Test
{
	CObj<NVFS::IVFS> savedVFS = NVFS::GetMainVFS();
protected:
	CObj<CCountingVFS> vfs = new CCountingVFS;
	void SetUp() override
	{
		vfs->source = NVFS::CreateWinVFS(GLTF_EXAMPLE_DIR "/ExampleTankUnit/");
		NVFS::SetMainVFS(vfs);
		// Prove the referenced source can be loaded and differs from our saved values.
		const auto file = NGltf::LoadFile(nullptr, "model.glb");
		ASSERT_TRUE(file);
		CVec3 minimum, maximum;
		ASSERT_TRUE(NGltf::GetMeshBoundingBox(file, "Basis", false, &minimum, &maximum));
		EXPECT_NE((minimum + maximum).x * 0.5f, 11.25f);
		vfs->accesses = 0;
	}
	void TearDown() override { NVFS::SetMainVFS(savedVFS); }
};

void ExpectVec( const CVec3 &actual, const CVec3 &expected )
{
	EXPECT_EQ(actual.x, expected.x);
	EXPECT_EQ(actual.y, expected.y);
	EXPECT_EQ(actual.z, expected.z);
}

template<class T> CObj<T> MakeResource()
{
	// Construct through the DLL's registered factory, so all virtual calls exercise
	// production serializers and do not need test-only Windows exports.
	return dynamic_cast<T *>(NObjectFactory::MakeObject(T::typeID));
}

void SetBounds( NDb::SGeometry *resource )
{
	resource->uid = {};
	resource->vCenter = CVec3(11.25f, -2.5f, 3.75f);
	resource->vSize = CVec3(4.5f, 6.25f, 8.0f);
	resource->szRootMesh = "Basis";
}
void SetBounds( NDb::SAIGeometry *resource )
{
	resource->uid = {};
	resource->vAABBCenter = CVec3(11.25f, -2.5f, 3.75f);
	resource->vAABBHalfSize = CVec3(4.5f, 6.25f, 8.0f);
	resource->szRootMesh = "AABB";
}
void ExpectBounds( const NDb::SGeometry *resource )
{
	ExpectVec(resource->vCenter, CVec3(11.25f, -2.5f, 3.75f));
	ExpectVec(resource->vSize, CVec3(4.5f, 6.25f, 8.0f));
}
void ExpectBounds( const NDb::SAIGeometry *resource )
{
	ExpectVec(resource->vAABBCenter, CVec3(11.25f, -2.5f, 3.75f));
	ExpectVec(resource->vAABBHalfSize, CVec3(4.5f, 6.25f, 8.0f));
}

template<class T> void CheckRoundTrip( bool xml, bool wide = false )
{
	for ( const char *reference : { "model.glb", "missing.gltf", "" } )
	{
		SCOPED_TRACE(reference);
		CObj<T> source = MakeResource<T>();
		CObj<T> loaded = MakeResource<T>();
		ASSERT_TRUE(source && loaded);
		SetBounds(source.GetPtr());
		source->szModelFileRef = reference;
		CMemoryStream stream;
		if ( xml )
		{
			{
				CObj<IXmlSaver> saver = CreateXmlSaver(&stream, SAVER_MODE_WRITE);
				saver->Add("Resource", static_cast<CXmlResource *>(source.GetPtr()));
			}
			stream.Seek(0);
			CObj<IXmlSaver> saver = CreateXmlSaver(&stream, SAVER_MODE_READ);
			saver->Add("Resource", static_cast<CXmlResource *>(loaded.GetPtr()));
		}
		else
		{
			{
				CObj<IBinSaver> saver = CreateBinSaver(&stream, wide ? SAVER_MODE_WRITE_64 : SAVER_MODE_WRITE);
				saver->AddPolymorphicBase(1, source.GetPtr());
			}
			stream.Seek(0);
			CObj<IBinSaver> saver = CreateBinSaver(&stream, wide ? SAVER_MODE_READ_64 : SAVER_MODE_READ);
			saver->AddPolymorphicBase(1, loaded.GetPtr());
		}
		ExpectBounds(loaded.GetPtr());
		EXPECT_EQ(loaded->szModelFileRef, reference);
		EXPECT_EQ(loaded->szRootMesh, source->szRootMesh);
	}
}
}

TEST_F( GltfBounds, XmlPreservesExportedBoundsWithoutReadingTheModel )
{
	CheckRoundTrip<NDb::SGeometry>(true);
	CheckRoundTrip<NDb::SAIGeometry>(true);
	EXPECT_EQ(vfs->accesses, 0);
}

TEST_F( GltfBounds, BinaryPreservesExportedBoundsWithoutReadingTheModel )
{
	for ( bool wide : { false, true } )
	{
		CheckRoundTrip<NDb::SGeometry>(false, wide);
		CheckRoundTrip<NDb::SAIGeometry>(false, wide);
	}
	EXPECT_EQ(vfs->accesses, 0);
}

namespace
{
struct SExportedBounds
{
	bool ai = false;
	CVec3 center = VNULL3, size = VNULL3;
	std::string root;
	int meshes = 0;
	int operator&( IXmlSaver &saver )
	{
		saver.Add(ai ? "AABBCenter" : "Center", &center);
		saver.Add(ai ? "AABBHalfSize" : "Size", &size);
		saver.Add("RootMesh", &root);
		saver.Add("NumMeshes", &meshes);
		return 0;
	}
};
}

TEST_F( GltfBounds, BundledExamplesContainExportedBounds )
{
	for ( const char *example : { "ExampleTankUnit", "ExampleDinosaurUnit" } )
	{
		SCOPED_TRACE(example);
		NVFS::SetMainVFS(NVFS::CreateWinVFS(std::string(GLTF_EXAMPLE_DIR) + "/" + example + "/"));
		const auto file = NGltf::LoadFile(nullptr,
			std::string(example) == "ExampleTankUnit" ? "model.glb" : "trex_split.glb");
		ASSERT_TRUE(file);
		for ( bool ai : { false, true } )
		{
			SCOPED_TRACE(ai);
			SExportedBounds stored;
			stored.ai = ai;
			CFileStream stream(NVFS::GetMainVFS(), ai ? "1_AIGeometry.xdb" : "1_Geometry.xdb");
			ASSERT_TRUE(stream.IsOk());
			CObj<IXmlSaver> saver = CreateXmlSaver(&stream, SAVER_MODE_READ);
			saver->Add(nullptr, &stored);
			ASSERT_FALSE(stored.root.empty());
			CVec3 minimum, maximum;
			ASSERT_TRUE(NGltf::GetMeshBoundingBox(file, stored.root, ai, &minimum, &maximum));
			const CVec3 center = (minimum + maximum) * 0.5f;
			const CVec3 size = (maximum - minimum) * (ai ? 0.5f : 1.0f);
			std::vector<size_t> nodes;
			ASSERT_TRUE(NGltf::GetMeshNodes(file, stored.root, &nodes));
			// Export can run on another architecture; the loaded values themselves
			// must be exact (above), while this source comparison allows rounding.
			EXPECT_NEAR(stored.center.x, center.x, 0.0001f);
			EXPECT_NEAR(stored.center.y, center.y, 0.0001f);
			EXPECT_NEAR(stored.center.z, center.z, 0.0001f);
			EXPECT_NEAR(stored.size.x, size.x, 0.0001f);
			EXPECT_NEAR(stored.size.y, size.y, 0.0001f);
			EXPECT_NEAR(stored.size.z, size.z, 0.0001f);
			if ( !ai ) EXPECT_EQ(stored.meshes, static_cast<int>(nodes.size()));
		}
	}
}
