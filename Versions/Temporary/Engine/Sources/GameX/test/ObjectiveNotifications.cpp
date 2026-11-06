#include "AILogic/stdafx.h"

#include "AILogic/B2AI.h"
#include "AILogic/DBAIConsts.h"
#include "AILogic/NewUpdater.h"
#include "AILogic/Scripts.h"
#include "B2_M1_World/MapObj.h"
#include "B2_M1_World/MissionObjectiveStates.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/Checksums.h"
#include "GameX/DBConsts.h"
#include "GameX/DBGameRoot.h"
#include "GameX/DBMPConsts.h"
#include "GameX/InterfaceNotifications.h"
#include "GameX/InterfaceState.h"
#include "GameX/MessageParts.h"
#include "GameX/ScenarioTracker.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Main/GameTimer.h"
#include "SceneB2/Camera.h"
#include "Script/ScriptWrapper.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "Stats_B2_M1/FeedBackUpdates.h"
#include "System/Text.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "UI/DBUserInterface.h"
#include "UI/test/MakeDescription.h"
#include "UI/Window.h"
#include "UISpecificB2/DBUISpecificB2.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <string>
#include <unordered_map>
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

struct SMapPointerFields
{
	CVec3 position;
	bool bPlaced = false;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 4, &position );
		saver.Add( 5, &bPlaced );
		return 0;
	}
};

struct SNotificationFields
{
	std::list<CObj<CVisualNotifications::SEvent>> events;
	det_map<int, std::wstring> postfixes;
	std::unordered_map<int, std::vector<SMapPointerFields>> pointers;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 20, &events );
		saver.Add( 30, &pointers );
		saver.Add( 32, &postfixes );
		return 0;
	}
};

void Placement( NDb::SWindowPlacement *placement, float width, float height )
{
	placement->position = VNULL2;
	placement->size = CVec2( width, height );
	placement->lowerMargin = VNULL2;
	placement->upperMargin = VNULL2;
	placement->horAllign = NDb::EPA_LOW_END;
	placement->verAllign = NDb::EPA_LOW_END;
}

CPtr<NDb::SWindowSimple> SimpleDescription( const char *name )
{
	auto shared = NUITest::MakeDescription<NDb::SWindowSimpleShared>();
	Placement( &shared->placement, 300, 20 );
	auto desc = NUITest::MakeDescription<NDb::SWindowSimple>();
	desc->nClassTypeID = 0x110772C1;
	desc->szName = name;
	desc->pShared = shared;
	desc->bVisible = true;
	Placement( &desc->placement, 300, 20 );
	return desc;
}

class ObjectiveNotifications : public testing::Test
{
protected:
	std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<SBinSaverExternalObject> external;
	CObj<IScenarioTracker> tracker;
	CObj<IInterfaceState> interfaceState;
	CObj<IVisualNotifications> notifications;
	CObj<IGameTimer> timer;
	CObj<ICamera> camera;
	CObj<IAILogic> logic;
	CObj<IScriptWrapper> vm;
	CScripts scripts;
	CObj<IWindow> parent;
	CObj<IWindow> row;
	CObj<IMiniMap> minimap;
	CObj<CObjectBase> figures;
	CObj<CAIMap> aiMap;
	NDb::SMapInfo *map = nullptr;
	NDb::SMissionObjective *objective = nullptr;
	NDb::STexture *objectiveTexture = nullptr;
	NDb::SNotificationEvent *objectiveEvent = nullptr;
	NDb::SUIConstsB2 *uiConsts = nullptr;

