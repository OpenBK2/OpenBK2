#include "../stdafx.h"

#include "GameX/CommandsHistory.h"
#include "GameX/ReplayPlayerInfo.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "port/time.h"
#include "SceneB2/Cursor.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/Text.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "UI/DBUserInterface.h"
#include "UI/test/MakeDescription.h"
#include "UI/WindowScrollableContainerBase.h"

#include <chrono>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

namespace
{
std::vector<std::vector<std::wstring>> replayCommands;

void CaptureReplay( const std::string &, const std::vector<std::wstring> &parameters )
{
	replayCommands.push_back( parameters );
}

// The interface destructor resets its cursor even when no screen was loaded.
class CTestCursor : public ICursor
{
	OBJECT_BASIC_METHODS( CTestCursor );

public:
	bool Init() override
	{
		return true;
	}

	void RegisterMode( int, const std::string & ) override {}

	bool SetMode( int ) override
	{
		return true;
	}

	void OnSetCursor() override {}

	void Step() override {}

	void Show( bool ) override {}

	void SetBounds( int, int, int, int ) override {}

	void Acquire( bool ) override {}

	void SetPos( int, int ) override {}

	const CVec2 GetPos() const override
	{
		return VNULL2;
	}

	void CanShow( bool ) override {}

	int operator&( IBinSaver & ) override
	{
		return 0;
	}
};

void SetPlacement( NDb::SWindowPlacement *placement, float width, float height )
{
	placement->position = VNULL2;
	placement->size = CVec2( width, height );
	placement->lowerMargin = VNULL2;
	placement->upperMargin = VNULL2;
	placement->horAllign = NDb::EPA_LOW_END;
	placement->verAllign = NDb::EPA_LOW_END;
}

CPtr<NDb::SWindowSimple> SimpleDescription( float width, float height )
{
	auto shared = NUITest::MakeDescription<NDb::SWindowSimpleShared>();
	SetPlacement( &shared->placement, width, height );
	auto desc = NUITest::MakeDescription<NDb::SWindowSimple>();
	desc->nClassTypeID = 0x110772C1;
	desc->pShared = shared;
	desc->bVisible = true;
	SetPlacement( &desc->placement, width, height );
	return desc;
}

CWindow *MakeScrollWindow()
{
	auto border = SimpleDescription( 300, 300 );
	border->placement.horAllign = NDb::EPA_MARGIN;
	border->placement.verAllign = NDb::EPA_MARGIN;
	auto shared = NUITest::MakeDescription<NDb::SWindowScrollableContainerShared>();
	SetPlacement( &shared->placement, 300, 300 );
	shared->pBorder = border;
	auto desc = NUITest::MakeDescription<NDb::SWindowScrollableContainer>();
	desc->nClassTypeID = 0x170AF301;
	desc->pShared = shared;
	desc->bVisible = true;
	SetPlacement( &desc->placement, 300, 300 );
	return CUIFactory::MakeWindow( desc );
}

struct SReplayEntryFields
{
	std::string name;
	SSystemTime time = {};
	CPtr<IWindow> item;
	SMultiplayerReplayInfo info;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &name );
		saver.Add( 3, &time );
		saver.Add( 4, &item );
		saver.Add( 5, &info );
		return 0;
	}
};

struct SReplayInterfaceFields
{
	bool saveMode = false;
	std::vector<SReplayEntryFields> replays;
	int selected = -1;
	CPtr<IScrollableContainer> replayList;
	CPtr<IScrollableContainer> playerList;
	CPtr<IWindow> playerTemplate;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &saveMode );
		saver.Add( 3, &replays );
		saver.Add( 4, &selected );
		saver.Add( 12, &replayList );
		saver.Add( 17, &playerList );
		saver.Add( 18, &playerTemplate );
		return 0;
	}
};

class ReplayPlayerSelection : public testing::Test
{
protected:
	std::filesystem::path directory;
	CObj<NDb::IObjMan> mapRecord;
	NDb::SMapInfo *map = nullptr;
	CObj<ICursor> cursor;
	CObj<CObjectBase> replayInterface;
	IProgrammedReactionsAndChecks *reactions = nullptr;
	std::vector<CObj<IWindow>> windows;
	std::vector<SBinSaverExternalObject> external;
	SReplayInterfaceFields fields;
	CWindowScrollableContainerBase *players = nullptr;
	const std::wstring format = L"<color=0xffabcdef><center>";

