#include "../stdafx.h"

#include "GameX/DBMPConsts.h"
#include "GameX/GameRoomData.h"
#include "GameX/MPCoopRules.h"
#include "GameX/MultiplayerCommandManager.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "SceneB2/Cursor.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/Text.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "UI/DBUserInterface.h"
#include "UI/test/MakeDescription.h"
#include "UI/UIComponents.h"
#include "UI/Window.h"

#include <chrono>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

namespace
{
// The real host action submits its message here without running a network session.
class CCreateGameSink : public IMPToUIManager
{
	OBJECT_BASIC_METHODS( CCreateGameSink );

public:
	CObj<SMPUIMessage> message;

	void AddUIMessage( SMPUIMessage *value ) override
	{
		message = value;
	}

	void AddUIMessage( EMPUIMessageType ) override {}

	SMPUIMessage *GetUIMessage() override
	{
		return message;
	}

	SMPUIMessage *PeekUIMessage() override
	{
		return message;
	}

	void MPUISegment() override {}

	bool SaveReplay( const std::string & ) override
	{
		return false;
	}

	int operator&( IBinSaver & ) override
	{
		return 0;
	}
};

// Screen destruction resets the cursor even when Init never loaded a screen.
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

CPtr<NDb::SWindowListCtrl> ListDescription()
{
	auto borderShared = NUITest::MakeDescription<NDb::SWindowSimpleShared>();
	SetPlacement( &borderShared->placement, 200, 100 );
	auto border = NUITest::MakeDescription<NDb::SWindowSimple>();
	border->nClassTypeID = 0x110772C1;
	border->pShared = borderShared;
	SetPlacement( &border->placement, 200, 100 );
	auto itemShared = NUITest::MakeDescription<NDb::SWindowListItemShared>();
	SetPlacement( &itemShared->placement, 200, 20 );
	auto item = NUITest::MakeDescription<NDb::SWindowListItem>();
	item->nClassTypeID = 0x11075B85;
	item->pShared = itemShared;
	SetPlacement( &item->placement, 200, 20 );
	auto shared = NUITest::MakeDescription<NDb::SWindowListSharedData>();
	shared->pBorder = border;
	shared->pListItem = item;
	SetPlacement( &shared->placement, 200, 100 );
	auto desc = NUITest::MakeDescription<NDb::SWindowListCtrl>();
	desc->nClassTypeID = 0x11075B86;
	desc->pShared = shared;
	SetPlacement( &desc->placement, 200, 100 );
	return desc;
}

CWindow *MakeCombo()
{
	auto shared = NUITest::MakeDescription<NDb::SWindowComboBoxShared>();
	shared->pList = ListDescription();
	const auto *listShared = dynamic_cast<const NDb::SWindowListSharedData *>( shared->pList->pShared.GetPtr() );
	shared->pLine = listShared->pListItem;
	SetPlacement( &shared->placement, 200, 20 );
	auto desc = NUITest::MakeDescription<NDb::SWindowComboBox>();
	desc->nClassTypeID = 0x17122340;
	desc->pShared = shared;
	SetPlacement( &desc->placement, 200, 20 );
	return CUIFactory::MakeWindow( desc );
}

CWindow *MakeButton()
{
	auto shared = NUITest::MakeDescription<NDb::SWindowMSButtonShared>();
	shared->visualStates.resize( 1 );
	SetPlacement( &shared->placement, 100, 20 );
	auto desc = NUITest::MakeDescription<NDb::SWindowMSButton>();
	desc->nClassTypeID = 0x11075B87;
	desc->pShared = shared;
	desc->buttonStates.resize( 1 );
	SetPlacement( &desc->placement, 100, 20 );
	return CUIFactory::MakeWindow( desc );
}

CWindow *MakeSimpleWindow( const char *name )
{
	auto shared = NUITest::MakeDescription<NDb::SWindowSimpleShared>();
	SetPlacement( &shared->placement, 200, 100 );
	auto desc = NUITest::MakeDescription<NDb::SWindowSimple>();
	desc->nClassTypeID = 0x110772C1;
	desc->szName = name;
	desc->pShared = shared;
	desc->bVisible = true;
	SetPlacement( &desc->placement, 200, 100 );
	return CUIFactory::MakeWindow( desc );
}

struct SCreateInterfaceFields
{
	CPtr<IWindow> main;
	CDBPtr<NDb::SMultiplayerMap> selected;
	CPtr<IListControl> maps;
	CPtr<IEditLine> session;
	CPtr<IComboBox> tech;
	CPtr<IComboBox> count;
	CPtr<IButton> create;
	CPtr<IEditLine> captureTime;
	CPtr<IEditLine> password;
	CPtr<IComboBox> difficulty;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &main );
		saver.Add( 4, &selected );
		saver.Add( 5, &maps );
		saver.Add( 6, &session );
		saver.Add( 7, &tech );
		saver.Add( 8, &count );
		saver.Add( 11, &create );
		saver.Add( 13, &captureTime );
		saver.Add( 20, &password );
		saver.Add( 21, &difficulty );
		return 0;
	}
};