	template <class T>
	T *Record( const char *name )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( name );
		EXPECT_NE( record, nullptr );
		if ( !record )
		{
			return nullptr;
		}
		records.push_back( record );
		const std::string path = std::to_string( records.size() ) + ".xdb";
		EXPECT_TRUE( NDb::AddNewObject( path, CDBID( path ), record ) );
		return dynamic_cast<T *>( record->GetObject() );
	}

	void TextFile( const char *name, const std::u16string &text )
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

	void Hydrate( CObjectBase *object, SFields fields )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, object );
	}

	void MakeRow()
	{
		auto description = SimpleDescription( "ItemTemplate" );
		auto shared = NUITest::MakeDescription<NDb::SWindowSimpleShared>();
		Placement( &shared->placement, 300, 20 );
		description->pShared = shared;
		auto textShared = NUITest::MakeDescription<NDb::SWindowTextViewShared>();
		// Zero width stores actual control text without creating a graphics device or loading fonts.
		Placement( &textShared->placement, 0, 20 );
		auto text = NUITest::MakeDescription<NDb::SWindowTextView>();
		text->nClassTypeID = 0x11075B8C;
		text->szName = "Desc";
		text->pShared = textShared;
		Placement( &text->placement, 0, 20 );
		shared->children.push_back( text.GetPtr() );
		auto buttonShared = NUITest::MakeDescription<NDb::SWindowMSButtonShared>();
		buttonShared->visualStates.resize( 1 );
		Placement( &buttonShared->placement, 20, 20 );
		buttonShared->children.push_back( SimpleDescription( "Icon" ).GetPtr() );
		auto button = NUITest::MakeDescription<NDb::SWindowMSButton>();
		button->nClassTypeID = 0x11075B87;
		button->szName = "IconBtn";
		button->pShared = buttonShared;
		button->buttonStates.resize( 1 );
		Placement( &button->placement, 20, 20 );
		shared->children.push_back( button.GetPtr() );
		row = CUIFactory::MakeWindow( description );
	}

	void SetUp() override
	{
		// Anchor the GameX DLL before constructing its registered classes.
		EXPECT_EQ( NGameX::GetMessagePart( nullptr, L"OBJECTIVE_NOTIFICATION_PREFIX" ), L"Objective: " );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-objective-notifications-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( OBJECTIVE_NOTIFICATION_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		Singleton<IUIInitialization>()->SetUIConsts( nullptr );
		uiConsts = Record<NDb::SUIConstsB2>( "UIConstsB2" );
		ASSERT_NE( uiConsts, nullptr );
		NGlobal::SetVar( "ui_consts", "1.xdb" );
		NGlobal::SetVar( "game_mode_editor", 1 );
		TextFile( "header.txt", u"Hold the \u017d bridge" );
		map = Record<NDb::SMapInfo>( "MapInfo" );
		ASSERT_NE( map, nullptr );
		objective = Record<NDb::SMissionObjective>( "MissionObjective" );
		ASSERT_NE( objective, nullptr );
		objective->szHeaderFileRef = "header.txt";
		map->objectives.assign( 3, objective );
		objectiveEvent = Record<NDb::SNotificationEvent>( "NotificationEvent" );
		ASSERT_NE( objectiveEvent, nullptr );
		objectiveEvent->eType = NDb::NEVT_OBJECTIVE_RECEIVED;
		objectiveEvent->fAutoRemoveTime = 0.5f;
		tracker = CreateScenarioTrackerMultiplayer();
		CDBPtr<NDb::SMapInfo> mapPointer = map;
		Hydrate( tracker, { [&]( IBinSaver &saver ) { saver.Add( 2, &mapPointer ); } } );
		NSingleton::RegisterSingleton( tracker, IScenarioTracker::tidTypeID );
		interfaceState = MakeObject<IInterfaceState>( 0x300C8D44 );
		NSingleton::RegisterSingleton( interfaceState, IInterfaceState::tidTypeID );
		timer = MakeObject<IGameTimer>( 0x10075C05 );
		NSingleton::RegisterSingleton( timer, IGameTimer::tidTypeID );
		camera = CreateCamera();
		NSingleton::RegisterSingleton( camera, ICamera::tidTypeID );
		logic = MakeObject<IAILogic>( 0x1108D441 );
		NSingleton::RegisterSingleton( logic, IAILogic::tidTypeID );
		parent = CUIFactory::MakeWindow( SimpleDescription( "Parent" ) );
		MakeRow();
		minimap = MakeObjectVirtual<IMiniMap>( 0x1508EAC0 );
		figures = NObjectFactory::MakeObject( 0x15099C00 );
		Hydrate( minimap, { [&]( IBinSaver &saver ) { saver.Add( 20, &figures ); } } );
		external.emplace_back( 1, parent );
		external.emplace_back( 2, row );
		external.emplace_back( 3, minimap );
		notifications = MakeObject<IVisualNotifications>( 0x11135C01 );
		int maxEvents = 2;
		int height = 20;
		int bottom = 100;
		std::vector<CDBPtr<NDb::SNotificationEvent>> dbEvents( NDb::NEVT_COUNT );
		dbEvents[NDb::NEVT_OBJECTIVE_RECEIVED] = objectiveEvent;
		Hydrate( notifications,
		         { [&]( IBinSaver &saver )
		           {
								 saver.Add( 2, &minimap );
								 saver.Add( 18, &parent );
								 saver.Add( 19, &row );
								 saver.Add( 22, &maxEvents );
								 saver.Add( 23, &height );
								 saver.Add( 24, &bottom );
								 saver.Add( 25, &dbEvents );
							 } } );
	}

	void TearDown() override
	{
		scripts.Clear();
		vm = nullptr;
		notifications = nullptr;
		external.clear();
		parent = nullptr;
		row = nullptr;
		minimap = nullptr;
		figures = nullptr;
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		logic = nullptr;
		SetAIMap( nullptr );
		aiMap = nullptr;
		NSingleton::UnRegisterSingleton( ICamera::tidTypeID );
		camera = nullptr;
		NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
		timer = nullptr;
		NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
		tracker = nullptr;
		NSingleton::UnRegisterSingleton( IInterfaceState::tidTypeID );
		interfaceState = nullptr;
		NSingleton::UnRegisterSingleton( IUIInitialization::tidTypeID );
		NGlobal::RemoveVar( "ui_consts" );
		NGlobal::RemoveVar( "game_root" );
		NGlobal::RemoveVar( "game_mode_editor" );
		records.clear();
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		NVFS::SetMainFileCreator( nullptr );
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	void SaveNotifications( CMemoryStream *stream )
	{
		// Preserve the real UI controls as external objects while exercising event/postfix serialization.
		external.resize( 3 );
		for ( int i = 0; i < parent->GetNumChildren(); ++i )
		{
			external.emplace_back( 100 + i, parent->GetChild( i ) );
		}
		CPtr<IBinSaver> saver = CreateBinSaver( stream, SAVER_MODE_WRITE, external );
		saver->AddPolymorphicBase( 1, notifications.GetPtr() );
	}

	SNotificationFields ReadNotifications()
	{
		CMemoryStream stream;
		SaveNotifications( &stream );
		stream.Seek( 0 );
		SNotificationFields result;
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, &result );
		return result;
	}

	void StartCooperativeObjectives()
	{
		auto *root = Record<NDb::SGameRoot>( "GameRoot" );
		ASSERT_NE( root, nullptr );
		NGlobal::SetVar( "game_root", std::to_string( records.size() ) + ".xdb" );
		auto *constants = Record<NDb::SGameConsts>( "GameConsts" );
		ASSERT_NE( constants, nullptr );
		constants->pAI = Record<NDb::SAIGameConsts>( "AIGameConsts" );
		constants->pMultiplayer = Record<NDb::SMultiplayerConsts>( "MultiplayerConsts" );
		root->pConsts = constants;
		objectiveTexture = Record<NDb::STexture>( "Texture" );
		ASSERT_NE( objectiveTexture, nullptr );
		root->textures.resize( 2 );
		root->textures[0].szTextID = "TX_MINIMAP_OBJECTIVE_NORMAL";
		root->textures[1].szTextID = "TX_MINIMAP_OBJECTIVE_SELECTED";
		for ( auto &entry : root->textures )
		{
			entry.pTexture = objectiveTexture;
		}
		map->players.resize( 3 );
		map->players[1].nDiplomacySide = 1;
		map->players.back().nDiplomacySide = 2;
		tracker->SetGameType( IAIScenarioTracker::EGT_MULTI_COOP );
		tracker->MissionStart( map, 0 );
		// Real flat terrain supplies GetZ for authored positions without rendering a scene.
		aiMap = new CAIMap( 16, 16, 32, 2, 16 );
		SetAIMap( aiMap );
	}

	std::vector<IMiniMap::SMarker> ReadMarkers()
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->AddPolymorphicBase( 1, minimap );
		}
		stream.Seek( 0 );
		std::vector<IMiniMap::SMarker> markers;
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 19, &markers ); } };
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, &fields );
		return markers;
	}

	void SetMapObject( CMapObj *object, CVec3 position, bool visible, float hp )
	{
		// Restore the real scene-side object without CreateSceneObject or a graphics device.
		SFields base{ [&]( IBinSaver &saver )
		              {
										saver.Add( 3, &position );
										saver.Add( 6, &hp );
										saver.Add( 12, &visible );
									} };
		Hydrate( object, { [&]( IBinSaver &saver ) { saver.Add( 1, &base ); } } );
	}

	uint32_t TrackerChecksum()
	{
		uint32_t checksum = 1;
		{
			CPtr<IBinSaver> saver = CreateCheckSumSaver( &checksum, nullptr, 0 );
			saver->AddPolymorphicBase( 1, tracker );
		}
		return checksum;
	}

	std::vector<CObj<SAIFeedbackUpdate>> RunLua( const char *source )
	{
		if ( !vm )
		{
			static const SRegFunction functions[] = {
				{ "KeepObjectiveNotified", CScripts::KeepObjectiveNotified },
				{ "UpdateObjectiveNotification", CScripts::UpdateObjectiveNotification },
				{ nullptr, nullptr } };
			vm = CreateScriptWrapper();
			vm->Init();
			vm->AddRegFunctions( functions );
			CDBPtr<NDb::SMapInfo> mapPointer = map;
			CMemoryStream stream;
			{
				SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 18, &mapPointer ); } };
				CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
				saver->Add( 1, &fields );
			}
			stream.Seek( 0 );
			CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
			loader->Add( 1, &scripts );
		}
		EXPECT_EQ( vm->RunScript( source ), 1 );
		vm->Segment();
		// Restore the queue as a mission load does, including its client transformer registry.
		// This also checks that postfix payloads survive while their feedback is still pending.
		CMemoryStream queuedUpdates;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &queuedUpdates, SAVER_MODE_WRITE );
			saver->Add( 1, &updater );
		}
		queuedUpdates.Seek( 0 );
		{
			CPtr<IBinSaver> loader = CreateBinSaver( &queuedUpdates, SAVER_MODE_READ );
			loader->Add( 1, &updater );
		}
		logic->PrepareUpdates();
		std::vector<CObj<SAIFeedbackUpdate>> result;
		while ( CPtr<CObjectBase> update = logic->GetUpdate() )
		{
			auto *feedback = dynamic_cast<SAIFeedbackUpdate *>( update.GetPtr() );
			EXPECT_NE( feedback, nullptr );
			if ( feedback )
			{
				result.emplace_back( feedback );
			}
		}
		return result;
	}
};

