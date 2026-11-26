#include "../stdafx.h"

#include "Client/ServerClientInterface.h"
#include "GameX/DBMPConsts.h"
#include "GameX/InterfaceState.h"
#include "GameX/MPCoopRules.h"
#include "GameX/MPManagerModeNivalNet.hpp"
#include "GameX/MultiplayerNetPackets.h"
#include "GameX/ScenarioTracker.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/NetSaver.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/ConsoleBuffer.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "UI/DBUserInterface.h"

#include <chrono>
#include <filesystem>

#include <gtest/gtest.h>

namespace
{
constexpr int HOST_ID = 10;
constexpr int LOCAL_ID = 20;
constexpr int OTHER_ID = 30;

struct SStartReached
{
};

// Stop at the first mission-loading callback after the real start packet handler.
// This observes successful start authorization without creating a graphics scene.
class CStartBoundary : public IInterfaceState
{
	OBJECT_NOCOPY_METHODS( CStartBoundary );
	std::wstring emptyText;

public:
	const NDb::SGameRoot *GetGameRoot() const override
	{
		return nullptr;
	}

	const NDb::SUIConstsB2 *GetUIConsts() const override
	{
		return nullptr;
	}

	CDBID GetScreenEntryDBID( const std::string &szName ) const override
	{
		return {};
	}

	const NDb::SUIScreenEntry *GetScreenEntry( const std::string &szName ) const override
	{
		return nullptr;
	}

	const std::wstring &GetTextEntry( const std::string &szTextID ) const override
	{
		return emptyText;
	}

	const NDb::SComplexSoundDesc *GetSoundEntry( const std::string &szName ) const override
	{
		return nullptr;
	}

	const NDb::STexture *GetTextureEntry( const std::string &szID ) const override
	{
		return nullptr;
	}

	const std::wstring &GetMPGameType( enum NDb::EMPGameType eType ) const override
	{
		return emptyText;
	}

	std::wstring GetSeasonName( enum NDb::ESeason eSeason ) const override
	{
		return {};
	}

	std::wstring GetMapSizeName( const NDb::SMapInfo *pMapInfo ) const override
	{
		return {};
	}

	NGScene::CScreenshotTexture *GetScreenShotTexture() override
	{
		return nullptr;
	}

	int GetDifficulty() const override
	{
		return {};
	}

	void SetDifficulty( int nDifficulty ) override {}

	ICampaignState *GetCampaign( const CDBID &dbidCampaign ) override
	{
		return nullptr;
	}

	void StartSingleMission( const CDBID &dbidCampaign, int nChapterNumber, int nMissionNumber, int nDifficulty ) override
	{
	}

	void SetMissionConsoleColor( uint32_t dwColor ) override {}

	void WriteToMissionConsole( const std::wstring &wszText ) override {}

	void WriteToMissionConsoleSelected( const std::wstring &wszText ) override {}

	std::wstring GetMissionConsoleMLTag() const override
	{
		return {};
	}

	bool IsSuppressEnableFocus() const override
	{
		return {};
	}

	void SetSuppressEnableFocus( bool bSuppress ) override {}

	void SendCommandsToCloseAllIncluding( struct IInterfaceBase *pLastInterfaceToClose ) override {}

	void SendCommandsToCloseAllIncluding( const std::string &szInterfaceID ) override {}

	void MakeScenarioTracker( EScenarioTrackerType eType ) override {}

	void VerifyScenarioTracker( EScenarioTrackerType eType ) const override
	{
		throw SStartReached();
	}

	bool IsTransitEffectFlag() const override
	{
		return {};
	}

	void SetTransitEffectFlag( bool bTransit ) override {}

	bool IsFirstTimeInChapter() const override
	{
		return {};
	}

	void SetFirstTimeInChapter( bool bFirstTime ) override {}

	bool IsAutoShowCommanderScreen() const override
	{
		return {};
	}

	void SetAutoShowCommanderScreen( bool bAutoShow ) override {}

	const NDb::STexture *GetMenuBackgroundTexture() const override
	{
		return nullptr;
	}

	std::wstring GetRandomCitation() override
	{
		return {};
	}

	int GetAndRegisterIDForMLHandler( const std::vector<std::pair<std::wstring, std::wstring>> &params ) override
	{
		return {};
	}

	void UnregisterIDForMLHandler( int nID ) override {}

	const std::wstring &ExpandMLTag( const std::wstring &wszTag, int nIDForHandler ) const override
	{
		return emptyText;
	}

	void AddMPChatMessage( const std::wstring &wszText ) override {}

	std::wstring GetMPChatMessage() override
	{
		return {};
	}

