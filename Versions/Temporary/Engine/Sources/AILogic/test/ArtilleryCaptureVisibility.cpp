#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../Artillery.h"
#include "../B2AI.h"
#include "../Diplomacy.h"
#include "../Formation.h"
#include "../GlobalWarFog.h"
#include "../NewUpdater.h"
#include "../Soldier.h"
#include "../Technics.h"
#include "../UnitGuns.h"
#include "../Units.h"
#include "../UnitStates.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/CommonPathFinder.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/BinSaver.h"
#include "System/Streams.h"
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

class ArtilleryCaptureVisibility : public testing::TestWithParam<int>
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<CObj<CObjectBase>> objects;
	std::vector<SBinSaverExternalObject> external;
	CObj<CAIMap> map;
	CMemoryStream updaterState;
	CPtr<IAILogic> logic;
	CPtr<CUnitGuns> guns;
	CDBPtr<NDb::SMechUnitRPGStats> stats;
	CPtr<NDb::SUnitStatsModifier> modifier;

protected:
	CPtr<CFormation> crew;
	CPtr<CArtillery> artillery;
	CPtr<CSoldier> soldier;

	static void SetUpTestSuite()
	{
		// Load the real unit factories on Windows as well as ELF.
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-artillery-visibility-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( ARTILLERY_CAPTURE_TYPES_FILE, directory / "types.xml" );
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

	template <class T>
	T *MakeObject( int nType )
	{
		T *object = dynamic_cast<T *>( NObjectFactory::MakeObject( nType ) );
		EXPECT_NE( object, nullptr );
		if ( object )
		{
			objects.push_back( object );
			external.emplace_back( int( external.size() + 1 ), object );
		}
		return object;
	}

	template <class T>
	T *MakeRecord( const char *type )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( type );
		EXPECT_NE( record, nullptr );
		static unsigned nextRecord = 0;
		const std::string name = std::string( type ) + std::to_string( nextRecord++ ) + ".xdb";
		EXPECT_TRUE( NDb::AddNewObject( name, CDBID( name ), record ) );
		records.push_back( record );
		return dynamic_cast<T *>( record->GetObject() );
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

	void SetUp() override
	{
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &updaterState, SAVER_MODE_WRITE );
			saver->Add( 1, &updater );
		}
		// Neither party has a rendering grid when the headless fixture applies its fog updates.
		CMemoryStream observerStream;
		int neutralParty = 2;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &observerStream, SAVER_MODE_WRITE );
			saver->Add( 10, &neutralParty );
		}
		observerStream.Seek( 0 );
		{
			CPtr<IBinSaver> loader = CreateBinSaver( &observerStream, SAVER_MODE_READ );
			updater.operator&( *loader );
		}
		NSingleton::RegisterSingleton( new CCommonPathFinder(), CCommonPathFinder::tidTypeID );
		logic = MakeObject<IAILogic>( 0x1108D441 );
		NSingleton::RegisterSingleton( logic, IAILogic::tidTypeID );
		logic->SetNPlayers( 2 );
		logic->SetNetGame( true );
		theDipl.SetParty( 1, 1 );
		logic->SetMyDiplomacyInfo( GetParam(), GetParam() );
		map = new CAIMap( 64, 64, 32, 2, 16 );
		SetAIMap( map );
		units.Init();
		theWarFog.Init( 64, 64, 16, 2.0f );
		artillery = MakeObject<CArtillery>( AI_ARTILLERY );
		crew = MakeObject<CFormation>( 0x1108D498 );
		soldier = MakeObject<CSoldier>( AI_INFANTRY );
		guns = MakeObject<CUnitGuns>( 0x1108D4C7 );
		artillery->nUniqueID = 101;
		soldier->nUniqueID = 102;
		modifier = MakeRecord<NDb::SUnitStatsModifier>( "UnitStatsModifier" );
		objects.push_back( modifier.GetPtr() );
		external.emplace_back( 1000, modifier );
		auto *artilleryStats = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
		artilleryStats->etype = NDb::RPG_TYPE_ART_GUN;
		artilleryStats->pActions = MakeRecord<NDb::SUnitActions>( "UnitActions" );
		artilleryStats->fSight = 4;
		stats = artilleryStats;
		uint8_t owner = 1 - GetParam();
		std::vector<CPtr<CSoldier>> soldiers{ soldier };
		LoadFields( crew,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 5, &owner );
									saver.Add( 24, &soldiers );
								} );
		SetVisibilitySnapshot( true, false );
	}

	void TearDown() override
	{
		theWarFog.Clear();
		updaterState.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &updaterState, SAVER_MODE_READ );
		loader->Add( 1, &updater );
		units.Clear();
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		objects.clear();
		SetAIMap( nullptr );
		NSingleton::UnRegisterSingleton( CCommonPathFinder::tidTypeID );
	}

	void SetVisibilitySnapshot( bool bGunVisible, bool bCrewVisible, bool bAlreadyOwned = false )
	{
		CUnitStatsModifiers modifiers;
		modifiers.Set( "test.fixture", modifier );
		// The gun has already been processed by visibility. Its new crew has a different cached value.
		uint8_t owner = crew->GetPlayer();
		uint8_t gunOwner = bAlreadyOwned ? owner : theDipl.GetNeutralPlayer();
		std::vector<int> gunVisibility{ bGunVisible, bGunVisible, false };
		std::vector<int> crewVisibility{ bCrewVisible, bCrewVisible, false };
		crewVisibility[owner] = true;
		std::vector<NTimer::STime> lastSeen( 3, 1 );
		std::vector<int> disappearing( 3, false );
		SFields ai{ [&]( IBinSaver &saver )
		            {
									saver.Add( 6, &gunOwner );
									saver.Add( 44, &disappearing );
									saver.Add( 46, &lastSeen );
									saver.Add( 51, &gunVisibility );
									saver.Add( 58, &modifiers );
								} };
		LoadFields( artillery,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 1, &ai );
									saver.Add( 2, &stats );
									saver.Add( 4, &guns );
								} );
		SFields soldierAI{ [&]( IBinSaver &saver )
		                   {
												 saver.Add( 6, &owner );
												 saver.Add( 51, &crewVisibility );
											 } };
		SFields soldierFields{ [&]( IBinSaver &saver ) { saver.Add( 1, &soldierAI ); } };
		LoadFields( soldier, [&]( IBinSaver &saver ) { saver.Add( 1, &soldierFields ); } );
		artillery->SetCurState( MakeObject<IUnitState>( 0x1508D494 ) );
	}

	void PrepareSoldierObserver()
	{
		auto *infantryStats = MakeRecord<NDb::SInfantryRPGStats>( "InfantryRPGStats" );
		infantryStats->fSight = 12;
		infantryStats->fSpyDiscoveryRange = 12;
		CDBPtr<NDb::SInfantryRPGStats> statsPointer = infantryStats;
		CUnitStatsModifiers modifiers;
		modifiers.Set( "test.fixture", modifier );
		SFields ai{ [&]( IBinSaver &saver ) { saver.Add( 58, &modifiers ); } };
		float ownSight = -1;
		SAIAngle maxAngle( 65535 );
		SFields infantry{ [&]( IBinSaver &saver )
		                  {
												saver.Add( 1, &ai );
												saver.Add( 4, &statsPointer );
												saver.Add( 9, &maxAngle );
												saver.Add( 10, &ownSight );
												saver.Add( 18, &crew );
											} };
		LoadFields( soldier, [&]( IBinSaver &saver ) { saver.Add( 1, &infantry ); } );
		soldier->SetVisionAngle( 32768 );
		ASSERT_TRUE( soldier->IsAlive() );
		ASSERT_EQ( soldier->GetFormation(), crew.GetPtr() );
	}

	void PlaceObserver( CAIUnit *unit, SVector tile )
	{
		// Load the completed movement without running pathfinding or the rendering world.
		CMemoryStream stream;
		CVec3 position( tile.x * AI_TILE_SIZE, tile.y * AI_TILE_SIZE, 0 );
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 9, &position );
			saver->Add( 10, &tile );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		unit->CBasePathUnit::operator&( *loader );
	}

	void CaptureAndCheckClient( bool bExpectedVisible )
	{
		// Exercise the real ownership transfer and crew attachment, before the queued serving command runs.
		artillery->SetCrew( crew, false );
		EXPECT_EQ( artillery->GetPlayer(), crew->GetPlayer() );
		for ( int party = 0; party < 3; ++party )
		{
			EXPECT_EQ( soldier->IsVisible( party ), artillery->IsVisible( party ) ) << party;
		}
		EXPECT_TRUE( soldier->IsVisible( crew->GetPlayer() ) );
		EXPECT_EQ( soldier->IsVisible( GetParam() ), bExpectedVisible );

		// Inspect the actual queued notifications without requiring the rendering world's terrain and models.
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->Add( 1, &updater );
		}
		stream.Seek( 0 );
		std::list<CPtr<CEventUpdater::CUpdateData>> pending;
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 2, &pending ); } };
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, &fields );
		bool bCrewUpdate = false;
		bool bOwnershipUpdate = false;
		for ( const auto &update : pending )
		{
			if ( update->eUpdateType == ACTION_NOTIFY_CHANGE_VISIBILITY && update->pObj == soldier )
			{
				bCrewUpdate = true;
				EXPECT_EQ( update->nParam, int( bExpectedVisible ) );
			}
			if ( update->eUpdateType == ACTION_NOTIFY_UPDATE_DIPLOMACY && update->pObj == artillery )
			{
				bOwnershipUpdate = true;
				CObj<CEventUpdater::CUpdateData::IUpdateTransformer> transformer =
						MakeObject<CEventUpdater::CUpdateData::IUpdateTransformer>( 0x120B3347 );
				CPtr<SAIBasicUpdate> client = transformer->Transform( update, 0 );
				auto *diplomacy = dynamic_cast<SAIDiplomacyUpdate *>( client.GetPtr() );
				ASSERT_NE( diplomacy, nullptr );
				EXPECT_EQ( diplomacy->info.nPlayer, crew->GetPlayer() );
				EXPECT_EQ( diplomacy->info.eDiplomacy, EDI_ENEMY );
			}
		}
		EXPECT_TRUE( bCrewUpdate );
		EXPECT_TRUE( bOwnershipUpdate );
	}
};