TEST_F( ObjectiveNotifications, RemembersPostfixAndUpdatesOneExistingControl )
{
	notifications->UpdateObjectiveNotification( 0, " - 00:30" );
	EXPECT_TRUE( ReadNotifications().events.empty() );
	notifications->KeepObjectiveNotified( 0, true );
	notifications->KeepObjectiveNotified( 0, true );
	auto state = ReadNotifications();
	ASSERT_EQ( state.events.size(), 1 );
	auto event = state.events.front();
	EXPECT_TRUE( event->bPersistentObjective );
	EXPECT_EQ( event->pDBEvent, objectiveEvent );
	EXPECT_EQ( event->wszText, L"Objective: Hold the \u017d bridge - 00:30" );
	IWindow *window = event->pItemWnd;
	ASSERT_NE( window, nullptr );
	ITextView *text = GetChildChecked<ITextView>( window, "Desc", true );
	ASSERT_NE( text, nullptr );
	EXPECT_NE( text->GetText().find( event->wszText ), std::wstring::npos );
	notifications->UpdateObjectiveNotification( 0, " - \xc5\xbd 00:29" );
	state = ReadNotifications();
	ASSERT_EQ( state.events.size(), 1 );
	EXPECT_EQ( state.events.front()->pItemWnd, window );
	EXPECT_EQ( state.events.front()->wszText, L"Objective: Hold the \u017d bridge - \u017d 00:29" );
	EXPECT_NE( text->GetText().find( state.events.front()->wszText ), std::wstring::npos );
	notifications->UpdateObjectiveNotification( 0, "" );
	EXPECT_EQ( ReadNotifications().events.front()->wszText, L"Objective: Hold the \u017d bridge" );
	notifications->KeepObjectiveNotified( 0, false );
	EXPECT_EQ( parent->GetNumChildren(), 0 );
}