	void ClearMPChatMessages() override {}

	std::wstring FilterMPChatText( const std::wstring &wszText ) override
	{
		return {};
	}

	int GetTutorialRecommendedMission() const override
	{
		return {};
	}

	void MarkTutorialRecommendedMission( int nMission ) override {}

	void ApplyTutorialRecommendedMission() override {}
};

class CRecordingClient : public IServerClient
{
	OBJECT_NOCOPY_METHODS( CRecordingClient );

public:
	std::vector<CPtr<CNetPacket>> sent;

	CNetPacket *GetPacket() override
	{
		return nullptr;
	}

	void SendPacket( CNetPacket *packet ) override
	{
		sent.push_back( packet );
	}

	void SendGamePacket( CNetPacket *packet, bool ) override
	{
		sent.push_back( packet );
	}

	void Segment() override {}
};
} // namespace

// Restore only lobby fixture state; packet dispatch and start authorization use the DLL.
class CMPPlayerDropTestAccess
{
public:
	static void Initialize( CMPManagerModeNivalNet *manager,
	                        IServerClient *client,
	                        const SB2GameSpecificData &game,
	                        const std::vector<SMPSlot> &slots,
	                        int ownSlot,
	                        bool host = false )
	{
		manager->eState = CMPManagerModeNivalNet::EGS_GAME_ROOM;
		manager->nMyClientID = host ? HOST_ID : LOCAL_ID;
		manager->bHost = host;
		manager->bLadderGame = false;
		manager->bGameRoomInit = true;
		manager->pClient = client;
		manager->gameDesc = game;
		manager->manualPlacement.clear();
		std::vector<SMPSlot> snapshot( slots );
		manager->slots.swap( snapshot );
		manager->nOwnSlot = ownSlot;
		manager->nHostClientID = HOST_ID;
		manager->nGameID = 123;
		manager->nSlotsUsed = NGameX::GetPresentMultiplayerPlayerCount( slots );
	}

	static const std::vector<SMPSlot> &GetSlots( const CMPManagerMode *manager )
	{
		return manager->slots;
	}

	static int GetOwnSlot( const CMPManagerMode *manager )
	{
		return manager->nOwnSlot;
	}

	static int GetPlayerCount( const CMPManagerMode *manager )
	{
		return manager->nSlotsUsed;
	}

	static int GetTeamWithLowestScore( CMPManagerMode *manager )
	{
		return manager->GetTeamWithLowestScore();
	}

	static bool HasStarted( const CMPManagerModeNivalNet *manager )
	{
		return manager->eState == CMPManagerModeNivalNet::EGS_GAME_STARTED;
	}

	static void WaitForLadderStart( CMPManagerModeNivalNet *manager )
	{
		manager->eState = CMPManagerModeNivalNet::EGS_LADDER_WAIT_CLIENTS;
		manager->bLadderGame = true;
	}
};

namespace
{
class MPStartRoster : public testing::Test
{
protected:
	std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	NDb::SMultiplayerConsts *constants = nullptr;
	NDb::SMultiplayerMap *map = nullptr;
	NDb::SMapInfo *mission = nullptr;
	NDb::SPartyDependentInfo *party = nullptr;
	SB2GameSpecificData game;
	std::vector<SMPSlot> hostSlots;
	CObj<CMPManagerModeNivalNet> manager;
	CObj<CRecordingClient> client;
	CObj<CStartBoundary> boundary;
	CObj<IConsoleBuffer> console;
	std::string error;

