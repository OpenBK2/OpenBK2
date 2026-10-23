#include "../stdafx.h"

#include "../AIMap.h"
#include "../Artillery.h"
#include "../Bridge.h"
#include "../BridgeCreation.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/CommonPathFinder.h"
#include "Common_RTS_AI/StaticPath.h"
#include "Common_RTS_AI/Terrain.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/BinSaver.h"
#include "System/Streams.h"
#include "System/System.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <list>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
struct SFields
{
	std::function<void( IBinSaver & )> visit;

	int operator&( IBinSaver &saver )
	{
		visit( saver );
		return 0;
	}
};

class BridgePaths : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<SBinSaverExternalObject> external;
	CDBPtr<NDb::SMechUnitRPGStats> unitStats;
	CDBPtr<NDb::SBridgeRPGStats> bridgeStats;

protected:
	CObj<CAIMap> map;
	CObj<CArtillery> unit;
	std::vector<CObj<CBridgeSpan>> spans;
	const CVec2 firstEnd{ 764, 500 };
	const CVec2 secondEnd{ 436, 500 };

	static void SetUpTestSuite()
	{
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-bridge-paths-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( BRIDGE_PATHS_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
	}

	static void TearDownTestSuite()
	{
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		NVFS::SetMainFileCreator( nullptr );
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	NDb::IObjMan *MakeRecord( const char *type )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( type );
		EXPECT_NE( record, nullptr );
		static unsigned nextRecord = 0;
		const std::string name = "record-" + std::to_string( nextRecord++ ) + ".xdb";
		EXPECT_TRUE( NDb::AddNewObject( name, CDBID( name ), record ) );
		records.push_back( record );
		return record;
	}

	void LoadFields( CObjectBase *object, const std::function<void( IBinSaver & )> &visit )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			SFields fields{ visit };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, object );
	}

	void PlaceUnit( const CVec2 &point )
	{
		CVec3 center( point, 0 );
		SVector tile = map->GetTile( point );
		CPtr<CAIMap> mapPointer = map.GetPtr();
		SFields path{ [&]( IBinSaver &saver )
		              {
										saver.Add( 9, &center );
										saver.Add( 10, &tile );
										saver.Add( 11, &tile );
										saver.Add( 35, &mapPointer );
									} };
		SFields common{ [&]( IBinSaver &saver )
		                {
											int saveVersion = 666;
											saver.Add( 20, &path );
											// Match modern saves and skip legacy orientation reconstruction.
											saver.Add( 30, &saveVersion );
										} };
		SFields ai{ [&]( IBinSaver &saver ) { saver.Add( 1, &common ); } };
		// Restore only the real pathfinding inputs; no movement or world notifications are needed.
		LoadFields( unit,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 1, &ai );
									saver.Add( 2, &unitStats );
								} );
	}

	void SetUp() override
	{
		// BasePathUnit restores this registered singleton when its saved path fields are loaded.
		NSingleton::RegisterSingleton( new CCommonPathFinder(), CCommonPathFinder::tidTypeID );
		map = new CAIMap( 16, 16, 64, 2, 16 );
		// A constructed map still needs terrain types and clearance initialized before pathfinding.
		CTerrain *terrain = map->GetTerrain();
		terrain->PrepareTerraTypes( 1 );
		terrain->SetTerraTypes( 0, 1.0f, EAC_NONE, 0, 0 );
		CArray2D<uint8_t> types( 8, 8 );
		types.FillZero();
		terrain->UpdateTypes( 0, 0, 8, 8, types );
		terrain->FinishInitMode();
		SetAIMap( map );
		external.emplace_back( 1, map );
		unit = dynamic_cast<CArtillery *>( NObjectFactory::MakeObject( 0x1108D4AB ) );
		ASSERT_NE( unit, nullptr );
		auto *stats = dynamic_cast<NDb::SMechUnitRPGStats *>( MakeRecord( "MechUnitRPGStats" )->GetObject() );
		ASSERT_NE( stats, nullptr );
		stats->etype = NDb::RPG_TYPE_ART_ROCKET;
		stats->nAIPassabilityClass = EAC_WHELL;
		stats->nBoundTileRadius = 1;
		unitStats = stats;
		auto *bridge = dynamic_cast<NDb::SBridgeRPGStats *>( MakeRecord( "BridgeRPGStats" )->GetObject() );
		ASSERT_NE( bridge, nullptr );
		bridge->edirection = NDb::SBridgeRPGStats::HORIZONTAL;
		bridgeStats = bridge;
		for ( const float x : { 500.0f, 700.0f } )
		{
			CObj<CBridgeSpan> span = dynamic_cast<CBridgeSpan *>( NObjectFactory::MakeObject( 0x1108D49F ) );
			ASSERT_NE( span, nullptr );
			CVec3 center( x, 500, 0 );
			SRect bounds;
			bounds.InitRect( CVec2( x, 500 ), CVec2( 1, 0 ), 64, 32 );
			SFields placement{ [&]( IBinSaver &saver )
			                   {
													 saver.Add( 2, &center );
													 saver.Add( 3, &bounds );
												 } };
			LoadFields( span,
			            [&]( IBinSaver &saver )
			            {
										saver.Add( 1, &placement );
										saver.Add( 2, &bridgeStats );
									} );
			spans.push_back( span );
		}
		PlaceUnit( CVec2( 400, 400 ) );
	}

	void TearDown() override
	{
		SetAIMap( nullptr );
		NSingleton::UnRegisterSingleton( CCommonPathFinder::tidTypeID );
	}

	void BlockTerrain()
	{
		std::list<SObjTileInfo> tiles;
		for ( int y = 0; y < map->GetSizeY(); ++y )
		{
			for ( int x = 0; x < map->GetSizeX(); ++x )
			{
				tiles.emplace_back( SVector( x, y ), EAC_WHELL );
			}
		}
		map->GetTerrain()->AddStaticObjectTiles( tiles );
	}
};

std::filesystem::path BridgePaths::directory;

TEST_F( BridgePaths, BothFailedPathsReturnNoEndpoint )
{
	BlockTerrain();
	EXPECT_EQ( CBridgeCreation::SortBridgeSpans( &spans, unit ), VNULL2 );
}

TEST_F( BridgePaths, AReachableFirstEndpointDoesNotRequireTheOtherPath )
{
	BlockTerrain();
	// The real pathfinder accepts the current tile even when all neighboring tiles are blocked.
	PlaceUnit( firstEnd );
	EXPECT_EQ( CBridgeCreation::SortBridgeSpans( &spans, unit ), firstEnd );
}

TEST_F( BridgePaths, AReachableSecondEndpointDoesNotRequireTheOtherPath )
{
	BlockTerrain();
	PlaceUnit( secondEnd );
	EXPECT_EQ( CBridgeCreation::SortBridgeSpans( &spans, unit ), secondEnd );
}

TEST_F( BridgePaths, ReachablePathsStillChooseTheShorterRoute )
{
	EXPECT_EQ( CBridgeCreation::SortBridgeSpans( &spans, unit ), secondEnd );
}
} // namespace