	void WriteText( const std::string &name, const std::u16string &text )
	{
		{
			std::ofstream file( directory / name, std::ios::binary );
			for ( char16_t character : std::u16string( 1, u'\ufeff' ) + text )
			{
				file.put( char( character & 0xff ) );
				file.put( char( character >> 8 ) );
			}
		}
		NText::Reload( name );
	}

	IWindow *KeepWindow( IWindow *window )
	{
		windows.emplace_back( window );
		external.emplace_back( static_cast<int>( external.size() ) + 1, window );
		return window;
	}

	CPtr<NDb::SWindowTextView> TextDescription( const char *name )
	{
		auto textFormat = NUITest::MakeDescription<NDb::STextFormat>();
		textFormat->szFormatStringFileRef = "format.txt";
		auto shared = NUITest::MakeDescription<NDb::SWindowTextViewShared>();
		// A zero-width real text control stores and formats text without a font/device.
		SetPlacement( &shared->placement, 0, 20 );
		auto desc = NUITest::MakeDescription<NDb::SWindowTextView>();
		desc->nClassTypeID = 0x11075B8C;
		desc->szName = name;
		desc->pShared = shared;
		desc->pTextFormat = textFormat;
		desc->szTextFileRef = "old-title.txt";
		desc->bVisible = true;
		SetPlacement( &desc->placement, 0, 20 );
		return desc;
	}