	template <class T>
	T *CreateRecord( const char *name )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( name );
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
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-start-roster-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( MP_START_ROSTER_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		constants = CreateRecord<NDb::SMultiplayerConsts>( "MultiplayerConsts" );
		map = CreateRecord<NDb::SMultiplayerMap>( "MultiplayerMap" );
		mission = CreateRecord<NDb::SMapInfo>( "MapInfo" );
		party = CreateRecord<NDb::SPartyDependentInfo>( "PartyDependentInfo" );
		ASSERT_NE( constants, nullptr );
		ASSERT_NE( map, nullptr );
		ASSERT_NE( mission, nullptr );
		ASSERT_NE( party, nullptr );
		constants->techLevels.resize( 1 );
		constants->sides.resize( 1 );
		constants->sides[0].pPartyInfo = party;
		constants->sides[0].techLevels.resize( 1 );
		constants->playerColorInfos.resize( 4 );
		const std::string constantsName = "mp-" + std::to_string( stamp ) + ".xdb";
		ASSERT_TRUE( NDb::AddNewObject( constantsName, CDBID( constantsName ), records[0] ) );
		NGlobal::SetVar( "mp_consts", constantsName );
		map->pMap = mission;
		map->nPlayers = 3;
		mission->nNumPatchesX = 4;
		mission->nNumPatchesY = 4;
		mission->players.resize( 4 );
		for ( int i = 0; i < mission->players.size(); ++i )
		{
			mission->players[i].pPartyInfo = party;
			mission->players[i].vMPStartPos = CVec2( 512.0f * ( i + 1 ), 1024.0f );
		}
		mission->players.back().nDiplomacySide = 2;
		game.pMPMap = map;
		game.nPlayers = 3;
		hostSlots.resize( 3 );
		for ( int i = 0; i < 3; ++i )
		{
			auto &slot = hostSlots[i];
			slot.bPresent = true;
			slot.bAccept = true;
			slot.nClientID = 10 * ( i + 1 );
			slot.szName = "Player " + std::to_string( i );
			slot.nTeam = i == 2 ? 1 : 0;
			slot.nColour = i;
			slot.bRandomCountry = false;
		}
		ASSERT_TRUE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) ) << error;
		ASSERT_TRUE( NGameX::ValidateMultiplayerLobby( game, hostSlots, &error, constants ) ) << error;
		manager = dynamic_cast<CMPManagerModeNivalNet *>( NObjectFactory::MakeObject( 0x192444C0 ) );
		console = dynamic_cast<IConsoleBuffer *>( NObjectFactory::MakeObject( 0x300C8D40 ) );
		ASSERT_NE( manager, nullptr );
		ASSERT_NE( console, nullptr );
		client = new CRecordingClient;
		boundary = new CStartBoundary;
		NSingleton::RegisterSingleton( boundary, IInterfaceState::tidTypeID );
		NSingleton::RegisterSingleton( console, IConsoleBuffer::tidTypeID );
		CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 1 );
	}

	void TearDown() override
	{
		manager = nullptr;
		client = nullptr;
		NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
		NSingleton::UnRegisterSingleton( IInterfaceState::tidTypeID );
		NSingleton::UnRegisterSingleton( IConsoleBuffer::tidTypeID );
		game.pMPMap = nullptr;
		records.clear();
		NDb::CloseDatabase();
		std::error_code cleanupError;
		std::filesystem::remove_all( directory, cleanupError );
	}

	CObj<CB2GameRoomStartGamePacket> MakeStart( const std::vector<uint8_t> &order = { 0, 1, 2 } )
	{
		CObj<CB2GameRoomStartGamePacket> packet = new CB2GameRoomStartGamePacket( HOST_ID );
		packet->slotRehash = order;
		packet->slots.resize( order.size() );
		for ( int i = 0; i < hostSlots.size(); ++i )
		{
			packet->slots[order[i]] = hostSlots[i];
			packet->slots[order[i]].nClientID = hostSlots[i].nClientID;
		}
		return packet;
	}

	CObj<CB2GameRoomStartGamePacket> RoundTrip( CB2GameRoomStartGamePacket *outgoing )
	{
		CMemoryStream stream;
		{
			CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->AddPolymorphicBase( 1, outgoing );
		}
		CObj<CB2GameRoomStartGamePacket> incoming =
				dynamic_cast<CB2GameRoomStartGamePacket *>( NObjectFactory::MakeObject( 104 ) );
		stream.Seek( 0 );
		{
			CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
			saver->AddPolymorphicBase( 1, incoming.GetPtr() );
		}
		// Transport supplies the authenticated sender outside the packet payload.
		incoming->nClientID = outgoing->nClientID;
		return incoming;
	}

	void Dispatch( CNetPacket *packet )
	{
		CPacketProcessorBase *processor = manager;
		EXPECT_TRUE( processor->ProcessPacket( packet ) );
	}

	void ExpectNoReplyToLobbyPing( int nSender )
	{
		// The fixture stops before creating a transceiver. Dispatch directly to the registered lobby handler.
		const size_t nBefore = client->sent.size();
		CObj<CPingPacket> ping = new CPingPacket( nSender, 0, 100 );
		EXPECT_TRUE( manager->CPacketProcessorBase::ProcessPacket( ping ) );
		EXPECT_EQ( client->sent.size(), nBefore );
	}

	CB2GameRoomStartGamePacket *GetSentStart() const
	{
		for ( const auto &packet : client->sent )
		{
			if ( auto *start = dynamic_cast<CB2GameRoomStartGamePacket *>( packet.GetPtr() ) )
			{
				return start;
			}
		}
		return nullptr;
	}
};