std::filesystem::path ArtilleryCaptureVisibility::directory;

TEST_P( ArtilleryCaptureVisibility, VisibleNeutralGunRevealsPreviouslyHiddenCrew )
{
	CaptureAndCheckClient( true );
}

TEST_P( ArtilleryCaptureVisibility, HiddenGunDoesNotRevealCrewToOpponent )
{
	SetVisibilitySnapshot( false, true );
	CaptureAndCheckClient( false );
}

TEST_P( ArtilleryCaptureVisibility, ReattachingCrewAlsoRefreshesVisibilityWithoutOwnershipChange )
{
	SetVisibilitySnapshot( true, false, true );
	CaptureAndCheckClient( true );
}

TEST_P( ArtilleryCaptureVisibility, ServingCrewWithdrawsPersonalSightAndRestoresItAtTheDetachPosition )
{
	const SVector captureTile( 16, 16 );
	const SVector destination( 44, 44 );
	const SVector beyondGunSight( 8, 0 );
	const int party = theDipl.GetNParty( crew->GetPlayer() );
	PrepareSoldierObserver();
	PlaceObserver( soldier, captureTile );
	PlaceObserver( artillery, captureTile );
	SWarFogUnitInfo info;
	soldier->GetFogInfo( &info );
	ASSERT_EQ( info.nRadius, 6 );
	ASSERT_EQ( info.vPos, SVector( 8, 8 ) );
	ASSERT_TRUE( info.sector.IsWholeCircle() );
	theWarFog.AddUnit( soldier->GetUniqueId(), info, party );
	artillery->SetCrew( crew, false );
	theWarFog.Segment();
	ASSERT_TRUE( theWarFog.IsTileVisible( captureTile + beyondGunSight, party ) );
	ASSERT_TRUE( theWarFog.IsUnitVisible( captureTile + beyondGunSight, party, true ) );

	// The stationary gun sees less far than its new gunner.
	CPtr<IUnitState> serving = MakeObject<IUnitState>( 0x1108D4BF );
	LoadFields( serving, [&]( IBinSaver &saver ) { saver.Add( 8, &artillery ); } );
	crew->SetCurState( serving );
	theWarFog.Segment();
	EXPECT_TRUE( theWarFog.IsTileVisible( captureTile, party ) );
	EXPECT_FALSE( theWarFog.IsTileVisible( captureTile + beyondGunSight, party ) );
	EXPECT_FALSE( theWarFog.IsUnitVisible( captureTile + beyondGunSight, party, true ) );

	PlaceObserver( soldier, destination );
	PlaceObserver( artillery, destination );
	soldier->WarFogChanged();
	artillery->WarFogChanged();
	theWarFog.Segment();
	EXPECT_FALSE( theWarFog.IsTileVisible( captureTile, party ) );
	EXPECT_TRUE( theWarFog.IsTileVisible( destination, party ) );
	EXPECT_FALSE( theWarFog.IsTileVisible( destination + beyondGunSight, party ) );

	crew->SetCurState( nullptr );
	theWarFog.Segment();
	EXPECT_TRUE( theWarFog.IsTileVisible( destination + beyondGunSight, party ) );
	EXPECT_TRUE( theWarFog.IsUnitVisible( destination + beyondGunSight, party, true ) );
	EXPECT_FALSE( theWarFog.IsTileVisible( captureTile, party ) );
}

