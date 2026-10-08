#include "ED_B2_M1/stdafx.h"
#include "MapEditorLib/BuilderFactory.h"
#include "MapEditorLib/Interface_Builder.h"
#include "MapEditorLib/Interface_MOD.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditor/CommandHandlerContainer.h"
#include "MapEditor/FolderCallback.h"
#include "MapEditor/ViewContainer.h"
#include "Stats_B2_M1/ActionCommand.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ResourceManager.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <utility>
#include <gtest/gtest.h>

namespace
{
namespace fs = std::filesystem;

class CTestLog : public NLog::ILoggerSink, public ILogger
{
	OBJECT_NOCOPY_METHODS( CTestLog );
public:
	ILogger *GetLogger() override { return this; }
	void Log( ELogOutputType, const std::string &text ) override { std::fputs(text.c_str(), stdout); }
	void ClearLog() override {}
};

class CTestUserData : public IUserDataContainer
{
	OBJECT_NOCOPY_METHODS( CTestUserData );
public:
	SUserData data;
	SUserData *Get() override { return &data; }
	void Load() override {}
	void Save() override {}
};

class CTestMod : public IMODContainer
{
	OBJECT_NOCOPY_METHODS( CTestMod );
public:
	std::string folder;
	bool CanNewMOD() override { return false; }
	bool CanOpenMOD() override { return false; }
	bool CanCloseMOD() override { return false; }
	bool NewMOD() override { return false; }
	bool OpenMOD() override { return false; }
	void CloseMOD() override {}
	std::string GetDataFolder( SUserData::ENormalizePathType ) override { return folder; }
};

// Supply the dialog's accepted settings; all resource creation uses production code.
class CTestBuilderContainer : public IBuilderContainer
{
	OBJECT_NOCOPY_METHODS( CTestBuilderContainer );
public:
	bool CanBuildObject( const std::string & ) override { return true; }
	bool CanDefaultBuildObject( const std::string & ) override { return false; }
	void Create( const std::string & ) override {}
	void Destroy( const std::string & ) override {}
	bool InsertObject( std::string *, std::string *, bool, bool *, bool *, bool * ) override { return false; }
	bool CopyObject( const std::string &, const std::string &, const std::string & ) override { return false; }
	bool RenameObject( const std::string &, const std::string &, const std::string & ) override { return false; }
	bool RemoveObject( const std::string &, const std::string & ) override { return false; }
	void GetDefaultFolder( const std::string &, std::string * ) override {}
	bool FillBuildData( std::string *, std::string *name, SBuildDataParams *, IBuildDataCallback * ) override
	{
		*name = "builder.xdb";
		return true;
	}
	bool FillNewObjectName( SBuildDataParams * ) override { return false; }
};

class VisObjBuilder : public testing::Test
{
protected:
	fs::path root;
	CObj<NVFS::IVFS> savedVFS;
	CObj<NVFS::IFileCreator> savedCreator;
	CPtr<IBuilder> builder;