TEST_F( MPStartRoster, WireSnapshotPreservesEveryStartFieldAndClientIdentity )
{
	auto outgoing = MakeStart( { 2, 0, 1 } );
	outgoing->bBaseReinforcementBonuses = false;
	outgoing->slots[1].bRandomCountry = true;
	auto incoming = RoundTrip( outgoing );
	ASSERT_EQ( incoming->slots.size(), 3u );
	EXPECT_EQ( incoming->slotRehash, outgoing->slotRehash );
	EXPECT_FALSE( incoming->bBaseReinforcementBonuses );
	for ( int i = 0; i < 3; ++i )
	{
		const auto &sent = outgoing->slots[i];
		const auto &received = incoming->slots[i];
		EXPECT_EQ( received.nClientID, sent.nClientID );
		EXPECT_EQ( received.bPresent, sent.bPresent );
		EXPECT_EQ( received.szName, sent.szName );
		EXPECT_EQ( received.nColour, sent.nColour );
		EXPECT_EQ( received.nCountry, sent.nCountry );
		EXPECT_EQ( received.nTeam, sent.nTeam );
		EXPECT_EQ( received.bAccept, sent.bAccept );
		EXPECT_EQ( received.bRandomCountry, sent.bRandomCountry );
	}
}

TEST_F( MPStartRoster, HostBroadcastsPlacementAndSendsItsCurrentLayoutToALateJoiner )
{
	const std::vector<uint8_t> order{ 2, 0, 1 };
	hostSlots[2].Clear();
	CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 0, true );
	CMPUIMessageTranslator *translator = manager;
	CObj<SMPUIManualPlacementMessage> enable = new SMPUIManualPlacementMessage( order );
	ASSERT_TRUE( translator->HandleMessage( enable ) );
	ASSERT_EQ( client->sent.size(), 1u );
	auto *broadcast = dynamic_cast<CB2ManualPlacementPacket *>( client->sent.front().GetPtr() );
	ASSERT_NE( broadcast, nullptr );
	EXPECT_EQ( broadcast->manualPlacement, order );
	// LAN transports chunked packets, and the online peer channel uses sequential NetSaver packets.
	for ( const bool online : { false, true } )
	{
		SCOPED_TRACE( online );
		CMemoryStream stream;
		auto makeSaver = [&]( ESaverMode mode )
		{ return online ? CreateNetSaver( &stream, mode ) : CreateBinSaver( &stream, mode ); };
		{
			CObj<IBinSaver> saver = makeSaver( SAVER_MODE_WRITE );
			saver->Add( 1, &client->sent.front() );
		}
		stream.Seek( 0 );
		CPtr<CNetPacket> received;
		{
			CObj<IBinSaver> saver = makeSaver( SAVER_MODE_READ );
			saver->Add( 1, &received );
		}
		auto *decoded = dynamic_cast<CB2ManualPlacementPacket *>( received.GetPtr() );
		ASSERT_NE( decoded, nullptr );
		EXPECT_EQ( decoded->manualPlacement, order );
	}

	client->sent.clear();
	CObj<CNewGameClient> join = new CNewGameClient( OTHER_ID );
	Dispatch( join );
	bool receivedSettings = false;
	bool receivedPlacement = false;
	for ( const auto &packet : client->sent )
	{
		if ( auto *settings = dynamic_cast<CB2GameSpecificInfoPacket *>( packet.GetPtr() ) )
		{
			receivedSettings = settings->nClientID == OTHER_ID;
		}
		if ( auto *placement = dynamic_cast<CB2ManualPlacementPacket *>( packet.GetPtr() ) )
		{
			EXPECT_TRUE( receivedSettings );
			EXPECT_EQ( placement->nClientID, OTHER_ID );
			EXPECT_EQ( placement->manualPlacement, order );
			receivedPlacement = true;
		}
	}
	EXPECT_TRUE( receivedPlacement );

	client->sent.clear();
	CObj<SMPUIManualPlacementMessage> disable = new SMPUIManualPlacementMessage;
	ASSERT_TRUE( translator->HandleMessage( disable ) );
	ASSERT_EQ( client->sent.size(), 1u );
	auto *disabled = dynamic_cast<CB2ManualPlacementPacket *>( client->sent.front().GetPtr() );
	ASSERT_NE( disabled, nullptr );
	EXPECT_TRUE( disabled->manualPlacement.empty() );
}

