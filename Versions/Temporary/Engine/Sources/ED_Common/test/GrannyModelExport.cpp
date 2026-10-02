#include "3Dmotor/stdafx.h"
#include "../GrannyModelExport.h"
#include "3Dmotor/DBScene.h"
#include "Stats_B2_M1/ActionCommand.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "libdb/Db.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include <fastgltf/core.hpp>
#include <simdjson.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <set>
#include <gtest/gtest.h>

TEST( GrannyModelExport, ExportsRepositoryModelsInAllTextureModesWithoutChangingResources )
{
	// Allow the sparse CI checkout to be reproduced without moving game data.
	const char *overrideRoot = std::getenv("OBK2_GRANNY_TEST_DATA_DIR");
	const auto dataRoot = std::filesystem::u8path(overrideRoot ? overrideRoot : OBK2_DATA_DIR);
	const char *modelPaths[] = {"Mines/All/MineUniversal/Model.xdb",
		"Units/Technics/USSR/Tanks/T_60/1_1_Model.xdb",
		"Units/Infantry/Japan/Japan_soldier/1_1_Model.xdb"};
	// Cone-mode sparse checkouts keep Data/index.bin and types.xml even when
	// Mines and Units are omitted. Check the actual fixtures before opening
	// the database; a missing model is otherwise a failure (or a Debug assert).
	for ( const char *path : {"index.bin", "types.xml", modelPaths[0], modelPaths[1], modelPaths[2]} )
		if ( !std::filesystem::is_regular_file(dataRoot / path) )
			GTEST_SKIP() << "Optional Granny model fixture is unavailable: " << path;
	// Force the stats module to load, so AnimB2 is registered just as in the editor.
	EXPECT_EQ(GetCommandByAction(NDb::USER_ACTION_MOVE), ACTION_COMMAND_MOVE_TO);
	CObj<NVFS::IVFS> saved = NVFS::GetMainVFS();
	NVFS::SetMainVFS(NVFS::CreateWinVFS(dataRoot.generic_u8string() + "/"));
	struct SCleanup
	{
		NVFS::IVFS *saved;
		~SCleanup() { NDb::CloseDatabase(); NVFS::SetMainVFS(saved); }
	} cleanup{saved};
	ASSERT_TRUE(NDb::OpenDatabase(NVFS::GetMainVFS(), nullptr, NDb::DATABASE_MODE_EDITOR));
	std::filesystem::create_directories(GRANNY_EXPORT_OUTPUT_DIR);
	int number = 0;
	for ( const char *path : modelPaths )
	{
		SCOPED_TRACE(path);
		CDBPtr<NDb::SModel> model = NDb::Get<NDb::SModel>(CDBID(path));
		ASSERT_TRUE(model);
		const auto geometryUID = model->pGeometry->uid;
		const auto animationCount = model->animations.size();
		for ( const auto &options : {NEditorGltf::SGrannyExportOptions{},
			NEditorGltf::SGrannyExportOptions{true, false, false},
			NEditorGltf::SGrannyExportOptions{false, true, true},
			NEditorGltf::SGrannyExportOptions{false, false, false}} )
		{
			const auto output = std::filesystem::path(GRANNY_EXPORT_OUTPUT_DIR) /
				std::filesystem::u8path("sample # \xc4\x8d-" + std::to_string(number++) + ".glb");
			SCOPED_TRACE(output.u8string());
			std::string error;
			ASSERT_TRUE(NEditorGltf::ExportGrannyModel(model.GetPtr(), output.u8string(), options, &error)) << error;
			EXPECT_EQ(model->pGeometry->uid, geometryUID);
			EXPECT_TRUE(model->pGeometry->szModelFileRef.empty());
			EXPECT_EQ(model->animations.size(), animationCount);
			auto data = fastgltf::GltfDataBuffer::FromPath(output);
			ASSERT_TRUE(data);
			fastgltf::Parser parser(fastgltf::Extensions::MSFT_texture_dds);
			std::vector<std::string> clipNames;
			parser.setUserPointer(&clipNames);
			parser.setExtrasParseCallback([](simdjson::dom::object *extras, size_t, fastgltf::Category category, void *user)
			{
				if ( category != fastgltf::Category::Animations ) return;
				uint64_t fps = 0;
				ASSERT_FALSE((*extras)["framesPerSecond"].get(fps));
				EXPECT_EQ(fps, 30u);
				simdjson::dom::array clips;
				ASSERT_FALSE((*extras)["clips"].get(clips));
				for ( auto clip : clips )
				{
					std::string_view name;
					ASSERT_FALSE(clip["name"].get(name));
					static_cast<std::vector<std::string> *>(user)->emplace_back(name);
				}
			});
			auto asset = parser.loadGltfBinary(data.get(), output.parent_path(), fastgltf::Options::LoadGLBBuffers);
			ASSERT_TRUE(asset);
			EXPECT_EQ(fastgltf::validate(asset.get()), fastgltf::Error::None);
			ASSERT_FALSE(asset->textures.empty());
			for ( const auto &texture : asset->textures )
			{
				ASSERT_TRUE(texture.imageIndex.has_value());
				const bool tga = options.separateTextures && options.convertTexturesToTga;
				EXPECT_EQ(texture.ddsImageIndex.has_value(), !tga);
				if ( options.separateTextures )
				{
					const auto &uri = std::get<fastgltf::sources::URI>(asset->images[*texture.imageIndex].data).uri;
					EXPECT_TRUE(uri.fspath().is_relative());
					EXPECT_EQ(uri.fspath().extension(), tga ? ".tga" : ".png");
					std::ifstream image(output.parent_path() / uri.fspath(), std::ios::binary);
					ASSERT_TRUE(image);
					if ( tga )
					{
						image.seekg(2); EXPECT_EQ(image.get(), 2); // Uncompressed truecolor TGA.
						image.seekg(16); EXPECT_EQ(image.get(), 32); // Includes alpha.
					}
					else
					{
						EXPECT_EQ(image.get(), 0x89); EXPECT_EQ(image.get(), 'P');
						const auto &ddsImage = asset->images[*texture.ddsImageIndex];
						const auto ddsPath = std::get<fastgltf::sources::URI>(ddsImage.data).uri.fspath();
						EXPECT_EQ(ddsPath.extension(), ".dds");
						std::ifstream dds(output.parent_path() / ddsPath, std::ios::binary);
						ASSERT_TRUE(dds);
						const std::vector<char> copied((std::istreambuf_iterator<char>(dds)), std::istreambuf_iterator<char>());
						CFileStream original(NVFS::GetMainVFS(), std::string(ddsImage.name));
						ASSERT_TRUE(original.IsOk());
						std::vector<char> expected(original.GetSize());
						original.Read(expected.data(), expected.size());
						EXPECT_EQ(copied, expected);
					}
				}
				else
				{
					EXPECT_EQ(std::get<fastgltf::sources::BufferView>(asset->images[*texture.imageIndex].data).mimeType, fastgltf::MimeType::PNG);
					EXPECT_EQ(std::get<fastgltf::sources::BufferView>(asset->images[*texture.ddsImageIndex].data).mimeType, fastgltf::MimeType::DDS);
				}
			}
			EXPECT_EQ(asset->scenes.size(), 2u);
			std::vector<std::string> expectedClips;
			std::set<std::string> seen;
			for ( const auto &clip : model->animations )
				if ( clip ) { expectedClips.push_back(NDb::GetFileName(clip->GetDBID())); seen.insert(expectedClips.back()); }
			if ( model->pSkeleton )
				for ( const auto &clip : model->pSkeleton->animations )
					if ( clip && seen.insert(NDb::GetFileName(clip->GetDBID())).second ) expectedClips.push_back(NDb::GetFileName(clip->GetDBID()));
			EXPECT_EQ(clipNames, expectedClips);
			EXPECT_EQ(asset->animations.size(), expectedClips.empty() ? 0u : 1u);
		}
	}
}