class MPDifficultySelection : public testing::Test
{
protected:
	std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<CObj<IWindow>> windows;
	std::vector<SBinSaverExternalObject> external;
	CObj<ICursor> cursor;
	CObj<CCreateGameSink> sink;
	CObj<CObjectBase> hostInterface;
	IProgrammedReactionsAndChecks *reactions = nullptr;
	SCreateInterfaceFields fields;
	CPtr<IWindow> difficultyLabel;
	NDb::SMapInfo *mission = nullptr;

	template <class T>
	T *CreateRecord( const char *className )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( className );
		EXPECT_NE( record, nullptr );
		if ( !record )
		{
			return nullptr;
		}
		const std::string name = std::to_string( records.size() ) + ".xdb";
		EXPECT_TRUE( NDb::AddNewObject( name, CDBID( name ), record ) );
		records.push_back( record );
		return dynamic_cast<T *>( record->GetObject() );
	}

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

	NDb::SMultiplayerMap *MakeMap( int levels )
	{
		auto *map = CreateRecord<NDb::SMultiplayerMap>( "MultiplayerMap" );
		mission = CreateRecord<NDb::SMapInfo>( "MapInfo" );
		map->pMap = mission;
		map->eGameType = NDb::COOP;
		map->nPlayers = 2;
		mission->players.resize( 4 );
		for ( int i = 0; i < 3; ++i )
		{
			mission->players[i].ePlayerType = i == 2 ? NDb::AI_BOT : NDb::HUMAN;
			mission->players[i].nDiplomacySide = i == 2 ? 1 : 0;
		}
		mission->players.back().nDiplomacySide = 2;
		for ( int i = 0; i < levels; ++i )
		{
			auto *level = CreateRecord<NDb::SDifficultyLevel>( "DifficultyLevel" );
			level->szLocalizedNameFileRef = std::to_string( i ) + ".txt";
			mission->customDifficultyLevels.push_back( level );
		}
		return map;
	}

	void SetUp() override
	{
		// This exported call anchors GameX and its registered interface factory.
		EXPECT_FALSE( NGameX::IsCoopGame( SB2GameSpecificData() ) );
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		Singleton<IUIInitialization>()->SetUIConsts( nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-mp-difficulty-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( MP_DIFFICULTY_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		auto *constants = CreateRecord<NDb::SMultiplayerConsts>( "MultiplayerConsts" );
		auto *party = CreateRecord<NDb::SPartyDependentInfo>( "PartyDependentInfo" );
		ASSERT_NE( constants, nullptr );
		ASSERT_NE( party, nullptr );
		constants->techLevels.resize( 1 );
		constants->sides.resize( 1 );
		constants->sides[0].pPartyInfo = party;
		constants->sides[0].techLevels.resize( 1 );
		NGlobal::SetVar( "mp_consts", "0.xdb" );
		WriteText( "0.txt", u"Veteran" );
		WriteText( "1.txt", u"Recruit" );
		WriteText( "2.txt", u"Regular" );
		fields.selected = MakeMap( 3 );

		cursor = new CTestCursor();
		NSingleton::RegisterSingleton( cursor, ICursor::tidTypeID );
		sink = new CCreateGameSink();
		NSingleton::RegisterSingleton( sink, IMPToUIManager::tidTypeID );
		hostInterface = NObjectFactory::MakeObject( 0x32197340 );
		ASSERT_NE( hostInterface, nullptr );
		reactions = dynamic_cast<IProgrammedReactionsAndChecks *>( hostInterface.GetPtr() );
		ASSERT_NE( reactions, nullptr );
		fields.main = KeepWindow( MakeSimpleWindow( "Main" ) );
		difficultyLabel = KeepWindow( MakeSimpleWindow( "DifficultyLabel" ) );
		fields.main->AddChild( difficultyLabel, false );
		fields.maps = dynamic_cast<IListControl *>( KeepWindow( CUIFactory::MakeWindow( ListDescription() ) ) );
		// A null row payload keeps the selected map supplied through the interface serializer.
		auto *row = fields.maps->AddItem( nullptr );
		dynamic_cast<IScrollableContainer *>( fields.maps.GetPtr() )->Select( dynamic_cast<IWindow *>( row ) );
		fields.tech = dynamic_cast<IComboBox *>( KeepWindow( MakeCombo() ) );
		fields.count = dynamic_cast<IComboBox *>( KeepWindow( MakeCombo() ) );
		fields.difficulty = dynamic_cast<IComboBox *>( KeepWindow( MakeCombo() ) );
		fields.tech->Select( 0 );
		fields.create = dynamic_cast<IButton *>( KeepWindow( MakeButton() ) );
		// These controls only supply their initially empty text; no graphics or font is needed.
		fields.session =
				dynamic_cast<IEditLine *>( KeepWindow( dynamic_cast<IWindow *>( NObjectFactory::MakeObject( 0x11075B83 ) ) ) );
		fields.captureTime =
				dynamic_cast<IEditLine *>( KeepWindow( dynamic_cast<IWindow *>( NObjectFactory::MakeObject( 0x11075B83 ) ) ) );
		fields.password =
				dynamic_cast<IEditLine *>( KeepWindow( dynamic_cast<IWindow *>( NObjectFactory::MakeObject( 0x11075B83 ) ) ) );
	}

	void TearDown() override
	{
		hostInterface = nullptr;
		fields = SCreateInterfaceFields();
		difficultyLabel = nullptr;
		windows.clear();
		external.clear();
		NSingleton::UnRegisterSingleton( IMPToUIManager::tidTypeID );
		sink = nullptr;
		NSingleton::UnRegisterSingleton( ICursor::tidTypeID );
		cursor = nullptr;
		NSingleton::UnRegisterSingleton( IUIInitialization::tidTypeID );
		NGlobal::RemoveVar( "mp_consts" );
		records.clear();
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		NVFS::SetMainFileCreator( nullptr );
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	void SelectMap()
	{
		// Exercise the private serializer and public reaction without adding test-only APIs.
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		{
			CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
			loader->AddPolymorphicBase( 1, hostInterface.GetPtr() );
		}
		ASSERT_TRUE( reactions->Execute( "MapList", "react_on_select_map" ) );
	}

	void ChooseDifficulty( int index )
	{
		auto *notify = dynamic_cast<ISelectNotify *>( fields.difficulty.GetPtr() );
		ASSERT_NE( notify, nullptr );
		// This is the actual dropdown-row selection callback, not a fixture-owned index.
		notify->OnSelectData( fields.difficulty->GetItem( index )->GetUserData() );
		ASSERT_EQ( fields.difficulty->GetSelectedIndex(), index );
	}

	void ExpectSubmission( int index )
	{
		sink->message = nullptr;
		ASSERT_TRUE( reactions->Execute( "CreateGame", "react_on_create" ) );
		const auto *message = dynamic_cast<SMPUICreateGameMessage *>( sink->message.GetPtr() );
		ASSERT_NE( message, nullptr );
		EXPECT_EQ( message->specificInfo.pMPMap, fields.selected );
		EXPECT_EQ( message->specificInfo.nDifficultyLevel, index );
	}
};

TEST_F( MPDifficultySelection, AuthoredOrderAndThirdSelectionReachCreateGameMessage )
{
	SelectMap();
	EXPECT_TRUE( fields.difficulty->IsVisible() );
	EXPECT_TRUE( difficultyLabel->IsVisible() );
	ASSERT_EQ( fields.difficulty->GetItemCount(), 3 );
	EXPECT_EQ( fields.difficulty->GetSelectedIndex(), 0 );
	const wchar_t *names[] = { L"Veteran", L"Recruit", L"Regular" };
	for ( int i = 0; i < 3; ++i )
	{
		const auto *data = dynamic_cast<CTextData *>( fields.difficulty->GetItem( i )->GetUserData() );
		ASSERT_NE( data, nullptr );
		EXPECT_EQ( data->wszText, names[i] );
	}
	ChooseDifficulty( 2 );
	ExpectSubmission( 2 );

	fields.selected = MakeMap( 3 );
	SelectMap();
	EXPECT_EQ( fields.difficulty->GetSelectedIndex(), 2 );
	ExpectSubmission( 2 );
}

TEST_F( MPDifficultySelection, MapSwitchResetsAnUnavailableOrMissingSelectedLevel )
{
	SelectMap();
	ChooseDifficulty( 2 );
	fields.selected = MakeMap( 1 );
	SelectMap();
	ASSERT_EQ( fields.difficulty->GetItemCount(), 1 );
	EXPECT_EQ( fields.difficulty->GetSelectedIndex(), 0 );
	ExpectSubmission( 0 );

	fields.selected = MakeMap( 3 );
	SelectMap();
	ChooseDifficulty( 2 );
	fields.selected = MakeMap( 3 );
	mission->customDifficultyLevels[2] = nullptr;
	SelectMap();
	EXPECT_EQ( fields.difficulty->GetItemCount(), 3 );
	EXPECT_EQ( fields.difficulty->GetSelectedIndex(), 0 );
	ExpectSubmission( 0 );
}

TEST_F( MPDifficultySelection, EmptyListAndOlderScreenWithoutControlSubmitZero )
{
	SelectMap();
	ChooseDifficulty( 2 );
	fields.selected = MakeMap( 0 );
	SelectMap();
	ASSERT_EQ( fields.difficulty->GetItemCount(), 1 );
	EXPECT_FALSE( fields.difficulty->IsVisible() );
	EXPECT_FALSE( difficultyLabel->IsVisible() );
	EXPECT_FALSE( fields.difficulty->IsEnabled() );
	EXPECT_EQ( fields.difficulty->GetSelectedIndex(), 0 );
	const auto *data = dynamic_cast<CTextData *>( fields.difficulty->GetItem( 0 )->GetUserData() );
	ASSERT_NE( data, nullptr );
	EXPECT_EQ( data->wszText, L"Default" );
	ExpectSubmission( 0 );

	// Returning to a configurable COOP map restores both controls and starts at zero.
	fields.selected = MakeMap( 3 );
	SelectMap();
	EXPECT_TRUE( fields.difficulty->IsVisible() );
	EXPECT_TRUE( fields.difficulty->IsEnabled() );
	EXPECT_TRUE( difficultyLabel->IsVisible() );
	EXPECT_EQ( fields.difficulty->GetSelectedIndex(), 0 );
	ChooseDifficulty( 2 );
	ExpectSubmission( 2 );

	fields.selected = MakeMap( 3 );
	fields.difficulty = nullptr;
	SelectMap();
	EXPECT_FALSE( difficultyLabel->IsVisible() );
	ExpectSubmission( 0 );
}

TEST_F( MPDifficultySelection, ClassicMapHidesDifficultyAndDiscardsPreviousCoopSelection )
{
	SelectMap();
	ChooseDifficulty( 2 );
	ExpectSubmission( 2 );
	auto *classic = MakeMap( 3 );
	classic->eGameType = NDb::CLASSIC;
	fields.selected = classic;
	SelectMap();
	EXPECT_FALSE( fields.difficulty->IsVisible() );
	EXPECT_FALSE( difficultyLabel->IsVisible() );
	EXPECT_EQ( fields.difficulty->GetSelectedIndex(), 0 );
	ExpectSubmission( 0 );

	fields.selected = MakeMap( 3 );
	SelectMap();
	EXPECT_TRUE( fields.difficulty->IsVisible() );
	EXPECT_TRUE( difficultyLabel->IsVisible() );
	EXPECT_EQ( fields.difficulty->GetSelectedIndex(), 0 );
	ChooseDifficulty( 2 );
	ExpectSubmission( 2 );
}
} // namespace