TEST_F( MPStartRoster, GuestDisplaysOnlyValidPlacementUpdatesSentByTheHost )
{
	const std::vector<uint8_t> order{ 2, 0, 1 };
	CObj<SMPUIManualPlacementMessage> localEdit = new SMPUIManualPlacementMessage( order );
	CMPUIMessageTranslator *translator = manager;
	ASSERT_TRUE( translator->HandleMessage( localEdit ) );
	EXPECT_TRUE( client->sent.empty() );
	EXPECT_EQ( manager->PeekMessage(), nullptr );

	CObj<CB2ManualPlacementPacket> placement = new CB2ManualPlacementPacket( OTHER_ID, order );
	Dispatch( placement );
	EXPECT_EQ( manager->PeekMessage(), nullptr );
	placement->nClientID = HOST_ID;
	placement->manualPlacement = { 0, 0, 1 };
	Dispatch( placement );
	EXPECT_EQ( manager->PeekMessage(), nullptr );

	placement->manualPlacement = order;
	Dispatch( placement );
	CPtr<SMPUIMessage> update = manager->GetMessage();
	auto *preview = dynamic_cast<SMPUIManualPlacementMessage *>( update.GetPtr() );
	ASSERT_NE( preview, nullptr );
	EXPECT_EQ( preview->manualPlacement, order );
	placement->manualPlacement.clear();
	Dispatch( placement );
	update = manager->GetMessage();
	preview = dynamic_cast<SMPUIManualPlacementMessage *>( update.GetPtr() );
	ASSERT_NE( preview, nullptr );
	EXPECT_TRUE( preview->manualPlacement.empty() );
}

TEST_F( MPStartRoster, LadderIgnoresManualPreviewAndStartButAcceptsTheOrdinaryStart )
{
	game.nPlayers = 2;
	hostSlots.resize( 2 );
	hostSlots[1].nTeam = 1;
	CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 1 );
	CMPPlayerDropTestAccess::WaitForLadderStart( manager );

	const std::vector<uint8_t> order{ 1, 2, 0 };
	ASSERT_TRUE( NGameX::IsValidManualPlacement( game, order ) );
	CObj<CB2ManualPlacementPacket> preview = new CB2ManualPlacementPacket( HOST_ID, order );
	Dispatch( preview );
	auto start = MakeStart( order );
	start->bManualPlacement = true;
	Dispatch( start );
	EXPECT_EQ( manager->PeekMessage(), nullptr );
	EXPECT_FALSE( CMPPlayerDropTestAccess::HasStarted( manager ) );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetOwnSlot( manager ), 1 );

	auto ordinaryStart = MakeStart( { 1, 0 } );
	EXPECT_THROW( Dispatch( ordinaryStart ), SStartReached );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetOwnSlot( manager ), 0 );
}

TEST_F( MPStartRoster, HostPacketOvertakesAnotherPeersColourAndReadyUpdates )
{
	std::vector<SMPSlot> stale( hostSlots );
	stale[2].nColour = 1;
	stale[2].bAccept = false;
	CMPPlayerDropTestAccess::Initialize( manager, client, game, stale, 1 );
	auto outgoing = MakeStart();
	auto incoming = RoundTrip( outgoing );
	EXPECT_THROW( Dispatch( incoming ), SStartReached );
	EXPECT_TRUE( CMPPlayerDropTestAccess::HasStarted( manager ) );
	const auto &committed = CMPPlayerDropTestAccess::GetSlots( manager );
	ASSERT_EQ( committed.size(), 3u );
	EXPECT_EQ( committed[2].nColour, 2 );
	EXPECT_TRUE( committed[2].bAccept );
	EXPECT_EQ( committed[2].nClientID, OTHER_ID );
}

TEST_F( MPStartRoster, SnapshotReplacesStalePresenceNamesAndLockedNationFlags )
{
	map->bDisableNationSelection = true;
	std::vector<SMPSlot> stale( hostSlots );
	stale[2].Clear();
	stale[1].szName = "Old name";
	stale[1].bRandomCountry = true;
	ASSERT_TRUE( NGameX::ValidateMultiplayerLobby( game, hostSlots, &error, constants ) ) << error;
	CMPPlayerDropTestAccess::Initialize( manager, client, game, stale, 1 );
	auto packet = MakeStart();
	EXPECT_THROW( Dispatch( packet ), SStartReached );
	const auto &committed = CMPPlayerDropTestAccess::GetSlots( manager );
	EXPECT_TRUE( committed[2].bPresent );
	EXPECT_EQ( committed[2].nClientID, OTHER_ID );
	EXPECT_FALSE( committed[1].bRandomCountry );
	EXPECT_EQ( committed[1].szName, hostSlots[1].szName );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetPlayerCount( manager ), 3 );
}

