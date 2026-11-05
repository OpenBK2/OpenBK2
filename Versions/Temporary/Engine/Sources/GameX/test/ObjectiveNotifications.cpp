#include "AILogic/stdafx.h"

#include "AILogic/B2AI.h"
#include "AILogic/NewUpdater.h"
#include "AILogic/Scripts.h"
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

struct SNotificationFields
{
	std::list<CObj<CVisualNotifications::SEvent>> events;
	det_map<int, std::wstring> postfixes;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 20, &events );
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
	NDb::SMapInfo *map = nullptr;
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
		auto *objective = Record<NDb::SMissionObjective>( "MissionObjective" );
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
} // namespace
