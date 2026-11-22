#include "ED_Common/stdafx.h"

#include "3Dmotor/GParticleFormat.h"
#include "ED_Common/ParticleExporter.h"
#include "libdb/EditorDb.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/Interface_MOD.h"
#include "Stats_B2_M1/ActionCommand.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

namespace
{
namespace fs = std::filesystem;

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

class ParticleExporter : public testing::Test
{
protected:
	fs::path root;
	CObj<NVFS::IVFS> savedVFS;
	CObj<NVFS::IFileCreator> savedCreator;
	CPtr<IManipulator> particle;

	void SetUp() override
	{
		// Load the resource factories before opening the editor database.
		ASSERT_EQ( GetCommandByAction( NDb::USER_ACTION_MOVE ), ACTION_COMMAND_MOVE_TO );
		root = fs::temp_directory_path() /
		       ( "obk2-particle-" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
		fs::create_directories( root );
		fs::copy_file( PARTICLE_SCHEMA, root / "types.xml" );
		std::ofstream( root / "particle.xdb" ) << "<Particle><uid>B70C16DF-F18D-4152-AE05-5B04D9648C33</uid></Particle>";
		const auto folder = root.generic_u8string() + "/";
		savedVFS = NVFS::GetMainVFS();
		savedCreator = NVFS::GetMainFileCreator();
		NVFS::SetMainVFS( NVFS::CreateWinVFS( folder ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( folder ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "particle.xdb" ) );
		IResourceManager::InitSingleton();
		auto *user = new CTestUserData;
		user->data.constUserData.szExportSourceFolder = ( root / "Source" ).generic_u8string() + "/";
		NSingleton::RegisterSingleton( user, IUserDataContainer::tidTypeID );
		auto *mod = new CTestMod;
		mod->folder = folder;
		NSingleton::RegisterSingleton( mod, IMODContainer::tidTypeID );
		particle = Singleton<IResourceManager>()->CreateObjectManipulator( "Particle", std::string( "particle.xdb" ) );
		ASSERT_NE( particle, nullptr );
	}

	void TearDown() override
	{
		particle = nullptr;
		NSingleton::UnRegisterSingleton( IMODContainer::tidTypeID );
		NSingleton::UnRegisterSingleton( IUserDataContainer::tidTypeID );
		IResourceManager::UninitSingleton();
		NDb::CloseDatabase();
		NVFS::SetMainVFS( savedVFS );
		NVFS::SetMainFileCreator( savedCreator );
		std::error_code error;
		fs::remove_all( root, error );
	}

	std::string Import( const char *file )
	{
		return CParticleExporter::ImportClip( particle, ( root / file ).u8string(), NVFS::GetMainFileCreator(), false );
	}
};

TEST_F( ParticleExporter, ImportsPackedClipAndKeepsLastGoodExport )
{
	// One billboard with five constant tracks. Write individual fields since
	// the packed directory contains int32 offsets aligned to just two bytes.
	CMemoryStream data;
	data << int32_t( 86 ) << 1.0f << 30.0f << int32_t( 1 ) << int16_t( 0 ) << int16_t( 30 );
	for ( int32_t offset : { 46, 60, 66, 76, 82 } )
	{
		data << int16_t( 1 ) << offset;
	}
	data << int16_t( 0 ) << 5.0f << 6.0f << 7.0f;
	data << int16_t( 0 ) << 0.0f;
	data << int16_t( 0 ) << 2.0f << 2.0f;
	data << int16_t( 0 ) << uint32_t( 0xffffffff );
	data << int16_t( 0 ) << int16_t( 0 );
	std::ofstream( root / "clip.PTC", std::ios::binary )
			.write( reinterpret_cast<const char *>( data.GetBuffer() ), data.GetSize() );

	// Exercise the editor's registered action with a source under the configured
	// art root and a different active-mod destination.
	fs::create_directories( root / "Source/FX" );
	fs::copy_file( root / "clip.PTC", root / "Source/FX/clip.PTC" );
	ASSERT_TRUE( particle->SetValue( "SrcName", "FX/clip.PTC" ) );
	CPtr<IExporter> exporter = NExporterFactory::CreateExporter( "Particle" );
	ASSERT_NE( exporter, nullptr );
	ASSERT_TRUE( exporter->StartExport( "Particle", false ) );
	ASSERT_EQ( exporter->ExportObject( particle, "Particle", "particle.xdb", false, ET_BEFORE_REF ), ER_SUCCESS );
	ASSERT_EQ( exporter->ExportObject( particle, "Particle", "particle.xdb", false, ET_NO_REF ), ER_SUCCESS );
	exporter->FinishExport( "Particle", false );
	CVariant before;
	ASSERT_TRUE( particle->GetValue( "Bound", &before ) );
	SBound bound;
	ASSERT_NE( before.GetPtr(), nullptr );
	memcpy( &bound, before.GetPtr(), sizeof( bound ) );
	EXPECT_FLOAT_EQ( bound.s.ptCenter.x, 5.0f );
	EXPECT_FLOAT_EQ( bound.s.ptCenter.z, 7.0f );
	EXPECT_FLOAT_EQ( bound.ptHalfBox.y, std::sqrt( 2.0f ) );

	std::ofstream( root / "broken.ptc", std::ios::binary ) << "bad";
	EXPECT_FALSE( Import( "broken.ptc" ).empty() );
	EXPECT_NE( Import( "missing.MB" ).find( "Maya particle export is no longer supported" ), std::string::npos );
	// A valid header with a track that crosses the end of the file must also fail.
	data.Seek( 22 );
	data << int32_t( 84 );
	std::ofstream( root / "offset.ptc", std::ios::binary )
			.write( reinterpret_cast<const char *>( data.GetBuffer() ), data.GetSize() );
	EXPECT_FALSE( Import( "offset.ptc" ).empty() );
	CVariant after;
	ASSERT_TRUE( particle->GetValue( "Bound", &after ) );
	EXPECT_TRUE( before == after );
	const auto path = root / "bin/Effects/b70c16df-f18d-4152-ae05-5b04d9648c33";
	CFileStream installed( path.u8string(), CFileStream::WIN_READ_ONLY );
	CFileStream source( ( root / "clip.PTC" ).u8string(), CFileStream::WIN_READ_ONLY );
	ASSERT_TRUE( installed.IsOk() && source.IsOk() );
	ASSERT_EQ( installed.GetSize(), source.GetSize() );
	EXPECT_EQ( memcmp( installed.GetBuffer(), source.GetBuffer(), source.GetSize() ), 0 );

	// Legacy records with an explicit nil UID use the same numeric fallback as the game.
	std::ofstream( root / "legacy.xdb" )
			<< R"(<Particle ObjectRecordID="8712"><uid>00000000-0000-0000-0000-000000000000</uid></Particle>)";
	ASSERT_TRUE( NDb::RegisterResourceFile( "legacy.xdb" ) );
	particle = Singleton<IResourceManager>()->CreateObjectManipulator( "Particle", std::string( "legacy.xdb" ) );
	ASSERT_NE( particle, nullptr );
	ASSERT_EQ( Import( "clip.PTC" ), "" );
	EXPECT_TRUE( fs::is_regular_file( root / "bin/Effects" / std::to_string( particle->GetID( "" ) ) ) );
}

TEST_F( ParticleExporter, ReadsBlenderExampleWithBothSources )
{
	if ( !fs::is_regular_file( PARTICLE_EXAMPLE ) )
	{
		GTEST_SKIP() << "Blender particle example is unavailable in this checkout";
	}
	const auto assets = fs::path( PARTICLE_EXAMPLE ).parent_path();
	fs::copy_file( assets / "Smoke_Particle.xdb", root / "Smoke_Particle.xdb" );
	ASSERT_TRUE( NDb::RegisterResourceFile( "Smoke_Particle.xdb" ) );
	particle = Singleton<IResourceManager>()->CreateObjectManipulator( "Particle", std::string( "Smoke_Particle.xdb" ) );
	ASSERT_NE( particle, nullptr );
	CVariant packagedBound;
	ASSERT_TRUE( particle->GetValue( "Bound", &packagedBound ) );
	ASSERT_EQ( CParticleExporter::ImportClip( particle, PARTICLE_EXAMPLE, NVFS::GetMainFileCreator(), true ), "" );
	CVariant importedBound;
	ASSERT_TRUE( particle->GetValue( "Bound", &importedBound ) );
	EXPECT_TRUE( importedBound == packagedBound );
	CFileStream source( PARTICLE_EXAMPLE, CFileStream::WIN_READ_ONLY );
	CObj<NGScene::CParticlesInfo> clip = new NGScene::CParticlesInfo;
	ASSERT_TRUE( clip->ReadClip( source ) );
	ASSERT_EQ( clip->nParticles, 19 );
	EXPECT_FLOAT_EQ( clip->fTEnd, 2 );
	EXPECT_FLOAT_EQ( clip->fFrameRate, 30 );
	const auto &wisp = clip->particles[0];
	CVec3 pos;
	wisp.pos.GetValue( 59, &pos );
	EXPECT_FLOAT_EQ( pos.x, -0.4f );
	EXPECT_FLOAT_EQ( pos.z, 2.0f );
	const auto &puff = clip->particles[1];
	short sprite;
	puff.sprite.GetValue( 40, &sprite );
	EXPECT_EQ( sprite, 1 );
	EXPECT_LT( puff.color.keys[puff.color.nKeys - 1].GetValue() >> 24, 12u );
}

TEST( ParticleClips, ReadsInstalledLegacyClips )
{
	const auto folder = fs::path( PARTICLE_SCHEMA ).parent_path() / "bin/Effects";
	if ( !fs::is_directory( folder ) )
	{
		GTEST_SKIP() << "Stock particle clips are unavailable in this checkout";
	}
	for ( const auto &entry : fs::directory_iterator( folder ) )
	{
		if ( entry.is_regular_file() )
		{
			CFileStream stream( entry.path().u8string(), CFileStream::WIN_READ_ONLY );
			CObj<NGScene::CParticlesInfo> clip = new NGScene::CParticlesInfo;
			EXPECT_TRUE( clip->ReadClip( stream ) ) << entry.path();
		}
	}
}
} // namespace