TEST_F( MPStartRoster, ClassicPermutationMovesLocalPlayerAndPreservesAllClientIDs )
{
	// Fixed nations belong to destination map slots even when player identities move.
	constants->sides.resize( 2 );
	constants->sides[1].pPartyInfo = CreateRecord<NDb::SPartyDependentInfo>( "PartyDependentInfo" );
	constants->sides[1].techLevels.resize( 1 );
	mission->players[0].pPartyInfo = constants->sides[1].pPartyInfo;
	map->bDisableNationSelection = true;
	auto packet = MakeStart( { 2, 0, 1 } );
	packet->slots[0].nCountry = 1;
	EXPECT_THROW( Dispatch( packet ), SStartReached );
	const auto &committed = CMPPlayerDropTestAccess::GetSlots( manager );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetOwnSlot( manager ), 0 );
	EXPECT_EQ( committed[0].nClientID, LOCAL_ID );
	EXPECT_EQ( committed[0].nCountry, 1 );
	EXPECT_EQ( committed[1].nClientID, OTHER_ID );
	EXPECT_EQ( committed[2].nClientID, HOST_ID );
}

TEST_F( MPStartRoster, SparseCoopSnapshotRetainsMapPlayerIDsAndExcludesBots )
{
	map->eGameType = NDb::COOP;
	map->nPlayers = 2;
	game.nPlayers = 2;
	mission->players.resize( 5 );
	for ( int i = 0; i < 4; ++i )
	{
		mission->players[i].nDiplomacySide = i % 2 == 0 ? 1 : 0;
		mission->players[i].ePlayerType = i % 2 == 0 ? NDb::AI_BOT : NDb::HUMAN;
	}
	mission->players.back().nDiplomacySide = 2;
	const SMPSlot host( hostSlots[0] );
	const SMPSlot local( hostSlots[1] );
	hostSlots.clear();
	hostSlots.resize( 5 );
	hostSlots[1] = host;
	hostSlots[1].nClientID = HOST_ID;
	hostSlots[3] = local;
	hostSlots[3].nClientID = LOCAL_ID;
	ASSERT_TRUE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) ) << error;
	ASSERT_TRUE( NGameX::ValidateMultiplayerLobby( game, hostSlots, &error, constants ) ) << error;
	CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 3 );
	auto packet = MakeStart( { 0, 1, 2, 3, 4 } );
	EXPECT_THROW( Dispatch( packet ), SStartReached );
	const auto &committed = CMPPlayerDropTestAccess::GetSlots( manager );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetOwnSlot( manager ), 3 );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetPlayerCount( manager ), 2 );
	for ( int i : { 0, 2, 4 } )
	{
		EXPECT_FALSE( committed[i].bPresent );
		EXPECT_EQ( committed[i].nClientID, -1 );
	}
	EXPECT_EQ( committed[1].nClientID, HOST_ID );
	EXPECT_EQ( committed[3].nClientID, LOCAL_ID );
}

TEST_F( MPStartRoster, RejectsForeignMalformedAndInvalidRostersBeforeCommit )
{
	for ( int invalidCase = 0; invalidCase < 9; ++invalidCase )
	{
		SCOPED_TRACE( invalidCase );
		CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 1 );
		auto packet = MakeStart( { 2, 0, 1 } );
		switch ( invalidCase )
		{
			case 0:
				packet->nClientID = OTHER_ID;
				break;
			case 1:
				packet->slotRehash[0] = packet->slotRehash[1];
				break;
			case 2:
				packet->slots.pop_back();
				break;
			case 3:
				packet->slots[0].nClientID = OTHER_ID;
				break;
			case 4:
				packet->slots[0].bPresent = false;
				break;
			case 5:
				packet->slots[1].nClientID = HOST_ID;
				break;
			case 6:
				packet->slots[2].bPresent = false;
				break;
			case 7:
				packet->slots[1].bAccept = false;
				break;
			case 8:
				packet->slots[1].nColour = packet->slots[0].nColour;
				break;
		}
		EXPECT_NO_THROW( Dispatch( packet ) );
		EXPECT_FALSE( CMPPlayerDropTestAccess::HasStarted( manager ) );
		EXPECT_EQ( CMPPlayerDropTestAccess::GetOwnSlot( manager ), 1 );
		const auto &unchanged = CMPPlayerDropTestAccess::GetSlots( manager );
		for ( int i = 0; i < 3; ++i )
		{
			EXPECT_EQ( unchanged[i].nClientID, hostSlots[i].nClientID );
			EXPECT_EQ( unchanged[i].nColour, hostSlots[i].nColour );
		}
	}
}