TEST_F( ObjectiveNotifications, PinnedEntriesSurviveExpiryClicksAndOtherNotificationCleanup )
{
	notifications->KeepObjectiveNotified( 0, true );
	notifications->KeepObjectiveNotified( 1, true );
	IVisualNotifications::SEventParams transient;
	transient.eEventType = NDb::NEVT_OBJECTIVE_RECEIVED;
	transient.nID = 0;
	notifications->OnEvent( transient );
	notifications->Step( 1000, true );
	notifications->Step( 1000, true );
	auto state = ReadNotifications();
	ASSERT_EQ( state.events.size(), 2 );
	const auto name = state.events.front()->szName;
	notifications->OnBtn( name, true );
	notifications->OnBtn( name, false );
	notifications->OnRemoveEvent( NDb::NEVT_OBJECTIVE_RECEIVED, 0 );
	notifications->OnRemoveEvent( static_cast<NDb::ENotificationEventType>( -1 ), -1 );
	EXPECT_EQ( ReadNotifications().events.size(), 2 );
	notifications->KeepObjectiveNotified( 0, false );
	state = ReadNotifications();
	ASSERT_EQ( state.events.size(), 1 );
	EXPECT_EQ( state.events.front()->nID, 1 );
}

TEST_F( ObjectiveNotifications, SaveLoadRetainsPinsAndHiddenPostfixes )
{
	notifications->UpdateObjectiveNotification( 0, " - 10" );
	notifications->UpdateObjectiveNotification( 1, " - 20" );
	notifications->KeepObjectiveNotified( 0, true );
	CMemoryStream stream;
	SaveNotifications( &stream );
	notifications = MakeObject<IVisualNotifications>( 0x11135C01 );
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, notifications.GetPtr() );
	}
	EXPECT_TRUE( ReadNotifications().events.front()->bPersistentObjective );
	notifications->KeepObjectiveNotified( 1, true );
	auto state = ReadNotifications();
	ASSERT_EQ( state.events.size(), 2 );
	EXPECT_EQ( state.events.back()->wszText, L"Objective: Hold the \u017d bridge - 20" );
	notifications->KeepObjectiveNotified( 1, false );
	notifications->KeepObjectiveNotified( 1, true );
	EXPECT_EQ( ReadNotifications().events.back()->wszText, L"Objective: Hold the \u017d bridge - 20" );
}

