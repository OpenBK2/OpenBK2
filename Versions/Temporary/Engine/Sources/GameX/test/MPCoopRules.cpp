#include "../stdafx.h"

#include "GameX/DBMPConsts.h"
#include "GameX/MPCoopRules.h"
#include "GameX/MPInterfaceData.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "UI/DBUserInterface.h"

#include <chrono>
#include <filesystem>

#include <gtest/gtest.h>

namespace
{
class MPCoopRules : public testing::Test
{
protected:
	std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	NDb::SMultiplayerMap *map = nullptr;
	NDb::SMapInfo *mission = nullptr;
	NDb::SMultiplayerConsts *constants = nullptr;
	NDb::SPartyDependentInfo *party = nullptr;
	SB2GameSpecificData game;
	std::string error;

	template <class T>
	T *CreateRecord( const char *pszClass )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( pszClass );
		EXPECT_NE( record, nullptr );
		if ( !record )
		{
			return nullptr;
		}
		records.push_back( record );
		return dynamic_cast<T *>( record->GetObject() );
	}

	void SetUp() override
	{
		// The editor DB constructs loaded resources inside their registering DLL.
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-coop-rules-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( MP_COOP_RULES_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		map = CreateRecord<NDb::SMultiplayerMap>( "MultiplayerMap" );
		mission = CreateRecord<NDb::SMapInfo>( "MapInfo" );
		constants = CreateRecord<NDb::SMultiplayerConsts>( "MultiplayerConsts" );
		party = CreateRecord<NDb::SPartyDependentInfo>( "PartyDependentInfo" );
		ASSERT_NE( map, nullptr );
		ASSERT_NE( mission, nullptr );
		ASSERT_NE( constants, nullptr );
		ASSERT_NE( party, nullptr );
		map->pMap = mission;
		map->eGameType = NDb::COOP;
		map->nPlayers = 3;
		mission->players.resize( 6 );
		for ( int i = 0; i < 5; ++i )
		{
			const bool bBot = i == 0 || i == 2;
			mission->players[i].ePlayerType = bBot ? NDb::AI_BOT : NDb::HUMAN;
			mission->players[i].nDiplomacySide = bBot ? 1 : 0;
		}
		mission->players.back().nDiplomacySide = 2;
		constants->techLevels.resize( 2 );
		constants->sides.resize( 3 );
		constants->playerColorInfos.resize( 4 );
		for ( auto &side : constants->sides )
		{
			side.pPartyInfo = party;
			side.techLevels.resize( 2 );
		}
		constants->sides[0].techLevels[1].bDisabled = true;
		game.pMPMap = map;
		game.nPlayers = 2;
		game.nTechLevel = 0;
	}

	void TearDown() override
	{
		game.pMPMap = nullptr;
		records.clear();
		NDb::CloseDatabase();
		std::error_code cleanupError;
		std::filesystem::remove_all( directory, cleanupError );
	}

	std::vector<SMPSlot> ReadySlots()
	{
		std::vector<SMPSlot> slots( mission->players.size() );
		for ( int i : { 1, 3 } )
		{
			slots[i].bPresent = true;
			slots[i].bAccept = true;
			slots[i].nTeam = 0;
			slots[i].nCountry = 1;
			slots[i].nColour = i;
		}
		return slots;
	}
};

TEST_F( MPCoopRules, ResolvesForcedSettingsWithoutTurningUnrestrictedTechIntoMinusOne )
{
	ASSERT_TRUE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) ) << error;
	EXPECT_EQ( game.nTechLevel, 0 );
	EXPECT_TRUE( NGameX::IsCoopGame( game ) );
	EXPECT_FALSE( game.bRandomPlacement );
	EXPECT_EQ( game.nTimeLimit, 0 );
	map->nForcedTechLevel = 1;
	map->nDifficultyLevel = 7;
	ASSERT_TRUE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) ) << error;
	EXPECT_EQ( game.nTechLevel, 1 );
	EXPECT_EQ( game.nDifficultyLevel, 7 );
	EXPECT_EQ( NGameX::GetAllowedMultiplayerNations( game, constants ), ( std::vector<int>{ 1, 2 } ) );
}

TEST_F( MPCoopRules, PreservesSparseMapIDsAndExcludesBotsNeutralAndExcessHumanSlots )
{
	ASSERT_TRUE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) ) << error;
	EXPECT_EQ( NGameX::GetMultiplayerSlotCount( game ), 6 );
	for ( int i = -1; i <= 6; ++i )
	{
		EXPECT_EQ( NGameX::IsHumanMultiplayerSlot( game, i ), i == 1 || i == 3 ) << i;
	}
	game.nPlayers = 3;
	EXPECT_TRUE( NGameX::IsHumanMultiplayerSlot( game, 4 ) );
}

