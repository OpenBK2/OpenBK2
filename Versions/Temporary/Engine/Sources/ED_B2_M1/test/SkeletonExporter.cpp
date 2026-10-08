#include "ED_B2_M1/stdafx.h"
#include "ED_B2_M1/SkeletonExporter.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/Interface_MOD.h"
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
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace
{
namespace fs = std::filesystem;

class CTestLog : public NLog::ILoggerSink, public ILogger
{
	OBJECT_NOCOPY_METHODS(CTestLog);
public:
	std::string messages;
	ILogger *GetLogger() override { return this; }
	void Log( ELogOutputType, const std::string &text ) override { messages += text; }
	void ClearLog() override { messages.clear(); }
};
class CTestUserData : public IUserDataContainer
{
	OBJECT_NOCOPY_METHODS(CTestUserData);
public:
	SUserData data;
	SUserData *Get() override { return &data; }
	void Load() override {}
	void Save() override {}
};
class CTestMod : public IMODContainer
{
	OBJECT_NOCOPY_METHODS(CTestMod);
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

class SkeletonExporter : public testing::Test
{
protected:
	fs::path root;
	CObj<NVFS::IVFS> savedVFS;
	CObj<NVFS::IFileCreator> savedCreator;
	CPtr<CTestLog> log;
	CPtr<IExporter> exporter;
	CPtr<IManipulator> skeleton;

	void WriteScene( const std::string &properties = R"("StartTime":100,"EndTime":119,"ActionTime":110,"Looped":true,"Speed":2,"AABBIndex":2)", bool markers = true )
	{
		// Like Blender's object animation export: the first track starts at frame
		// 50, while the second supplies the full absolute 1..240 frame clock.
		std::ofstream(root / "model.gltf") << R"({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0,2,5]}],
"nodes":[{"name":"Basis","children":[1]},{"name":"Handle"},{"name":")" << (markers ? "Animations" : "Unused") << R"(","children":[3,4]},
{"name":"Death01","extras":{)" << properties << R"(}},
{"name":"install_attack","extras":{"starttime":1,"endtime":50}},
{"name":"Death99","extras":{"StartTime":10000,"EndTime":20000}}],
"buffers":[{"uri":"motion.bin","byteLength":3840}],
"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":960},{"buffer":0,"byteOffset":960,"byteLength":2880}],
"accessors":[{"bufferView":0,"componentType":5126,"count":240,"type":"SCALAR"},
{"bufferView":1,"componentType":5126,"count":240,"type":"VEC3"},
{"bufferView":0,"byteOffset":196,"componentType":5126,"count":191,"type":"SCALAR"},
{"bufferView":1,"byteOffset":588,"componentType":5126,"count":191,"type":"VEC3"}],
"animations":[{"name":")" << (markers ? "BasisAction" : "idle") << R"(","samplers":[{"input":2,"output":3}],"channels":[{"sampler":0,"target":{"node":0,"path":"translation"}}]},
{"name":"HandleAction","samplers":[{"input":0,"output":1}],"channels":[{"sampler":0,"target":{"node":1,"path":"translation"}}]}]})";
	}

	void SetUp() override
	{
		log = new CTestLog;
		NLog::SetLogger(log);
		ASSERT_EQ(GetCommandByAction(NDb::USER_ACTION_MOVE), ACTION_COMMAND_MOVE_TO);
		root = fs::temp_directory_path() / ("obk2-skeleton-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		fs::create_directories(root);
		fs::copy_file(SKELETON_SCHEMA, root / "types.xml");
		{
			std::ofstream binary(root / "motion.bin", std::ios::binary);
			for ( int i = 1; i <= 240; ++i ) { const float time = i / 30.0f; binary.write(reinterpret_cast<const char *>(&time), sizeof(time)); }
			for ( int i = 1; i <= 240; ++i ) { const float position[3] = {float(i), 0, 0}; binary.write(reinterpret_cast<const char *>(position), sizeof(position)); }
		}
		WriteScene();
		const std::string folder = root.generic_u8string() + "/";
		savedVFS = NVFS::GetMainVFS();
		savedCreator = NVFS::GetMainFileCreator();
		NVFS::SetMainVFS(NVFS::CreateWinVFS(folder));
		NVFS::SetMainFileCreator(NVFS::CreateWinFileCreator(folder));
		ASSERT_TRUE(NDb::OpenDatabase(NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR));
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
		exporter = NExporterFactory::CreateExporter("Skeleton");
		ASSERT_NE(exporter, nullptr);
		ASSERT_TRUE(Singleton<IFolderCallback>()->InsertObject("Skeleton", "Unit/Anim_Skeleton.xdb"));
		skeleton = Singleton<IResourceManager>()->CreateObjectManipulator("Skeleton", std::string("Unit/Anim_Skeleton.xdb"));
		ASSERT_NE(skeleton, nullptr);
		ASSERT_TRUE(skeleton->SetValue("ModelFileRef", "model.gltf"));
		ASSERT_TRUE(skeleton->SetValue("RootJoint", "Basis"));
	}
	void TearDown() override
	{
		skeleton = nullptr;
		exporter = nullptr;
		const unsigned ids[] = {IFolderCallback::tidTypeID, IViewContainer::tidTypeID, ICommandHandlerContainer::tidTypeID,
			IMODContainer::tidTypeID, IUserDataContainer::tidTypeID};
		for ( unsigned id : ids ) NSingleton::UnRegisterSingleton(id);
		IResourceManager::UninitSingleton();
		NDb::CloseDatabase();
		NVFS::SetMainVFS(savedVFS);
		NVFS::SetMainFileCreator(savedCreator);
		NLog::SetLogger(nullptr);
		std::error_code error;
		fs::remove_all(root, error);
	}
	// Qualify the call so Windows links the DLL that registers the exporter.
	bool Import() { return static_cast<CSkeletonExporter *>(exporter.GetPtr())->CSkeletonExporter::ImportGltfInfo(skeleton); }
	template<class T> T Value( IManipulator *resource, const char *field )
	{
		T value{};
		EXPECT_TRUE(CManipulatorManager::GetValue(&value, resource, field));
		return value;
	}
	CPtr<IManipulator> Animation( const char *name )
	{
		return Singleton<IResourceManager>()->CreateObjectManipulator("AnimB2", std::string("Unit/") + name);
	}
};