TEST_F( ObjectiveNotifications, PinnedEntriesSurviveNotificationCapacityRemoval )
{
	notifications->KeepObjectiveNotified( 0, true );
	notifications->KeepObjectiveNotified( 1, true );
	IVisualNotifications::SEventParams transient;
	transient.eEventType = NDb::NEVT_OBJECTIVE_RECEIVED;
	transient.nID = 0;
	notifications->OnEvent( transient );
	// No time passes: this exercises queue capacity independently of expiration.
	notifications->Step( 0, true );
	auto state = ReadNotifications();
	ASSERT_EQ( state.events.size(), 2 );
	for ( const auto &event : state.events )
	{
		EXPECT_TRUE( event->bPersistentObjective );
	}
	notifications->KeepObjectiveNotified( 2, true );
	notifications->Step( 0, true );
	EXPECT_EQ( ReadNotifications().events.size(), 3 );
}

TEST_F( ObjectiveNotifications, OldSavesHaveNoPersistentEntriesOrRememberedPostfixes )
{
	CObj<CVisualNotifications::SEvent> event = new CVisualNotifications::SEvent;
	event->bPersistentObjective = true;
	// A legacy event has fields 2-9; the absent field 10 must restore the old behavior.
	CDBPtr<NDb::SNotificationEvent> dbEvent = objectiveEvent;
	Hydrate( event, { [&]( IBinSaver &saver ) { saver.Add( 3, &dbEvent ); } } );
	EXPECT_FALSE( event->bPersistentObjective );
	notifications->UpdateObjectiveNotification( 1, "old text" );
	ASSERT_FALSE( ReadNotifications().postfixes.empty() );
	// Loading an older notification manager into an existing object must also clear its new map.
	Hydrate( notifications, { []( IBinSaver & ) {} } );
	EXPECT_TRUE( ReadNotifications().postfixes.empty() );
}

TEST_F( ObjectiveNotifications, PrefixUsesTheExistingLocalizationCatalogue )
{
	TextFile( "prefix.txt", u"Zadatak: " );
	uiConsts->tags.resize( 1 );
	uiConsts->tags[0].szName = "OBJECTIVE_NOTIFICATION_PREFIX";
	uiConsts->tags[0].szTextFileRef = "prefix.txt";
	notifications->KeepObjectiveNotified( 0, true );
	EXPECT_EQ( ReadNotifications().events.front()->wszText, L"Zadatak: Hold the \u017d bridge" );
}

TEST_F( ObjectiveNotifications, LuaQueuesOrderedUpdatesAndSerializesUtf8Payload )
{
	auto updates = RunLua( "UpdateObjectiveNotification(0, ' - \xc5\xbd 00:30');"
	                       "KeepObjectiveNotified(0, 1); KeepObjectiveNotified(0, 0);" );
	ASSERT_EQ( updates.size(), 3 );
	EXPECT_EQ( updates[0]->info.feedBackType, EFB_UPDATE_OBJECTIVE_NOTIFICATION );
	EXPECT_EQ( updates[0]->info.nParam, 0 );
	EXPECT_EQ( updates[1]->info.feedBackType, EFB_KEEP_OBJECTIVE_NOTIFIED );
	EXPECT_EQ( updates[1]->info.nParam, 1 );
	EXPECT_EQ( updates[2]->info.nParam, 0 );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &updates[0] );
	}
	stream.Seek( 0 );
	CObj<SAIFeedbackUpdate> restored;
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->Add( 1, &restored );
	}
	ASSERT_NE( restored, nullptr );
	const auto *postfix = dynamic_cast<SFeedBackObjectiveNotification *>( restored->info.pParam.GetPtr() );
	ASSERT_NE( postfix, nullptr );
	EXPECT_EQ( postfix->szPostfix, " - \xc5\xbd 00:30" );
}