	void SetUp() override
	{
		NLog::SetLogger(new CTestLog);
		// Loading Stats registers VisObj, while 3Dmotor registers its dependencies.
		ASSERT_EQ( GetCommandByAction(NDb::USER_ACTION_MOVE), ACTION_COMMAND_MOVE_TO );
		root = fs::temp_directory_path() / ("obk2-visobj-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		fs::create_directories(root);
		fs::copy_file(VISOBJ_SCHEMA, root / "types.xml");
		fs::copy_file(VISOBJ_MODEL, root / "model.glb");
		// One opaque, uncompressed 24-bit TGA pixel.
		const unsigned char tga[] = {0,0,2,0,0,0,0,0,0,0,0,0,1,0,1,0,24,0,255,255,255};
		std::ofstream(root / "texture.tga", std::ios::binary).write(reinterpret_cast<const char *>(tga), sizeof(tga));
		std::ofstream(root / "builder.xdb") <<
			"<VisObjBuilder><ModelFileName href=\"/model.glb\"/><TextureFileName href=\"/texture.tga\"/>"
			"<TextureType>AM_OPAQUE</TextureType><RootMesh>Basis</RootMesh><RootJoint>Basis</RootJoint>"
			"<AIRootMesh>AABB</AIRootMesh><Skeleton href=\"\"/></VisObjBuilder>";
		const std::string folder = root.generic_u8string() + "/";
		savedVFS = NVFS::GetMainVFS();
		savedCreator = NVFS::GetMainFileCreator();
		NVFS::SetMainVFS(NVFS::CreateWinVFS(folder));
		NVFS::SetMainFileCreator(NVFS::CreateWinFileCreator(folder));
		ASSERT_TRUE( NDb::OpenDatabase(NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR) );
		IResourceManager::InitSingleton();
		auto *user = new CTestUserData;
		user->data.constUserData.szExportSourceFolder = folder;
		user->data.constUserData.szExportDestinationFolder = folder;
		NSingleton::RegisterSingleton(user, IUserDataContainer::tidTypeID);
		auto *mod = new CTestMod;
		mod->folder = folder;
		NSingleton::RegisterSingleton(mod, IMODContainer::tidTypeID);
		NSingleton::RegisterSingleton(new CCommandHandlerContainer, ICommandHandlerContainer::tidTypeID);
		NSingleton::RegisterSingleton(new CViewContainer, IViewContainer::tidTypeID);
		NSingleton::RegisterSingleton(new CFolderCallback, IFolderCallback::tidTypeID);
		NSingleton::RegisterSingleton(new CTestBuilderContainer, IBuilderContainer::tidTypeID);
		builder = NBuilderFactory::CreateBuilder("VisObj");
		ASSERT_NE( builder, nullptr );
	}

	void TearDown() override
	{
		builder = nullptr;
		const unsigned ids[] = { IBuilderContainer::tidTypeID, IFolderCallback::tidTypeID, IViewContainer::tidTypeID,
			ICommandHandlerContainer::tidTypeID, IMODContainer::tidTypeID, IUserDataContainer::tidTypeID };
		for ( unsigned id : ids )
			NSingleton::UnRegisterSingleton(id);
		IResourceManager::UninitSingleton();
		NLog::SetLogger(nullptr);
		NDb::CloseDatabase();
		NVFS::SetMainVFS(savedVFS);
		NVFS::SetMainFileCreator(savedCreator);
		std::error_code error;
		fs::remove_all(root, error);
	}

	void CreateAndCheck( const char *object = "visualObject.xdb" )
	{
		std::string type = "VisObj", name = std::string("Unit/") + object;
		bool canRename = true, exportObject = false, edit = false;
		ASSERT_TRUE( builder->InsertObject(&type, &name, false, &canRename, &exportObject, &edit) );
		CPtr<IManipulator> visual = Singleton<IResourceManager>()->CreateObjectManipulator(type, name);
		ASSERT_NE( visual, nullptr );
		int count = 0;
		ASSERT_TRUE( CManipulatorManager::GetValue(&count, visual, "Models") );
		EXPECT_EQ( count, 6 );
		CPtr<IManipulator> model = CManipulatorManager::CreateManipulatorFromReference("Models.[0].Model", visual, nullptr, nullptr, nullptr);
		ASSERT_NE( model, nullptr );
		for ( const char *field : { "Geometry", "Skeleton", "Materials.[0]" } )
		{
			CPtr<IManipulator> child = CManipulatorManager::CreateManipulatorFromReference(field, model, nullptr, nullptr, nullptr);
			ASSERT_NE( child, nullptr ) << field;
		}
		CPtr<IManipulator> geometry = CManipulatorManager::CreateManipulatorFromReference("Geometry", model, nullptr, nullptr, nullptr);
		CPtr<IManipulator> aiGeometry = CManipulatorManager::CreateManipulatorFromReference("AIGeometry", geometry, nullptr, nullptr, nullptr);
		CPtr<IManipulator> material = CManipulatorManager::CreateManipulatorFromReference("Materials.[0]", model, nullptr, nullptr, nullptr);
		CPtr<IManipulator> texture = CManipulatorManager::CreateManipulatorFromReference("Texture", material, nullptr, nullptr, nullptr);
		ASSERT_NE( aiGeometry, nullptr );
		ASSERT_NE( texture, nullptr );
		std::string value;
		ASSERT_TRUE( CManipulatorManager::GetValue(&value, aiGeometry, "RootMesh") );
		EXPECT_EQ( value, "AABB" );
		ASSERT_TRUE( CManipulatorManager::GetValue(&value, geometry, "ModelFileRef") );
		EXPECT_EQ( CDBID(value), CDBID("model.glb") );
		ASSERT_TRUE( CManipulatorManager::GetValue(&value, texture, "SrcName") );
		EXPECT_EQ( CDBID(value), CDBID("texture.tga") );
		NDb::SaveChanges();
		EXPECT_TRUE( fs::is_regular_file(root / "Unit" / object) );
		for ( const char *file : { "model_texture_Model.xdb", "texture_Material.xdb",
			"texture_Texture.xdb", "model_Geometry.xdb", "model_AIGeometry.xdb", "model_Skeleton.xdb" } )
			EXPECT_TRUE( fs::is_regular_file(root / "Unit" / file) ) << file;
	}
};

TEST_F( VisObjBuilder, CreatesCompleteResourceGraph )
{
	CreateAndCheck();
}

TEST_F( VisObjBuilder, RecreatesDependenciesWhoseFilesWereDeleted )
{
	fs::create_directories(root / "Unit");
	for ( const auto &resource : { std::pair<const char *, const char *>{"Model", "model_texture_Model.xdb"},
		{"Material", "texture_Material.xdb"}, {"Texture", "texture_Texture.xdb"}, {"Geometry", "model_Geometry.xdb"},
		{"AIGeometry", "model_AIGeometry.xdb"}, {"Skeleton", "model_Skeleton.xdb"} } )
	{
		const std::string name = std::string("Unit/") + resource.second;
		std::ofstream(root / name) << "<" << resource.first << "/>";
		ASSERT_TRUE( NDb::RegisterResourceFile(name) );
		fs::remove(root / name);
		ASSERT_TRUE( NDb::DoesObjectExist(CDBID(name)) );
	}
	CreateAndCheck();
}

TEST_F( VisObjBuilder, RestoresDeletedFilesForCachedDependencies )
{
	ASSERT_NO_FATAL_FAILURE( CreateAndCheck() );
	ASSERT_TRUE( fs::remove(root / "Unit/texture_Texture.xdb") );
	ASSERT_TRUE( fs::remove(root / "Unit/model_AIGeometry.xdb") );
	CreateAndCheck("second.xdb");
}

TEST_F( VisObjBuilder, RecreatesMissingChildrenOfExistingResourcesAfterReload )
{
	ASSERT_NO_FATAL_FAILURE( CreateAndCheck() );
	NDb::CloseDatabase();
	ASSERT_TRUE( fs::remove(root / "Unit/texture_Texture.xdb") );
	ASSERT_TRUE( fs::remove(root / "Unit/model_AIGeometry.xdb") );
	ASSERT_TRUE( NDb::OpenDatabase(NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR) );
	CreateAndCheck("second.xdb");
}
}