TEST_F(SkeletonExporter, ImportsMarkerRangesAndLegacyProperties)
{
	ASSERT_TRUE(Import()) << log->messages;
	EXPECT_EQ(Value<int>(skeleton, "Animations"), 2);
	CPtr<IManipulator> death = Animation("Anim_death_01_animb2.xdb");
	CPtr<IManipulator> install = Animation("Anim_install_attack_00_animb2.xdb");
	ASSERT_NE(death, nullptr);
	ASSERT_NE(install, nullptr);
	EXPECT_EQ(Value<std::string>(death, "Type"), "ANIMATION_DEATH");
	EXPECT_EQ(Value<std::string>(death, "ClipName"), "");
	EXPECT_EQ(Value<int>(death, "FirstFrame"), 100);
	EXPECT_EQ(Value<int>(death, "LastFrame"), 119);
	EXPECT_EQ(Value<int>(death, "Length"), 633);
	EXPECT_EQ(Value<int>(death, "ActionFrame"), 10);
	EXPECT_EQ(Value<int>(death, "Action"), 333);
	EXPECT_TRUE(Value<bool>(death, "Looped"));
	EXPECT_FLOAT_EQ(Value<float>(death, "MoveSpeed"), 2.0f);
	EXPECT_EQ(Value<std::string>(death, "AABBAName"), "AABB_A02");
	EXPECT_EQ(Value<std::string>(death, "AABBDName"), "AABB_D02");
	EXPECT_EQ(Value<std::string>(install, "Type"), "ANIMATION_INSTALL");
	EXPECT_EQ(Value<int>(install, "FirstFrame"), 1);
	EXPECT_EQ(Value<int>(install, "LastFrame"), 50);
	EXPECT_EQ(Value<int>(install, "Length"), 1633);
	NDb::SaveChanges();
	EXPECT_TRUE(fs::is_regular_file(root / "Unit/Anim_death_01_animb2.xdb"));
	EXPECT_TRUE(fs::is_regular_file(root / "Unit/Anim_install_attack_00_animb2.xdb"));
	// Re-export refreshes generated settings without duplicating references.
	ASSERT_TRUE(death->SetValue("FirstFrame", 101));
	ASSERT_TRUE(Import()) << log->messages;
	EXPECT_EQ(Value<int>(skeleton, "Animations"), 2);
	EXPECT_EQ(Value<int>(death, "FirstFrame"), 100);
}

TEST_F(SkeletonExporter, RejectsMissingMetadataWithoutPartialCreation)
{
	WriteScene("");
	EXPECT_FALSE(Import());
	EXPECT_EQ(Value<int>(skeleton, "Animations"), 0);
	EXPECT_FALSE(NDb::DoesObjectExist(CDBID("Unit/Anim_death_01_animb2.xdb")));
	EXPECT_NE(log->messages.find("StartTime and EndTime"), std::string::npos);
}