TEST_F( ObjectiveNotifications, InvalidLuaArgumentsAndMissingObjectivesProduceNoUpdates )
{
	map->objectives[2] = nullptr;
	EXPECT_TRUE( RunLua( "KeepObjectiveNotified(-1, 1); KeepObjectiveNotified(3, 1);"
	                     "KeepObjectiveNotified(0.5, 1); KeepObjectiveNotified(2, 1);"
	                     "KeepObjectiveNotified(0, 2); KeepObjectiveNotified(0, 0.5);"
	                     "UpdateObjectiveNotification(255, 'x'); UpdateObjectiveNotification(0, {});"
	                     "UpdateObjectiveNotification(0/0, 'x'); KeepObjectiveNotified(1/0, 1);" )
	                 .empty() );
	notifications->UpdateObjectiveNotification( 3, "ignored" );
	notifications->KeepObjectiveNotified( 3, true );
	EXPECT_TRUE( ReadNotifications().events.empty() );
}

TEST_F( ObjectiveNotifications, CoopTracksKnownObjectivesAndResetsWithoutChangingClassic )
{
	StartCooperativeObjectives();
	ASSERT_EQ( tracker->GetObjectiveCount(), 3 );
	EXPECT_EQ( tracker->GetKnownObjectiveCount(), 0 );
	EXPECT_EQ( tracker->GetObjectiveState( 0 ), EMOS_WAITING );
	tracker->SetObjectiveState( 2, EMOS_RECEIVED );
	tracker->SetObjectiveState( 0, EMOS_RECEIVED );
	tracker->SetObjectiveState( 2, EMOS_RECEIVED );
	ASSERT_EQ( tracker->GetKnownObjectiveCount(), 2 );
	EXPECT_EQ( tracker->GetKnownObjectiveID( 0 ), 2 );
	EXPECT_EQ( tracker->GetKnownObjectiveID( 1 ), 0 );
	tracker->SetObjectiveState( 2, EMOS_COMPLETED );
	tracker->SetObjectiveState( 0, EMOS_WAITING );
	EXPECT_EQ( tracker->GetKnownObjectiveCount(), 1 );
	EXPECT_EQ( tracker->GetObjectiveState( 2 ), EMOS_COMPLETED );
	tracker->SetObjectiveState( 2, EMOS_FAILED );
	EXPECT_EQ( tracker->GetObjectiveState( 2 ), EMOS_FAILED );
	EXPECT_EQ( tracker->GetScriptOutcome(), -1 );
	// Bad script IDs, missing authored records, and invalid states cannot create markers.
	map->objectives[1] = nullptr;
	tracker->SetObjectiveState( -1, EMOS_RECEIVED );
	tracker->SetObjectiveState( 3, EMOS_RECEIVED );
	tracker->SetObjectiveState( 1, EMOS_RECEIVED );
	tracker->SetObjectiveState( 0, static_cast<EMissionObjectiveState>( 255 ) );
	EXPECT_EQ( tracker->GetKnownObjectiveCount(), 1 );
	EXPECT_EQ( tracker->GetKnownObjectiveID( -1 ), -1 );
	EXPECT_EQ( tracker->GetKnownObjectiveID( 1 ), -1 );
	EXPECT_EQ( tracker->GetObjectiveState( -1 ), EMOS_MIN );
	EXPECT_EQ( tracker->GetObjectiveState( 0 ), EMOS_WAITING );
	tracker->SetObjectiveObjects( 2, {} );
	ASSERT_TRUE( tracker->IsDynamicObjective( 2 ) );
	tracker->MissionStart( map, 0 );
	EXPECT_EQ( tracker->GetKnownObjectiveCount(), 0 );
	EXPECT_EQ( tracker->GetObjectiveState( 2 ), EMOS_WAITING );
	EXPECT_FALSE( tracker->IsDynamicObjective( 2 ) );
	tracker->SetGameType( IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL );
	tracker->MissionStart( map, 0 );
	tracker->SetObjectiveState( 0, EMOS_RECEIVED );
	tracker->SetObjectiveObjects( 0, {} );
	EXPECT_EQ( tracker->GetObjectiveCount(), 0 );
	EXPECT_EQ( tracker->GetKnownObjectiveCount(), 0 );
	EXPECT_FALSE( tracker->IsDynamicObjective( 0 ) );
	std::vector<CVec3> places( 1, CVec3( 1, 2, 3 ) );
	EXPECT_FALSE( tracker->GetObjectivePlaces( 0, &places ) );
	EXPECT_TRUE( places.empty() );
}