TEST_P( ArtilleryCaptureVisibility, LoadedStationaryCrewWithdrawsLegacySightAtItsOldPosition )
{
	const SVector oldTile( 16, 16 );
	const int party = theDipl.GetNParty( crew->GetPlayer() );
	PrepareSoldierObserver();
	PlaceObserver( soldier, oldTile );
	SWarFogUnitInfo info;
	soldier->GetFogInfo( &info );
	theWarFog.AddUnit( soldier->GetUniqueId(), info, party );
	theWarFog.Segment();
	ASSERT_TRUE( theWarFog.IsTileVisible( oldTile, party ) );

	// Old saves can contain a fully applied observer at the capture point while the gunner is already elsewhere.
	PlaceObserver( soldier, SVector( 44, 44 ) );
	CPtr<IUnitState> serving = MakeObject<IUnitState>( 0x1108D4BF );
	SFields queue{ [&]( IBinSaver &saver ) { saver.Add( 2, &serving ); } };
	SFields common{ [&]( IBinSaver &saver ) { saver.Add( 3, &queue ); } };
	bool disabled = true;
	LoadFields( crew,
	            [&]( IBinSaver &saver )
	            {
								saver.Add( 1, &common );
								saver.Add( 9, &disabled );
							} );
	ASSERT_EQ( crew->GetState(), serving.GetPtr() );
	crew->Segment();
	theWarFog.Segment();
	EXPECT_FALSE( theWarFog.IsTileVisible( oldTile, party ) );
	EXPECT_FALSE( theWarFog.IsUnitVisible( oldTile, party, true ) );
}

INSTANTIATE_TEST_SUITE_P( BothHumanViewpoints, ArtilleryCaptureVisibility, testing::Values( 0, 1 ) );
} // namespace