TEST_F( MPStartRoster, ManualPlacementCommitsPreviewBeyondRoomCapacityOnHostAndPeer )
{
	game.nPlayers = 2;
	hostSlots.resize( 2 );
	hostSlots[1].nTeam = 1;
	const std::vector<uint8_t> order{ 1, 2, 0 };
	for ( bool bRandom : { false, true } )
	{
		SCOPED_TRACE( bRandom );
		game.bRandomPlacement = bRandom;
		CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 0, true );
		CMPUIMessageTranslator *translator = manager;
		CObj<SMPUIMessage> placement = new SMPUIManualPlacementMessage( order );
		ASSERT_TRUE( translator->HandleMessage( placement ) );
		CObj<SMPUIMessage> message = new SMPUISimpleMessage( EMUI_START_GAME );
		EXPECT_THROW( translator->HandleMessage( message ), SStartReached );
		EXPECT_EQ( CMPPlayerDropTestAccess::GetOwnSlot( manager ), 1 );
		auto *start = GetSentStart();
		ASSERT_NE( start, nullptr );
		ASSERT_TRUE( start->bManualPlacement );
		ASSERT_EQ( start->slotRehash, order );
		ASSERT_EQ( start->slots.size(), 3u );
		EXPECT_FALSE( start->slots[0].bPresent );
		EXPECT_EQ( start->slots[1].nClientID, HOST_ID );
		EXPECT_EQ( start->slots[2].nClientID, LOCAL_ID );
		ExpectNoReplyToLobbyPing( LOCAL_ID );

		// A peer can still have the pre-toggle preview when the final authoritative roster arrives.
		CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 1 );
		auto incoming = RoundTrip( start );
		incoming->nClientID = HOST_ID;
		EXPECT_THROW( Dispatch( incoming ), SStartReached );
		EXPECT_EQ( CMPPlayerDropTestAccess::GetOwnSlot( manager ), 2 );
		const auto &committed = CMPPlayerDropTestAccess::GetSlots( manager );
		ASSERT_EQ( committed.size(), 3u );
		EXPECT_EQ( committed[1].szName, hostSlots[0].szName );
		EXPECT_EQ( committed[2].nColour, hostSlots[1].nColour );
		EXPECT_EQ( CMPPlayerDropTestAccess::GetPlayerCount( manager ), 2 );
		ExpectNoReplyToLobbyPing( HOST_ID );
		client->sent.clear();
	}
}

TEST_F( MPStartRoster, TimedScoreTiebreakStaysWithTheOriginalLobbyTeamAcrossManualPlacements )
{
	game.nPlayers = 2;
	hostSlots.resize( 2 );
	hostSlots[0].nTeam = 1;
	hostSlots[1].nTeam = 0;
	CObj<IScenarioTracker> tracker = dynamic_cast<IScenarioTracker *>( NObjectFactory::MakeObject( 0x1117BB00 ) );
	ASSERT_NE( tracker, nullptr );
	NSingleton::RegisterSingleton( tracker, IScenarioTracker::tidTypeID );
	// All map starts score equally, whichever pair is occupied.
	for ( int i = 0; i < map->nPlayers; ++i )
	{
		tracker->SetStatistics( i, IScenarioTracker::ESK_SCORE, 5 );
	}
	const std::vector<std::vector<uint8_t>> orders{ { 0, 1, 2 }, { 1, 0, 2 }, { 1, 2, 0 } };
	for ( const auto &order : orders )
	{
		SCOPED_TRACE( testing::PrintToString( order ) );
		CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 0, true );
		CMPUIMessageTranslator *translator = manager;
		CObj<SMPUIMessage> placement = new SMPUIManualPlacementMessage( order );
		ASSERT_TRUE( translator->HandleMessage( placement ) );
		CObj<SMPUIMessage> begin = new SMPUISimpleMessage( EMUI_START_GAME );
		EXPECT_THROW( translator->HandleMessage( begin ), SStartReached );
		EXPECT_EQ( CMPPlayerDropTestAccess::GetTeamWithLowestScore( manager ), 0 );
		auto *start = GetSentStart();
		ASSERT_NE( start, nullptr );
		auto incoming = RoundTrip( start );
		incoming->nClientID = HOST_ID;
		// A guest may still have an older team update when the final roster arrives.
		std::vector<SMPSlot> stale( hostSlots );
		stale[0].nTeam = 0;
		CMPPlayerDropTestAccess::Initialize( manager, client, game, stale, 1 );
		EXPECT_THROW( Dispatch( incoming ), SStartReached );
		EXPECT_EQ( CMPPlayerDropTestAccess::GetTeamWithLowestScore( manager ), 0 );
		client->sent.clear();
	}

	// Ordinary placement still gives map slot zero's team the tiebreak.
	CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 1 );
	auto ordinary = MakeStart( { 1, 0 } );
	EXPECT_THROW( Dispatch( ordinary ), SStartReached );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetTeamWithLowestScore( manager ), 1 );
}