TEST_F( ObjectiveNotifications, CoopStaticLocationsReachMinimapAndMapPointerRequests )
{
	StartCooperativeObjectives();
	objective->mapPositions = { CVec2( 128, 160 ), CVec2( 224, 256 ) };
	CObj<IScenarioTracker> single = dynamic_cast<IScenarioTracker *>( NObjectFactory::MakeObject( 0x11165340 ) );
	ASSERT_NE( single, nullptr );
	CDBPtr<NDb::SMapInfo> mapPointer = map;
	Hydrate( single, { [&]( IBinSaver &saver ) { saver.Add( 9, &mapPointer ); } } );
	std::vector<CVec3> expected;
	std::vector<CVec3> places;
	ASSERT_TRUE( single->GetObjectivePlaces( 0, &expected ) );
	ASSERT_TRUE( tracker->GetObjectivePlaces( 0, &places ) );
	ASSERT_EQ( places.size(), 2 );
	EXPECT_EQ( places, expected );
	EXPECT_EQ( places[0], CVec3( 128, 160, Vis2AI( 5.0f ) ) );
	EXPECT_EQ( places[1], CVec3( 224, 256, Vis2AI( 5.0f ) ) );
	NGlobal::SetVar( "game_mode_editor", 0 );
	tracker->SetObjectiveState( 0, EMOS_RECEIVED );
	ASSERT_TRUE( notifications->Notify( EVNT_RECEIVE_OBJECTIVE, 0, VNULL2 ) );
	const auto state = ReadNotifications();
	const auto pointers = state.pointers.find( 0 );
	ASSERT_NE( pointers, state.pointers.end() );
	ASSERT_EQ( pointers->second.size(), 2 );
	EXPECT_EQ( pointers->second[0].position, places[0] );
	EXPECT_EQ( pointers->second[1].position, places[1] );
	// No pointer model is loaded in this fixture; creation requests still carry the correct locations.
	EXPECT_FALSE( pointers->second[0].bPlaced );
	auto markers = ReadMarkers();
	ASSERT_EQ( markers.size(), 2 );
	EXPECT_EQ( markers[0].vPos, objective->mapPositions[0] );
	EXPECT_EQ( markers[1].vPos, objective->mapPositions[1] );
	EXPECT_EQ( markers[0].pTexture, objectiveTexture );
	tracker->SetObjectiveState( 0, EMOS_COMPLETED );
	ASSERT_TRUE( notifications->Notify( EVNT_COMPLETE_OBJECTIVE, 0, VNULL2 ) );
	EXPECT_TRUE( ReadNotifications().pointers.empty() );
	EXPECT_TRUE( ReadMarkers().empty() );
	// Like SP, opening the objective list may show completed locations; ordinary play hides them.
	ASSERT_TRUE( notifications->Notify( EVNT_SHOW_OBJECTIVES, 0, VNULL2 ) );
	EXPECT_EQ( ReadMarkers().size(), 2 );
	ASSERT_TRUE( notifications->Notify( EVNT_HIDE_OBJECTIVES, 0, VNULL2 ) );
	EXPECT_TRUE( ReadMarkers().empty() );
}

