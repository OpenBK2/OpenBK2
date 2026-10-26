#include "ED_B2_M1/stdafx.h"

#include "B2_MapEditor/VFSDbObserver.h"
#include "ED_Common/GltfImport.h"
#include "GameX/AICmdsAutoMagic.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ResourceManager.h"
#include "MapEditor/BuildDataView.h"
#include "MapEditor/CommandHandlerContainer.h"
#include "MapEditor/FolderCallback.h"
#include "MapEditor/ObjectBrowserView.h"
#include "MapEditor/PropertyPaneView.h"
#include "MapEditor/ViewContainer.h"
#include "MapEditorLib/BuilderFactory.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/Interface_Builder.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_MOD.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/ObjectController.h"
#include "MapEditorLib/SourceFilePath.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/Tools_Resources.h"
#include "Stats_B2_M1/ActionCommand.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>

#include <gtest/gtest.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/propgrid/manager.h>

namespace
{
namespace fs = std::filesystem;

class CTestLog : public NLog::ILoggerSink, public ILogger
{
	OBJECT_NOCOPY_METHODS( CTestLog );

public:
	ILogger *GetLogger() override
	{
		return this;
	}

	void Log( ELogOutputType, const std::string &text ) override
	{
		std::fputs( text.c_str(), stdout );
	}

	void ClearLog() override {}
};

class CTestUserData : public IUserDataContainer
{
	OBJECT_NOCOPY_METHODS( CTestUserData );

public:
	SUserData data;

	SUserData *Get() override
	{
		return &data;
	}

	void Load() override {}

	void Save() override {}
};

class CTestMod : public IMODContainer
{
	OBJECT_NOCOPY_METHODS( CTestMod );

public:
	std::string folder;

	bool CanNewMOD() override
	{
		return false;
	}

	bool CanOpenMOD() override
	{
		return false;
	}

	bool CanCloseMOD() override
	{
		return false;
	}

	bool NewMOD() override
	{
		return false;
	}

	bool OpenMOD() override
	{
		return false;
	}

	void CloseMOD() override {}

	std::string GetDataFolder( SUserData::ENormalizePathType ) override
	{
		return folder;
	}
};

// Supply the dialog's accepted settings; all resource creation uses production code.
class CTestBuilderContainer : public IBuilderContainer
{
	OBJECT_NOCOPY_METHODS( CTestBuilderContainer );

public:
	bool CanBuildObject( const std::string & ) override
	{
		return true;
	}

	bool CanDefaultBuildObject( const std::string & ) override
	{
		return false;
	}

	void Create( const std::string & ) override {}

	void Destroy( const std::string & ) override {}

	bool InsertObject( std::string *, std::string *, bool, bool *, bool *, bool * ) override
	{
		return false;
	}

	bool CopyObject( const std::string &, const std::string &, const std::string & ) override
	{
		return false;
	}

	bool RenameObject( const std::string &, const std::string &, const std::string & ) override
	{
		return false;
	}

	bool RemoveObject( const std::string &, const std::string & ) override
	{
		return false;
	}

	void GetDefaultFolder( const std::string &, std::string * ) override {}

	bool FillBuildData( std::string *, std::string *name, SBuildDataParams *, IBuildDataCallback * ) override
	{
		*name = "builder.xdb";
		return true;
	}