TEST_F( MPCoopRules, RejectsUnavailableWhitelistAndInvalidIndices )
{
	map->nForcedTechLevel = 1;
	map->allowedNations = { 0 };
	EXPECT_FALSE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) );
	map->allowedNations = { 2, 2, 1 };
	ASSERT_TRUE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) ) << error;
	EXPECT_EQ( NGameX::GetAllowedMultiplayerNations( game, constants ), ( std::vector<int>{ 1, 2 } ) );
	map->allowedNations = { 3 };
	EXPECT_FALSE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) );
	map->allowedNations.clear();
	map->nForcedTechLevel = 2;
	EXPECT_FALSE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) );
	map->nForcedTechLevel = -2;
	EXPECT_FALSE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) );
}

TEST_F( MPCoopRules, JoiningUsesOnlyOpenHumanSlotsAndCountsPresentPeers )
{
	ASSERT_TRUE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) ) << error;
	std::vector<SMPSlot> slots( NGameX::GetMultiplayerSlotCount( game ) );
	EXPECT_EQ( NGameX::FindAvailableMultiplayerSlot( game, slots ), 1 );
	slots[1].bPresent = true;
	slots[1].nClientID = 40;
	slots[3].bAccept = true;
	// Open-looking bot/neutral holes and excess HUMAN positions cannot replace
	// the closed second human slot, even though the room has spare capacity.
	EXPECT_EQ( NGameX::GetPresentMultiplayerPlayerCount( slots ), 1 );
	EXPECT_EQ( NGameX::FindAvailableMultiplayerSlot( game, slots ), -1 );
	slots[3].bAccept = false;
	EXPECT_EQ( NGameX::FindAvailableMultiplayerSlot( game, slots ), 3 );
	slots[3].bPresent = true;
	EXPECT_EQ( NGameX::GetPresentMultiplayerPlayerCount( slots ), 2 );
	EXPECT_EQ( NGameX::FindAvailableMultiplayerSlot( game, slots ), -1 );

	// Closing/removing a peer frees one place, independent of previous counters.
	slots[1].Clear();
	slots[1].bAccept = true;
	EXPECT_EQ( NGameX::GetPresentMultiplayerPlayerCount( slots ), 1 );
	EXPECT_EQ( NGameX::FindAvailableMultiplayerSlot( game, slots ), -1 );
	game.nPlayers = 3;
	EXPECT_EQ( NGameX::FindAvailableMultiplayerSlot( game, slots ), 4 );
}

TEST_F( MPCoopRules, RejectsInvalidRolesNeutralPlacementAndMaskOverflow )
{
	mission->players[0].nDiplomacySide = 0;
	EXPECT_FALSE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) );
	mission->players[0].nDiplomacySide = 1;
	mission->players[2].nDiplomacySide = 2;
	EXPECT_FALSE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) );
	mission->players[2].nDiplomacySide = 1;
	mission->players.resize( 17 );
	mission->players.back().nDiplomacySide = 2;
	EXPECT_FALSE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) );
}

TEST_F( MPCoopRules, StartValidationRejectsInvalidSlotsTeamsNationsAndSingleHuman )
{
	map->allowedNations = { 1 };
	ASSERT_TRUE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) ) << error;
	auto slots = ReadySlots();
	EXPECT_TRUE( NGameX::ValidateMultiplayerLobby( game, slots, &error, constants ) ) << error;
	slots[0] = slots[1];
	EXPECT_FALSE( NGameX::ValidateMultiplayerLobby( game, slots, &error, constants ) );
	slots[0].bPresent = false;
	slots[3].nTeam = 1;
	EXPECT_FALSE( NGameX::ValidateMultiplayerLobby( game, slots, &error, constants ) );
	slots[3].nTeam = 0;
	slots[3].nCountry = 2;
	EXPECT_FALSE( NGameX::ValidateMultiplayerLobby( game, slots, &error, constants ) );
	slots[3].nCountry = 1;
	slots[3].bPresent = false;
	EXPECT_FALSE( NGameX::ValidateMultiplayerLobby( game, slots, &error, constants ) );
}

TEST_F( MPCoopRules, ClassicRetainsBothTeamsRequirement )
{
	map->eGameType = NDb::CLASSIC;
	game.nTimeLimit = 60;
	ASSERT_TRUE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) ) << error;
	std::vector<SMPSlot> slots( 2 );
	for ( int i = 0; i < 2; ++i )
	{
		slots[i].bPresent = true;
		slots[i].bAccept = true;
		slots[i].nColour = i;
	}
	EXPECT_FALSE( NGameX::ValidateMultiplayerLobby( game, slots, &error, constants ) );
	slots[1].nTeam = 1;
	EXPECT_TRUE( NGameX::ValidateMultiplayerLobby( game, slots, &error, constants ) ) << error;
	EXPECT_EQ( game.nTimeLimit, 60 );
}
} // namespace