TEST_F( MPStartRoster, ManualCoopStartUsesUnoccupiedHumanRoleAndKeepsScriptedSlots )
{
	map->eGameType = NDb::COOP;
	game.nPlayers = 2;
	mission->players.resize( 5 );
	for ( int i = 0; i < 4; ++i )
	{
		mission->players[i].nDiplomacySide = i == 1 ? 1 : 0;
		mission->players[i].ePlayerType = i == 1 ? NDb::AI_BOT : NDb::HUMAN;
	}
	mission->players.back().nDiplomacySide = 2;
	SMPSlot local( hostSlots[1] );
	hostSlots.resize( 5 );
	hostSlots[1].Clear();
	hostSlots[2] = local;
	hostSlots[2].nClientID = LOCAL_ID;
	ASSERT_TRUE( NGameX::ResolveMultiplayerMapRules( &game, &error, constants ) ) << error;
	CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 2 );
	// The third human role is a legal destination even when only two peers may join.
	auto start = MakeStart( { 2, 1, 3, 0, 4 } );
	start->bManualPlacement = true;
	EXPECT_THROW( Dispatch( start ), SStartReached );
	const auto &committed = CMPPlayerDropTestAccess::GetSlots( manager );
	ASSERT_EQ( committed.size(), 5u );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetOwnSlot( manager ), 3 );
	EXPECT_EQ( committed[2].nClientID, HOST_ID );
	for ( int i : { 0, 1, 4 } )
	{
		EXPECT_FALSE( committed[i].bPresent );
	}
}

TEST_F( MPStartRoster, TurningPreviewOffRestoresTheOriginalFixedRoomPlacement )
{
	game.bRandomPlacement = false;
	game.nPlayers = 2;
	hostSlots.resize( 2 );
	hostSlots[1].nTeam = 1;
	CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 0, true );
	CMPUIMessageTranslator *translator = manager;
	CObj<SMPUIMessage> enable = new SMPUIManualPlacementMessage( { 2, 1, 0 } );
	CObj<SMPUIMessage> disable = new SMPUIManualPlacementMessage;
	ASSERT_TRUE( translator->HandleMessage( enable ) );
	ASSERT_TRUE( translator->HandleMessage( disable ) );
	CObj<SMPUIMessage> message = new SMPUISimpleMessage( EMUI_START_GAME );
	EXPECT_THROW( translator->HandleMessage( message ), SStartReached );
	auto *start = GetSentStart();
	ASSERT_NE( start, nullptr );
	EXPECT_FALSE( start->bManualPlacement );
	EXPECT_EQ( start->slotRehash, ( std::vector<uint8_t>{ 0, 1 } ) );
	EXPECT_EQ( CMPPlayerDropTestAccess::GetOwnSlot( manager ), 0 );
}

TEST_F( MPStartRoster, HostBroadcastContainsCompletePostShuffleRoster )
{
	game.bRandomPlacement = true;
	constants->sides.resize( 2 );
	constants->sides[1].pPartyInfo = CreateRecord<NDb::SPartyDependentInfo>( "PartyDependentInfo" );
	constants->sides[1].techLevels.resize( 1 );
	mission->players[1].pPartyInfo = constants->sides[1].pPartyInfo;
	map->bDisableNationSelection = true;
	hostSlots[1].nCountry = 1;
	CMPPlayerDropTestAccess::Initialize( manager, client, game, hostSlots, 0, true );
	CObj<SMPUIMessage> message = new SMPUISimpleMessage( EMUI_START_GAME );
	CMPUIMessageTranslator *translator = manager;
	EXPECT_THROW( translator->HandleMessage( message ), SStartReached );
	auto *start = GetSentStart();
	ASSERT_NE( start, nullptr );
	ASSERT_EQ( start->slots.size(), hostSlots.size() );
	for ( int i = 0; i < hostSlots.size(); ++i )
	{
		const auto &sent = start->slots[start->slotRehash[i]];
		EXPECT_EQ( sent.nClientID, hostSlots[i].nClientID );
		EXPECT_EQ( sent.nColour, hostSlots[i].nColour );
		EXPECT_EQ( sent.nCountry, start->slotRehash[i] == 1 ? 1 : 0 );
		EXPECT_EQ( sent.szName, hostSlots[i].szName );
		EXPECT_TRUE( sent.bPresent );
		EXPECT_TRUE( sent.bAccept );
		EXPECT_FALSE( sent.bRandomCountry );
	}
}
} // namespace