TEST_F(SkeletonExporter, RejectsOutOfRangeMetadataWithoutPartialCreation)
{
	WriteScene(R"("StartTime":240,"EndTime":300)");
	EXPECT_FALSE(Import());
	EXPECT_EQ(Value<int>(skeleton, "Animations"), 0);
	EXPECT_NE(log->messages.find("invalid frame range"), std::string::npos);
}

TEST_F(SkeletonExporter, RejectsReversedRanges)
{
	WriteScene(R"("StartTime":119,"EndTime":100)");
	EXPECT_FALSE(Import());
	EXPECT_EQ(Value<int>(skeleton, "Animations"), 0);
	EXPECT_NE(log->messages.find("Death01' has an invalid frame range 119..100"), std::string::npos);
}

TEST_F(SkeletonExporter, RecreatesStaleAnimationRecords)
{
	fs::create_directories(root / "Unit");
	const std::string name = "Unit/Anim_death_01_animb2.xdb";
	std::ofstream(root / name) << "<AnimB2/>";
	ASSERT_TRUE(NDb::RegisterResourceFile(name));
	ASSERT_TRUE(fs::remove(root / name));
	ASSERT_TRUE(Import()) << log->messages;
	NDb::SaveChanges();
	EXPECT_TRUE(fs::is_regular_file(root / name));
	EXPECT_EQ(Value<int>(skeleton, "Animations"), 2);
}

TEST_F(SkeletonExporter, KeepsManuallyAssignedAnimations)
{
	const std::string name = "Unit/manual.xdb";
	ASSERT_TRUE(Singleton<IFolderCallback>()->InsertObject("AnimB2", name));
	ASSERT_TRUE(skeleton->InsertNode("Animations"));
	ASSERT_TRUE(skeleton->SetValue("Animations.[0]", name));
	ASSERT_TRUE(Import()) << log->messages;
	EXPECT_EQ(Value<int>(skeleton, "Animations"), 3);
	EXPECT_EQ(CDBID(Value<std::string>(skeleton, "Animations.[0]")), CDBID(name));
	ASSERT_TRUE(Import()) << log->messages;
	EXPECT_EQ(Value<int>(skeleton, "Animations"), 3);
}

TEST_F(SkeletonExporter, RetainsNamedClipFallback)
{
	WriteScene("", false);
	ASSERT_TRUE(Import()) << log->messages;
	EXPECT_EQ(Value<int>(skeleton, "Animations"), 1);
	CPtr<IManipulator> idle = Animation("Anim_Skeleton_idle_0_animb2.xdb");
	ASSERT_NE(idle, nullptr);
	EXPECT_EQ(Value<std::string>(idle, "ClipName"), "idle");
	EXPECT_EQ(Value<std::string>(idle, "Type"), "ANIMATION_IDLE");
}

TEST_F(SkeletonExporter, SuppliedArtilleryModel)
{
	// Optional local integration check; never write to the artist's game data.
	const char *source = std::getenv("OBK2_SKELETON_GLB");
	if ( !source ) GTEST_SKIP() << "Set OBK2_SKELETON_GLB to the artillery Anim.glb for the real-model check.";
	fs::copy_file(fs::u8path(source), root / "artillery.glb");
	ASSERT_TRUE(skeleton->SetValue("ModelFileRef", "artillery.glb"));
	ASSERT_TRUE(Import()) << log->messages;
	ASSERT_EQ(Value<int>(skeleton, "Animations"), 12);
	CPtr<IManipulator> death = Animation("Anim_death_01_animb2.xdb");
	CPtr<IManipulator> install = Animation("Anim_install_attack_00_animb2.xdb");
	ASSERT_NE(death, nullptr);
	ASSERT_NE(install, nullptr);
	EXPECT_EQ(Value<int>(death, "FirstFrame"), 100);
	EXPECT_EQ(Value<int>(death, "LastFrame"), 119);
	EXPECT_EQ(Value<int>(death, "Length"), 633);
	EXPECT_EQ(Value<int>(install, "Length"), 1633);
	NDb::SaveChanges();
	int files = 0;
	for ( const auto &entry : fs::directory_iterator(root / "Unit") )
		if ( entry.path().filename().string().find("_animb2.xdb") != std::string::npos ) ++files;
	EXPECT_EQ(files, 12);
}
}