	bool FillNewObjectName( SBuildDataParams * ) override
	{
		return false;
	}
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
		NLog::SetLogger( new CTestLog );
		// Loading Stats registers VisObj, while 3Dmotor registers its dependencies.
		ASSERT_EQ( GetCommandByAction( NDb::USER_ACTION_MOVE ), ACTION_COMMAND_MOVE_TO );
		root = fs::temp_directory_path() /
		       ( "obk2-visobj-" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
		fs::create_directories( root );
		fs::copy_file( VISOBJ_SCHEMA, root / "types.xml" );
		fs::copy_file( VISOBJ_MODEL, root / "model.glb" );
		// One opaque, uncompressed 24-bit TGA pixel.
		const unsigned char tga[] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 24, 0, 255, 255, 255 };
		std::ofstream( root / "texture.tga", std::ios::binary )
				.write( reinterpret_cast<const char *>( tga ), sizeof( tga ) );
		std::ofstream( root / "builder.xdb" )
				<< "<VisObjBuilder><ModelFileName href=\"/model.glb\"/><TextureFileName href=\"/texture.tga\"/>"
					 "<TextureType>AM_OPAQUE</TextureType><RootMesh>Basis</RootMesh><RootJoint>Basis</RootJoint>"
					 "<AIRootMesh>AABB</AIRootMesh><Skeleton href=\"\"/></VisObjBuilder>";
		const std::string folder = root.generic_u8string() + "/";
		savedVFS = NVFS::GetMainVFS();
		savedCreator = NVFS::GetMainFileCreator();
		NVFS::SetMainVFS( NVFS::CreateWinVFS( folder ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( folder ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		IResourceManager::InitSingleton();
		auto *user = new CTestUserData;
		user->data.constUserData.szExportSourceFolder = folder;
		user->data.constUserData.szExportDestinationFolder = folder;
		NSingleton::RegisterSingleton( user, IUserDataContainer::tidTypeID );
		auto *mod = new CTestMod;
		mod->folder = folder;
		NSingleton::RegisterSingleton( mod, IMODContainer::tidTypeID );
		NSingleton::RegisterSingleton( new CCommandHandlerContainer, ICommandHandlerContainer::tidTypeID );
		NSingleton::RegisterSingleton( new CViewContainer, IViewContainer::tidTypeID );
		NSingleton::RegisterSingleton( new CFolderCallback, IFolderCallback::tidTypeID );
		NSingleton::RegisterSingleton( new CTestBuilderContainer, IBuilderContainer::tidTypeID );
		builder = NBuilderFactory::CreateBuilder( "VisObj" );
		ASSERT_NE( builder, nullptr );
	}

	void TearDown() override
	{
		builder = nullptr;
		const unsigned ids[] = { IBuilderContainer::tidTypeID,
		                         IFolderCallback::tidTypeID,
		                         IViewContainer::tidTypeID,
		                         ICommandHandlerContainer::tidTypeID,
		                         IMODContainer::tidTypeID,
		                         IUserDataContainer::tidTypeID };
		for ( unsigned id : ids )
		{
			NSingleton::UnRegisterSingleton( id );
		}
		IResourceManager::UninitSingleton();
		NLog::SetLogger( nullptr );
		NDb::CloseDatabase();
		NVFS::SetMainVFS( savedVFS );
		NVFS::SetMainFileCreator( savedCreator );
		std::error_code error;
		fs::remove_all( root, error );
	}

	void CreateAndCheck( const char *object = "visualObject.xdb" )
	{
		std::string type = "VisObj", name = std::string( "Unit/" ) + object;
		bool canRename = true, exportObject = false, edit = false;
		ASSERT_TRUE( builder->InsertObject( &type, &name, false, &canRename, &exportObject, &edit ) );
		CPtr<IManipulator> visual = Singleton<IResourceManager>()->CreateObjectManipulator( type, name );
		ASSERT_NE( visual, nullptr );
		int count = 0;
		ASSERT_TRUE( CManipulatorManager::GetValue( &count, visual, "Models" ) );
		EXPECT_EQ( count, 6 );
		CPtr<IManipulator> model =
				CManipulatorManager::CreateManipulatorFromReference( "Models.[0].Model", visual, nullptr, nullptr, nullptr );
		ASSERT_NE( model, nullptr );
		for ( const char *field : { "Geometry", "Skeleton", "Materials.[0]" } )
		{
			CPtr<IManipulator> child =
					CManipulatorManager::CreateManipulatorFromReference( field, model, nullptr, nullptr, nullptr );
			ASSERT_NE( child, nullptr ) << field;
		}
		CPtr<IManipulator> geometry =
				CManipulatorManager::CreateManipulatorFromReference( "Geometry", model, nullptr, nullptr, nullptr );
		CPtr<IManipulator> aiGeometry =
				CManipulatorManager::CreateManipulatorFromReference( "AIGeometry", geometry, nullptr, nullptr, nullptr );
		CPtr<IManipulator> material =
				CManipulatorManager::CreateManipulatorFromReference( "Materials.[0]", model, nullptr, nullptr, nullptr );
		CPtr<IManipulator> texture =
				CManipulatorManager::CreateManipulatorFromReference( "Texture", material, nullptr, nullptr, nullptr );
		ASSERT_NE( aiGeometry, nullptr );
		ASSERT_NE( texture, nullptr );
		std::string value;
		ASSERT_TRUE( CManipulatorManager::GetValue( &value, aiGeometry, "RootMesh" ) );
		EXPECT_EQ( value, "AABB" );
		ASSERT_TRUE( CManipulatorManager::GetValue( &value, geometry, "ModelFileRef" ) );
		EXPECT_EQ( CDBID( value ), CDBID( "model.glb" ) );
		ASSERT_TRUE( CManipulatorManager::GetValue( &value, texture, "SrcName" ) );
		EXPECT_EQ( CDBID( value ), CDBID( "texture.tga" ) );
		NDb::SaveChanges();
		EXPECT_TRUE( fs::is_regular_file( root / "Unit" / object ) );
		for ( const char *file : { "model_texture_Model.xdb",
		                           "texture_Material.xdb",
		                           "texture_Texture.xdb",
		                           "model_Geometry.xdb",
		                           "model_AIGeometry.xdb",
		                           "model_Skeleton.xdb" } )
		{
			EXPECT_TRUE( fs::is_regular_file( root / "Unit" / file ) ) << file;
		}
	}
};

TEST_F( VisObjBuilder, CreatesCompleteResourceGraph )
{
	CreateAndCheck();
}

TEST_F( VisObjBuilder, PickerPathsSurvivePropertyParsing )
{
	CPtr<IManipulator> settings =
			Singleton<IResourceManager>()->CreateObjectManipulator( "VisObjBuilder", CDBID( "builder.xdb" ) );
	ASSERT_NE( settings, nullptr );
	// Native Windows pickers return a drive-letter path, including for external models.
	for ( const char *path : { "C:/Authoring/model.glb", "/authoring/model.gltf", "Models/model.glb" } )
	{
		CVariant value;
		ASSERT_TRUE( NPropertyPane::ParseValueText( settings, "ModelFileName", path, &value ) );
		ASSERT_TRUE( settings->SetValue( "ModelFileName", value ) );
		std::string stored;
		ASSERT_TRUE( CManipulatorManager::GetValue( &stored, settings, "ModelFileName" ) );
		EXPECT_EQ( stored, path );
	}
	EXPECT_FALSE( IsValidFileName( "C:/invalid:name.glb", true ) );
	EXPECT_FALSE( IsValidFileName( "invalid:name.glb", true ) );
}

TEST_F( VisObjBuilder, PathsAreRelativeToModDataOrSourceAndResolveBack )
{
	auto &data = Singleton<IUserDataContainer>()->Get()->constUserData;
	data.szDataStorageFolder = ( root / "Data" ).generic_u8string();
	data.szExportSourceFolder = ( root / "Source" ).generic_u8string();
	auto *mod = static_cast<CTestMod *>( Singleton<IMODContainer>() );
	mod->folder = ( root / "Mod" ).generic_u8string();
	// The fixture's original root must not hide the more specific folders.
	data.szExportDestinationFolder = data.szDataStorageFolder;
	for ( const char *folder : { "Mod", "Data", "Source" } )
	{
		const auto relative = fs::path( "Models" ) / ( std::string( folder ) + ".glb" );
		const auto file = root / folder / relative;
		fs::create_directories( file.parent_path() );
		std::ofstream( file ) << "model";
		EXPECT_EQ( NEditorSource::MakeRelative( file.generic_u8string() ), relative.generic_u8string() );
		EXPECT_EQ( NEditorSource::Resolve( relative.generic_u8string() ), file.generic_u8string() );
	}
#if BOOST_OS_WINDOWS
	EXPECT_EQ( NEditorSource::MakeRelative( ( root / "dAtA" / "Models" / "Data.glb" ).generic_u8string() ),
	           "Models/Data.glb" );
#endif
	const auto outside = ( root / "DataExtra" / "external.glb" ).generic_u8string();
	EXPECT_EQ( NEditorSource::MakeRelative( outside ), outside );
}

TEST_F( VisObjBuilder, ImportKeepsBaseDataRelativePathInActiveMod )
{
	auto &data = Singleton<IUserDataContainer>()->Get()->constUserData;
	data.szDataStorageFolder = ( root / "Data" ).generic_u8string() + "/";
	auto *mod = static_cast<CTestMod *>( Singleton<IMODContainer>() );
	mod->folder = ( root / "Mod" ).generic_u8string() + "/";
	fs::create_directories( root / "Data" / "Models" );
	const auto source = root / "Data" / "Models" / "tank.glb";
	fs::copy_file( VISOBJ_MODEL, source );
	std::string reference, error;
	ASSERT_TRUE( NEditorGltf::ImportModelFile( "Objects/tank.xdb", source.generic_u8string(), &reference, &error ) )
			<< error;
	EXPECT_EQ( CDBID( reference ), CDBID( "Models/tank.glb" ) );
	EXPECT_TRUE( fs::is_regular_file( root / "Mod" / "Models" / "tank.glb" ) );
}

TEST_F( VisObjBuilder, ValidationReportsMissingModelTextureAndSkeleton )
{
	CPtr<IManipulator> settings =
			Singleton<IResourceManager>()->CreateObjectManipulator( "VisObjBuilder", CDBID( "builder.xdb" ) );
	ASSERT_NE( settings, nullptr );
	auto *callback = dynamic_cast<IBuildDataCallback *>( builder.GetPtr() );
	ASSERT_NE( callback, nullptr );
	std::string error;
	ASSERT_TRUE( callback->IsValidBuildData( settings, &error, nullptr ) ) << error;
	for ( const auto &field :
	      { std::make_pair( "ModelFileName", "missing.glb" ), std::make_pair( "TextureFileName", "missing.tga" ) } )
	{
		CVariant old;
		ASSERT_TRUE( settings->GetValue( field.first, &old ) );
		ASSERT_TRUE( settings->SetValue( field.first, field.second ) );
		EXPECT_FALSE( callback->IsValidBuildData( settings, &error, nullptr ) );
		EXPECT_NE( error.find( field.first ), std::string::npos ) << error;
		ASSERT_TRUE( settings->SetValue( field.first, old ) );
	}
	// Assign while the reference is valid, then remove its resource while the dialog is open.
	ASSERT_TRUE( Singleton<IFolderCallback>()->InsertObject( "Skeleton", "shared.xdb" ) );
	ASSERT_TRUE( settings->SetValue( "Skeleton", "shared.xdb" ) );
	ASSERT_TRUE( callback->IsValidBuildData( settings, &error, nullptr ) ) << error;
	// Editor-only references must still follow a rename before their target is deleted.
	ASSERT_TRUE( Singleton<IFolderCallback>()->RenameObject( "Skeleton", "renamed.xdb", "shared.xdb" ) );
	CVariant reference;
	ASSERT_TRUE( settings->GetValue( "Skeleton", &reference ) );
	EXPECT_EQ( reference.GetDBID(), CDBID( "renamed.xdb" ) );
	ASSERT_TRUE( NDb::RemoveObject( CDBID( "renamed.xdb" ) ) );
	EXPECT_FALSE( callback->IsValidBuildData( settings, &error, nullptr ) );
	EXPECT_NE( error.find( "Skeleton" ), std::string::npos ) << error;
}

TEST_F( VisObjBuilder, ValidationRejectsCachedSkeletonWithMissingFile )
{
	CPtr<IManipulator> settings =
			Singleton<IResourceManager>()->CreateObjectManipulator( "VisObjBuilder", CDBID( "builder.xdb" ) );
	ASSERT_TRUE( Singleton<IFolderCallback>()->InsertObject( "Skeleton", "shared.xdb" ) );
	ASSERT_TRUE( settings->SetValue( "Skeleton", "shared.xdb" ) );
	NDb::SaveChanges();
	ASSERT_TRUE( fs::remove( root / "shared.xdb" ) );
	std::string error;
	EXPECT_FALSE( dynamic_cast<IBuildDataCallback *>( builder.GetPtr() )->IsValidBuildData( settings, &error, nullptr ) );
	EXPECT_NE( error.find( "Skeleton" ), std::string::npos ) << error;
}

TEST_F( VisObjBuilder, DeletedResourcesStayDeletedUntilDiscardOrReplacement )
{
	ASSERT_TRUE( Singleton<IFolderCallback>()->InsertObject( "VisObj", "original.xdb" ) );
	NDb::SaveChanges();
	ASSERT_TRUE( NDb::RemoveObject( CDBID( "original.xdb" ) ) );
	EXPECT_FALSE( NDb::DoesObjectExist( CDBID( "original.xdb" ) ) );
	EXPECT_FALSE( NDb::RegisterResourceFile( "original.xdb" ) );
	EXPECT_EQ( NDb::GetManipulator( CDBID( "original.xdb" ) ), nullptr );
	NDb::DropCachedResources();
	EXPECT_TRUE( NDb::DoesObjectExist( CDBID( "original.xdb" ) ) );
	ASSERT_TRUE( NDb::RemoveObject( CDBID( "original.xdb" ) ) );
	ASSERT_TRUE( Singleton<IFolderCallback>()->InsertObject( "VisObj", "original.xdb" ) );
	EXPECT_NE( NDb::GetManipulator( CDBID( "original.xdb" ) ), nullptr );
	NDb::DropCachedResources();
	EXPECT_TRUE( NDb::IsFileRegistered( "original.xdb" ) );
	EXPECT_NE( NDb::GetManipulator( CDBID( "original.xdb" ) ), nullptr );
}

TEST_F( VisObjBuilder, SavingDeletionRemovesFileAndKeepsReplacement )
{
	NDb::AddDbObserver( NDb::CreateVFSDbObserver() );
	for ( const char *name : { "deleted.xdb", "replaced.xdb" } )
	{
		ASSERT_TRUE( Singleton<IFolderCallback>()->InsertObject( "VisObj", name ) );
	}
	NDb::SaveChanges();
	ASSERT_TRUE( NDb::RemoveObject( CDBID( "deleted.xdb" ) ) );
	ASSERT_TRUE( NDb::RemoveObject( CDBID( "deleted.xdb" ) ) );
	ASSERT_TRUE( NDb::RemoveObject( CDBID( "replaced.xdb" ) ) );
	ASSERT_TRUE( NDb::RemoveObject( CDBID( "replaced.xdb" ) ) );
	ASSERT_TRUE( Singleton<IFolderCallback>()->InsertObject( "VisObj", "replaced.xdb" ) );
	NDb::SaveChanges();
	EXPECT_FALSE( fs::exists( root / "deleted.xdb" ) );
	EXPECT_FALSE( NDb::DoesObjectExist( CDBID( "deleted.xdb" ) ) );
	EXPECT_TRUE( fs::is_regular_file( root / "replaced.xdb" ) );
	EXPECT_NE( NDb::GetManipulator( CDBID( "replaced.xdb" ) ), nullptr );
}

class EditorViews : public VisObjBuilder
{
protected:
	bool wxStarted = false;
	std::unique_ptr<wxFrame> frame;

	void SetUp() override
	{
		VisObjBuilder::SetUp();
		wxApp::SetInstance( new wxApp );
		int argc = 1;
		char name[] = "EditorViews_test";
		char *argv[] = { name, nullptr };
		wxStarted = wxEntryStart( argc, argv );
		if ( !wxStarted )
		{
			GTEST_SKIP() << "A display is required for wx editor tests (use xvfb-run on Linux).";
		}
		frame = std::make_unique<wxFrame>( nullptr, wxID_ANY, "Editor regression" );
	}

	void TearDown() override
	{
		frame.reset();
		if ( wxStarted )
		{
			wxTheApp->ProcessPendingEvents();
			wxEntryCleanup();
		}
		VisObjBuilder::TearDown();
	}
};

TEST_F( EditorViews, RepeatedTechLevelDeletionAndUndoRebuildsSelectedRows )
{
	// Use the exact MPConsts type, including text references and nested side tech levels.
	// Keep GameX loaded so its generated MPConsts metadata is registered on Windows too.
	CObj<IAICmdsAutoMagic> registration = CreateAICmdsAutoMagic();
	ASSERT_TRUE( Singleton<IFolderCallback>()->InsertObject( "MultiplayerConsts", "mp.xdb" ) );
	CPtr<IManipulator> object =
			Singleton<IResourceManager>()->CreateObjectManipulator( "MultiplayerConsts", CDBID( "mp.xdb" ) );
	ASSERT_NE( object, nullptr );
	ASSERT_TRUE( object->InsertNode( "Sides" ) );
	const char *arrays[] = { "TechLevels", "Sides.[0].TechLevels", "Sides.[0].TechLevels.[0].Reinforcements" };
	for ( const char *array : arrays )
	{
		for ( int i = 0; i < 5; ++i )
		{
			ASSERT_TRUE( object->InsertNode( array ) );
		}
	}
	ASSERT_TRUE( Singleton<IFolderCallback>()->InsertObject( "Reinforcement", "reinf.xdb" ) );
	const std::string reinforcements = "Sides.[0].TechLevels.[0].Reinforcements";
	EXPECT_EQ( object->GetDesc( reinforcements + ".[0]." ), nullptr );
	for ( int i = 0; i < 5; ++i )
	{
		ASSERT_TRUE( object->SetValue( reinforcements + ".[" + std::to_string( i ) + "]", CDBID( "reinf.xdb" ) ) );
	}
	std::unique_ptr<NPropertyPane::IGrid> grid( NPropertyPane::CreateGridWx( frame.get(), nullptr, nullptr, "test" ) );
	SObjectSet objects;
	objects.szObjectTypeName = "MultiplayerConsts";
	InsertHashSetElement( &objects.objectNameSet, CDBID( "mp.xdb" ) );
	grid->GetView()->SetViewManipulator( object, objects, "" );
	grid->BuildTree();
	auto *manager = static_cast<wxPropertyGridManager *>( grid->GetWindow() );
	for ( const char *array : arrays )
	{
		for ( int i = 0; i < 15; ++i )
		{
			const std::string selected = std::string( array ) + ".[1]";
			ASSERT_NE( manager->GetPropertyByName( selected ), nullptr );
			manager->SelectProperty( manager->GetPropertyByName( selected ) );
			CPtr<CObjectController> controller = grid->GetView()->CreateController<CObjectController>( nullptr );
			ASSERT_TRUE( controller->AddRemoveOperation( array, 1, object ) );
			ASSERT_TRUE( controller->Redo( false, true, nullptr ) );
			wxTheApp->ProcessPendingEvents();
			ASSERT_TRUE( controller->Undo( true, true, nullptr ) );
			wxTheApp->ProcessPendingEvents();
			EXPECT_NE( manager->GetPropertyByName( selected ), nullptr );
		}
	}
	// Clearing and restoring a link array must restore its count and every DBID.
	CPtr<CObjectController> clear = grid->GetView()->CreateController<CObjectController>( nullptr );
	ASSERT_TRUE( clear->AddRemoveOperation( reinforcements, NODE_REMOVEALL_INDEX, object ) );
	ASSERT_TRUE( clear->Redo( false, true, nullptr ) );
	ASSERT_TRUE( clear->Undo( true, true, nullptr ) );
	int count = 0;
	ASSERT_TRUE( CManipulatorManager::GetValue( &count, object, reinforcements ) );
	ASSERT_EQ( count, 5 );
	for ( int i = 0; i < count; ++i )
	{
		CVariant reference;
		ASSERT_TRUE( object->GetValue( reinforcements + ".[" + std::to_string( i ) + "]", &reference ) );
		EXPECT_EQ( reference.GetDBID(), CDBID( "reinf.xdb" ) );
	}
}

TEST_F( EditorViews, BuilderWarningsRefreshAfterEditsAndUndo )
{
	CPtr<IManipulator> settings =
			Singleton<IResourceManager>()->CreateObjectManipulator( "VisObjBuilder", CDBID( "builder.xdb" ) );
	std::unique_ptr<NPropertyPane::IGrid> grid( NPropertyPane::CreateGridWx( frame.get(), nullptr, nullptr, "test" ) );
	SObjectSet objects;
	objects.szObjectTypeName = "VisObjBuilder";
	InsertHashSetElement( &objects.objectNameSet, CDBID( "builder.xdb" ) );
	grid->GetView()->SetViewManipulator( settings, objects, "" );
	grid->BuildTree();
	SBuildDataParams params;
	params.nFlags = BDF_CHECK_PROPERTIES;
	std::string error;
	bool canAccept = true;
	int changes = 0;
	grid->SetChangeCallback(
			[&]()
			{
				++changes;
				canAccept = NBuildData::CanAccept(
						&params, dynamic_cast<IBuildDataCallback *>( builder.GetPtr() ), grid->GetView(), &error );
			} );
	CPtr<CObjectController> change = grid->GetView()->CreateController<CObjectController>( nullptr );
	ASSERT_TRUE( change->AddChangeOperation( "ModelFileName", "missing.glb", settings ) );
	ASSERT_TRUE( change->Redo( false, true, nullptr ) );
	EXPECT_EQ( changes, 1 );
	EXPECT_FALSE( canAccept );
	EXPECT_NE( error.find( "ModelFileName" ), std::string::npos );
	ASSERT_TRUE( change->Undo( true, true, nullptr ) );
	EXPECT_EQ( changes, 2 );
	EXPECT_TRUE( canAccept ) << error;
	EXPECT_TRUE( error.empty() );
}

TEST_F( EditorViews, BrowserDeletionClearsSelectedExistingAndStaleRows )
{
	ASSERT_TRUE( Singleton<IFolderCallback>()->InsertObject( "VisObj", "one.xdb" ) );
	ASSERT_TRUE( Singleton<IFolderCallback>()->InsertObject( "VisObj", "two.xdb" ) );
	wxWindow *window = nullptr;
	std::unique_ptr<IObjectBrowser> browser(
			NObjectBrowser::CreateWxIn( frame.get(), nullptr, nullptr, IObjectBrowser::KIND_LINK, &window ) );
	IObjectTree *tree = browser->AddTable( "VisObj" );
	ASSERT_NE( tree, nullptr );
	SObjectSet objects;
	objects.szObjectTypeName = "VisObj";
	InsertHashSetElement( &objects.objectNameSet, CDBID( VIEW_COLLECTION_ID ) );
	CPtr<IManipulator> folder = Singleton<IResourceManager>()->CreateFolderManipulator( "VisObj" );
	tree->GetView()->SetViewManipulator( folder, objects, "" );
	tree->CreateTree();
	wxTheApp->ProcessPendingEvents();
	for ( const char *name : { "one.xdb", "two.xdb" } )
	{
		ASSERT_TRUE( tree->SetCurrentTreeItemName( name, true ) );
		// The second row still exists in the tree after the database entry is gone.
		if ( std::string( name ) == "two.xdb" )
		{
			ASSERT_TRUE( NDb::RemoveObject( CDBID( name ) ) );
		}
		ASSERT_TRUE( Singleton<IFolderCallback>()->RemoveObject( "VisObj", name, false ) );
		wxTheApp->ProcessPendingEvents();
		std::string selected;
		tree->GetCurrentTreeItemName( &selected );
		EXPECT_NE( selected, name );
	}
}

TEST_F( VisObjBuilder, DeletesCreatedResourceGraphAndMissingDependencies )
{
	CreateAndCheck();
	ASSERT_TRUE( builder->RemoveObject( "VisObj", "Unit/visualObject.xdb" ) );
	EXPECT_FALSE( NDb::DoesObjectExist( CDBID( "Unit/visualObject.xdb" ) ) );
	EXPECT_FALSE( builder->RemoveObject( "VisObj", "missing.xdb" ) );
}

TEST_F( VisObjBuilder, RecreatesDependenciesWhoseFilesWereDeleted )
{
	fs::create_directories( root / "Unit" );
	for ( const auto &resource : { std::pair<const char *, const char *>{ "Model", "model_texture_Model.xdb" },
	                               { "Material", "texture_Material.xdb" },
	                               { "Texture", "texture_Texture.xdb" },
	                               { "Geometry", "model_Geometry.xdb" },
	                               { "AIGeometry", "model_AIGeometry.xdb" },
	                               { "Skeleton", "model_Skeleton.xdb" } } )
	{
		const std::string name = std::string( "Unit/" ) + resource.second;
		std::ofstream( root / name ) << "<" << resource.first << "/>";
		ASSERT_TRUE( NDb::RegisterResourceFile( name ) );
		fs::remove( root / name );
		ASSERT_TRUE( NDb::DoesObjectExist( CDBID( name ) ) );
	}
	CreateAndCheck();
}

TEST_F( VisObjBuilder, RestoresDeletedFilesForCachedDependencies )
{
	ASSERT_NO_FATAL_FAILURE( CreateAndCheck() );
	ASSERT_TRUE( fs::remove( root / "Unit/texture_Texture.xdb" ) );
	ASSERT_TRUE( fs::remove( root / "Unit/model_AIGeometry.xdb" ) );
	CreateAndCheck( "second.xdb" );
}

TEST_F( VisObjBuilder, RecreatesMissingChildrenOfExistingResourcesAfterReload )
{
	ASSERT_NO_FATAL_FAILURE( CreateAndCheck() );
	NDb::CloseDatabase();
	ASSERT_TRUE( fs::remove( root / "Unit/texture_Texture.xdb" ) );
	ASSERT_TRUE( fs::remove( root / "Unit/model_AIGeometry.xdb" ) );
	ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
	CreateAndCheck( "second.xdb" );
}
} // namespace