	void SetUp() override
	{
		// Anchor GameX before its registered replay-interface factory is used.
		EXPECT_TRUE( NGameX::GetReplayPlayers( SMultiplayerReplayInfo() ).empty() );
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		Singleton<IUIInitialization>()->SetUIConsts( nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-replay-selection-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( REPLAY_SELECTION_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		mapRecord = NDb::CreateNewObject( "MapInfo" );
		ASSERT_NE( mapRecord, nullptr );
		ASSERT_TRUE( NDb::AddNewObject( "map.xdb", CDBID( "map.xdb" ), mapRecord ) );
		map = dynamic_cast<NDb::SMapInfo *>( mapRecord->GetObject() );
		ASSERT_NE( map, nullptr );
		map->players.resize( 7 );
		map->players.back().nDiplomacySide = 2;
		WriteText( "format.txt", u"<color=0xffabcdef><center>" );
		WriteText( "old-title.txt", u"Old localized team title" );

		cursor = new CTestCursor();
		NSingleton::RegisterSingleton( cursor, ICursor::tidTypeID );
		replayInterface = NObjectFactory::MakeObject( 0x19288C40 );
		ASSERT_NE( replayInterface, nullptr );
		reactions = dynamic_cast<IProgrammedReactionsAndChecks *>( replayInterface.GetPtr() );
		ASSERT_NE( reactions, nullptr );
		fields.replayList = dynamic_cast<IScrollableContainer *>( KeepWindow( MakeScrollWindow() ) );
		fields.playerList = dynamic_cast<IScrollableContainer *>( KeepWindow( MakeScrollWindow() ) );
		players = dynamic_cast<CWindowScrollableContainerBase *>( fields.playerList.GetPtr() );
		ASSERT_NE( players, nullptr );

		auto row = SimpleDescription( 280, 40 );
		row->szName = "ItemTemplate";
		auto shared = NUITest::MakeDescription<NDb::SWindowSimpleShared>();
		SetPlacement( &shared->placement, 280, 40 );
		row->pShared = shared;
		for ( const char *name : { "ItemHeaderWin", "ItemHeaderLose", "ItemPlayers" } )
		{
			shared->children.push_back( TextDescription( name ).GetPtr() );
		}
		for ( const char *name : { "ItemFlag0", "ItemFlag1" } )
		{
			auto flag = SimpleDescription( 0, 0 );
			flag->szName = name;
			shared->children.push_back( flag.GetPtr() );
		}
		fields.playerTemplate = KeepWindow( CUIFactory::MakeWindow( row ) );

		// RegisterCmd chains all handlers, so remove the game's playback handler first.
		// This executable never steps MainLoop or starts a mission.
		NGlobal::RemoveVar( "replay" );
		NGlobal::RegisterCmd( "replay", CaptureReplay );
		replayCommands.clear();
	}

	void TearDown() override
	{
		replayInterface = nullptr;
		reactions = nullptr;
		fields = SReplayInterfaceFields();
		windows.clear();
		external.clear();
		NSingleton::UnRegisterSingleton( ICursor::tidTypeID );
		cursor = nullptr;
		NGlobal::RemoveVar( "replay" );
		NSingleton::UnRegisterSingleton( IUIInitialization::tidTypeID );
		mapRecord = nullptr;
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		NVFS::SetMainFileCreator( nullptr );
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	SMultiplayerReplayInfo ClassicInfo()
	{
		SMultiplayerReplayInfo info;
		info.pMap = map;
		info.nWinningSide = 1;
		info.slots.resize( 6 );
		for ( int player : { 1, 3, 5 } )
		{
			info.slots[player].bPresent = true;
			info.slots[player].nTeam = player == 1 ? 0 : 1;
			info.slots[player].szName = "Human " + std::to_string( player );
		}
		return info;
	}

	SMultiplayerReplayInfo CooperativeInfo()
	{
		SMultiplayerReplayInfo info = ClassicInfo();
		info.nMultiplayerMode = 1;
		info.nScriptResult = 2;
		info.slots[3].bPresent = false;
		for ( int player : { 1, 5 } )
		{
			map->players[player].ePlayerType = NDb::HUMAN;
			map->players[player].nDiplomacySide = 0;
			info.slots[player].nTeam = 0;
		}
		info.bCoopPlayersRecorded = true;
		for ( int player : { 0, 2 } )
		{
			map->players[player].ePlayerType = NDb::AI_BOT;
			map->players[player].nDiplomacySide = 1;
			auto &bot = info.coopPlayers.emplace_back();
			bot.nPlayerIndex = player;
			bot.wszName = L"General " + std::to_wstring( player );
		}
		return info;
	}

	void AddReplay( const std::string &name, const SMultiplayerReplayInfo &info )
	{
		auto &entry = fields.replays.emplace_back();
		entry.name = name;
		entry.info = info;
		entry.item = KeepWindow( CUIFactory::MakeWindow( SimpleDescription( 280, 40 ) ) );
		fields.replayList->PushBack( entry.item, true );
	}

	void HydrateInterface()
	{
		// Feed the real private serializer instead of exposing UI internals for tests.
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, replayInterface.GetPtr() );
	}

	void SelectReplay( int index )
	{
		fields.replayList->Select( fields.replays[index].item );
		ASSERT_TRUE( reactions->Execute( "ReplayList", "react_on_select_replay" ) );
	}

	void ClickPlayer( int row )
	{
		IWindow *window = players->GetItem( row );
		ASSERT_NE( window, nullptr );
		const CTRect<float> rect = dynamic_cast<CWindow *>( window )->GetWindowRect();
		const CVec2 point( ( rect.x1 + rect.x2 ) * 0.5f, ( rect.y1 + rect.y2 ) * 0.5f );
		window->OnButtonDown( point, MSTATE_BUTTON1 );
		window->OnButtonUp( point, MSTATE_BUTTON1 );
	}

	void ExpectLoad( const std::wstring &name, int player )
	{
		replayCommands.clear();
		ASSERT_TRUE( reactions->Execute( "ButtonLoad", "react_on_load" ) );
		ASSERT_EQ( replayCommands.size(), 1 );
		EXPECT_EQ( replayCommands[0], ( std::vector<std::wstring>{ name, std::to_wstring( player ) } ) );
	}

	void ExpectRow( int row, const std::wstring &name, const std::wstring &result )
	{
		IWindow *window = players->GetItem( row );
		ASSERT_NE( window, nullptr );
		auto *title = dynamic_cast<ITextView *>( window->GetChild( "ItemHeaderWin", false ) );
		auto *status = dynamic_cast<ITextView *>( window->GetChild( "ItemPlayers", false ) );
		ASSERT_NE( title, nullptr );
		ASSERT_NE( status, nullptr );
		EXPECT_EQ( title->GetText(), format + name );
		EXPECT_EQ( status->GetText(), format + result );
		EXPECT_FALSE( window->GetChild( "ItemHeaderLose", false )->IsVisible() );
	}
};

TEST_F( ReplayPlayerSelection, SecondAndThirdClassicRowsKeepSparsePlayerIDsAfterSwitchingReplays )
{
	AddReplay( "First match", ClassicInfo() );
	AddReplay( "Second match", ClassicInfo() );
	HydrateInterface();
	for ( int replay : { 0, 1, 0 } )
	{
		SelectReplay( replay );
		const std::wstring name = replay == 0 ? L"First match" : L"Second match";
		ExpectRow( 0, L"Human 1", L"Loser" );
		ExpectRow( 1, L"Human 3", L"Winner" );
		ExpectRow( 2, L"Human 5", L"Winner" );
		ClickPlayer( 1 );
		ExpectLoad( name, 3 );
		ClickPlayer( 2 );
		ExpectLoad( name, 5 );
		if ( replay == 1 )
		{
			// Reproduce the allocator-ID drift which broke the old team lookup.
			EXPECT_GT( fields.playerList->GetItemNumber( players->GetItem( 1 ) ), 2 );
		}
	}
}

TEST_F( ReplayPlayerSelection, HumansAndBotsAreIndividualSelectableViewpointsAfterHumanDefeat )
{
	AddReplay( "Cooperative defeat", CooperativeInfo() );
	AddReplay( "Cooperative defeat copy", CooperativeInfo() );
	HydrateInterface();
	for ( int replay : { 0, 1 } )
	{
		SelectReplay( replay );
		const std::wstring name = replay == 0 ? L"Cooperative defeat" : L"Cooperative defeat copy";
		ExpectLoad( name, 1 ); // The first human remains the default, even with bot slot zero.
		ExpectRow( 0, L"General 0", L"Winner" );
		ExpectRow( 1, L"Human 1", L"Loser" );
		ExpectRow( 2, L"General 2", L"Winner" );
		ClickPlayer( 1 );
		ExpectLoad( name, 1 );
		ClickPlayer( 2 );
		ExpectLoad( name, 2 );
		ClickPlayer( 3 );
		ExpectLoad( name, 5 );
	}
}

TEST_F( ReplayPlayerSelection, UnfinishedCooperativeRowsReplaceBothLocalizedTeamTitles )
{
	auto info = CooperativeInfo();
	info.nScriptResult = 0;
	AddReplay( "Unfinished mission", info );
	HydrateInterface();
	SelectReplay( 0 );
	ExpectRow( 0, L"General 0", L"Unfinished" );
	ExpectRow( 1, L"Human 1", L"Unfinished" );
	ExpectRow( 2, L"General 2", L"Unfinished" );
}

TEST_F( ReplayPlayerSelection, InvalidBotRowsAndUnmappedSelectionsCannotStartPlayback )
{
	auto info = CooperativeInfo();
	map->players[2].nDiplomacySide = 2;
	AddReplay( "Invalid bot", info );
	HydrateInterface();
	SelectReplay( 0 );
	ASSERT_NE( players->GetItem( 2 ), nullptr );
	EXPECT_FALSE( players->GetItem( 2 )->IsEnabled() );
	ClickPlayer( 2 );
	ExpectLoad( L"Invalid bot", 1 );

	// Even forced/stale selections must pass the actual playback validation.
	fields.playerList->Select( players->GetItem( 2 ) );
	replayCommands.clear();
	EXPECT_TRUE( reactions->Execute( "ButtonLoad", "react_on_load" ) );
	EXPECT_TRUE( replayCommands.empty() );
	fields.playerList->Select( fields.playerTemplate );
	EXPECT_TRUE( reactions->Execute( "ButtonLoad", "react_on_load" ) );
	EXPECT_TRUE( replayCommands.empty() );
}
} // namespace