TEST_F( ObjectiveNotifications, CoopDynamicLocationsFollowOnlyLivingVisibleObjects )
{
	StartCooperativeObjectives();
	objective->mapPositions = { CVec2( 64, 96 ) };
	CObj<CMapObj> moving = dynamic_cast<CMapObj *>( NObjectFactory::MakeObject( 0x100A7484 ) );
	CObj<CMapObj> hidden = dynamic_cast<CMapObj *>( NObjectFactory::MakeObject( 0x100A7484 ) );
	CObj<CMapObj> dead = dynamic_cast<CMapObj *>( NObjectFactory::MakeObject( 0x100A7484 ) );
	CObj<CMapObj> removed = dynamic_cast<CMapObj *>( NObjectFactory::MakeObject( 0x100A7484 ) );
	ASSERT_NE( moving, nullptr );
	ASSERT_NE( hidden, nullptr );
	ASSERT_NE( dead, nullptr );
	ASSERT_NE( removed, nullptr );
	SetMapObject( moving, CVec3( 200, 220, 17 ), true, 1.0f );
	SetMapObject( hidden, CVec3( 300, 320, 19 ), false, 1.0f );
	SetMapObject( dead, CVec3( 400, 420, 21 ), true, 0.0f );
	SetMapObject( removed, CVec3( 450, 470, 23 ), true, 1.0f );
	tracker->SetObjectiveObjects( 0, { moving, hidden, dead, removed, nullptr } );
	removed = nullptr;
	tracker->SetObjectiveState( 0, EMOS_RECEIVED );
	std::vector<CVec3> places;
	ASSERT_TRUE( tracker->GetObjectivePlaces( 0, &places ) );
	ASSERT_EQ( places.size(), 1 );
	EXPECT_EQ( places.front(), moving->GetCenter() );
	SetMapObject( moving, CVec3( 232, 252, 18 ), true, 1.0f );
	ASSERT_TRUE( tracker->GetObjectivePlaces( 0, &places ) );
	EXPECT_EQ( places.front(), moving->GetCenter() );
	NGlobal::SetVar( "game_mode_editor", 0 );
	notifications->Step( 0, true );
	auto markers = ReadMarkers();
	ASSERT_EQ( markers.size(), 1 );
	EXPECT_EQ( markers.front().vPos, CVec2( 232, 252 ) );
	SetMapObject( moving, moving->GetCenter(), false, 1.0f );
	notifications->Step( 0, true );
	EXPECT_TRUE( ReadMarkers().empty() );
	tracker->SetObjectiveObjects( 0, {} );
	EXPECT_TRUE( tracker->IsDynamicObjective( 0 ) );
	EXPECT_TRUE( tracker->GetObjectivePlaces( 0, &places ) );
	EXPECT_TRUE( places.empty() );
	// Clearing a dynamic group must not revive the old authored coordinate.
	EXPECT_FALSE( tracker->GetObjectivePlaces( 99, &places ) );
	EXPECT_TRUE( places.empty() );
}

TEST_F( ObjectiveNotifications, CoopObjectivePresentationSurvivesSaveLoadAndStaysOutOfChecksums )
{
	StartCooperativeObjectives();
	const uint32_t baseline = TrackerChecksum();
	CObj<CMapObj> object = dynamic_cast<CMapObj *>( NObjectFactory::MakeObject( 0x100A7484 ) );
	ASSERT_NE( object, nullptr );
	SetMapObject( object, CVec3( 112, 144, 8 ), true, 1.0f );
	tracker->SetObjectiveState( 2, EMOS_COMPLETED );
	tracker->SetObjectiveState( 0, EMOS_RECEIVED );
	tracker->SetObjectiveObjects( 0, { object } );
	EXPECT_EQ( TrackerChecksum(), baseline );
	// Client objects remain externally owned as they are by the loaded world.
	std::vector<SBinSaverExternalObject> worldObjects{ { 1000, object } };
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, worldObjects );
		saver->AddPolymorphicBase( 1, tracker );
	}
	CObj<IScenarioTracker> restored = CreateScenarioTrackerMultiplayer();
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, worldObjects );
		loader->AddPolymorphicBase( 1, restored );
	}
	ASSERT_EQ( restored->GetKnownObjectiveCount(), 2 );
	EXPECT_EQ( restored->GetKnownObjectiveID( 0 ), 2 );
	EXPECT_EQ( restored->GetKnownObjectiveID( 1 ), 0 );
	EXPECT_EQ( restored->GetObjectiveState( 2 ), EMOS_COMPLETED );
	EXPECT_EQ( restored->GetObjectiveState( 0 ), EMOS_RECEIVED );
	EXPECT_TRUE( restored->IsDynamicObjective( 0 ) );
	std::vector<CVec3> places;
	ASSERT_TRUE( restored->GetObjectivePlaces( 0, &places ) );
	ASSERT_EQ( places.size(), 1 );
	EXPECT_EQ( places.front(), object->GetCenter() );
	CDBPtr<NDb::SMapInfo> mapPointer = map;
	auto mode = IAIScenarioTracker::EGT_MULTI_COOP;
	// An old COOP save loaded into a reused tracker has no objective chunks 29-31.
	Hydrate( restored,
	         { [&]( IBinSaver &saver )
	           {
							 saver.Add( 2, &mapPointer );
							 saver.Add( 19, &mode );
						 } } );
	EXPECT_EQ( restored->GetObjectiveCount(), 3 );
	EXPECT_EQ( restored->GetKnownObjectiveCount(), 0 );
	EXPECT_EQ( restored->GetObjectiveState( 0 ), EMOS_WAITING );
	EXPECT_EQ( restored->GetObjectiveState( 2 ), EMOS_WAITING );
	EXPECT_FALSE( restored->IsDynamicObjective( 0 ) );
}
} // namespace
