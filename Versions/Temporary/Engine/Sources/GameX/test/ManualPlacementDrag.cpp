#include "../stdafx.h"

#include "GameX/InterfaceMPGameRoom.h"
#include "GameX/InterfaceState.h"
#include "GameX/MPCoopRules.h"
#include "libdb/Db.h"
#include "SceneB2/Camera.h"
#include "SceneB2/Cursor.h"
#include "SceneB2/Scene.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "UI/DBUserInterface.h"
#include "UI/test/MakeDescription.h"
#include "UISpecificB2/DBUISpecificB2.h"

#include <gtest/gtest.h>

namespace
{
// Capture outgoing UI requests. Delivering a captured message back to the real
// room models the host's authoritative broadcast without starting the transport.
class CPlacementSink : public IMPToUIManager
{
	OBJECT_BASIC_METHODS( CPlacementSink );

public:
	CObj<SMPUIManualPlacementMessage> message;

	void AddUIMessage( SMPUIMessage *pMsg ) override
	{
		message = dynamic_cast<SMPUIManualPlacementMessage *>( pMsg );
	}

	void AddUIMessage( EMPUIMessageType ) override {}

	SMPUIMessage *GetUIMessage() override
	{
		return nullptr;
	}

	SMPUIMessage *PeekUIMessage() override
	{
		return nullptr;
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

template <class T>
const T *LoadControl( const std::string &szName )
{
	return NDb::Get<T>( CDBID( "UI/Game/Menu/MPGameRoom/Placement/" + szName + ".xdb" ) );
}

CVec2 Center( const IWindow *pWindow )
{
	const auto rect = pWindow->GetWindowRect();
	return CVec2( rect.left + rect.Width() * 0.5f, rect.top + rect.Height() * 0.5f );
}
} // namespace

class CManualPlacementTestAccess
{
public:
	static void Prepare( CInterfaceMPGameRoom *pRoom, IWindow *pPanel, bool bHost )
	{
		pRoom->bConnected = true;
		pRoom->bHost = bHost;
		pRoom->pMain = pPanel;
		auto pInfo = NUITest::MakeDescription<NDb::SMapInfo>();
		pInfo->nNumPatchesX = 4;
		pInfo->nNumPatchesY = 4;
		pInfo->players.resize( 3 );
		for ( int i = 0; i < 3; ++i )
		{
			const float fCoordinate = ( 0.2f + i * 0.3f ) * pInfo->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE;
			pInfo->players[i].vMPStartPos = CVec2( fCoordinate, fCoordinate );
		}
		auto pMap = NUITest::MakeDescription<NDb::SMultiplayerMap>();
		pMap->nPlayers = 3;
		pMap->pMap = pInfo;
		pRoom->gameDesc.pMPMap = pMap;
		pRoom->gameDesc.nPlayers = 2;
		pRoom->slots.resize( 2 );
		pRoom->slots[0].info.bPresent = true;
		pRoom->slots[1].info.bPresent = true;
		pRoom->slots[1].info.nTeam = 1;

		// Supply the lobby preview. The room itself creates the overlay and host-only button.
		auto pShared = NUITest::MakeDescription<NDb::SWindowMiniMapShared>();
		pShared->bRotable = true;
		pShared->vPoint00 = CVec2( 30, 30 );
		pShared->vPoint11 = CVec2( 200, 200 );
		auto pDesc = NUITest::MakeDescription<NDb::SWindowMiniMap>();
		pDesc->PostLoad( false );
		pDesc->szName = "Minimap";
		pDesc->pShared = pShared;
		pDesc->placement.size = CVec2( 230, 230 );
		AddWindowCopy( pPanel, pDesc.GetPtr() )->SetPlacement( 50, 35, 230, 230, EWPF_ALL );
		pRoom->InitPlacementControls();
	}

	static IWindow *Flag( CInterfaceMPGameRoom *pRoom, int nSlot )
	{
		return pRoom->placementMarkers[nSlot].pFlag;
	}
};

namespace
{
class ManualPlacementDrag : public testing::Test
{
protected:
	CObj<NVFS::ICombinerVFS> vfs;
	CObj<CPlacementSink> sink;
	CObj<CInterfaceMPGameRoom> room;
	CObj<IWindow> panel;

	void SetUp() override
	{
		// Load GameX and register the same focus, cursor and input services the real room uses.
		ASSERT_FALSE( NGameX::IsCoopGame( SB2GameSpecificData() ) );
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( 2048, 1536 );
		NSingleton::RegisterSingleton( CreateCursor(), ICursor::tidTypeID );
		NSingleton::RegisterSingleton( CreateCamera(), ICamera::tidTypeID );
		NSingleton::RegisterSingleton( CreateScene(), IScene::tidTypeID );
		NSingleton::RegisterSingleton( NObjectFactory::MakeObject( 0x300C8D44 ), IInterfaceState::tidTypeID );
		NSingleton::RegisterSingleton( NObjectFactory::MakeObject( 0x10075C05 ), IGameTimer::tidTypeID );
		sink = new CPlacementSink();
		NSingleton::RegisterSingleton( sink, IMPToUIManager::tidTypeID );
		vfs = NVFS::CreateCombinerVFS( 0 );
		vfs->SetVFSList(
				{ NVFS::CreateWinVFS( std::string( PLACEMENT_UI_GAME_DATA ) + "/" ), NVFS::CreateWinVFS( PLACEMENT_UI_PAK ) } );
		NVFS::SetMainVFS( vfs );
		ASSERT_TRUE( NDb::OpenDatabase( vfs, nullptr, NDb::DATABASE_MODE_GAME ) );
		room = dynamic_cast<CInterfaceMPGameRoom *>( NObjectFactory::MakeObject( 0x3219DAC0 ) );
		ASSERT_NE( room, nullptr );
		panel = AddWindowCopy( nullptr, LoadControl<NDb::SWindowSimple>( "TeamBlock" ) );
		ASSERT_NE( panel, nullptr );
		panel->SetPlacement( 120, 80, 378, 531, EWPF_ALL );
		room->AfterLoad();
		room->OnGetFocus( true );
	}

	void TearDown() override
	{
		room = nullptr;
		panel = nullptr;
		sink = nullptr;
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		for ( int id : std::initializer_list<int>{ IMPToUIManager::tidTypeID,
		                                           IGameTimer::tidTypeID,
		                                           IInterfaceState::tidTypeID,
		                                           IScene::tidTypeID,
		                                           ICamera::tidTypeID,
		                                           ICursor::tidTypeID,
		                                           IUIInitialization::tidTypeID } )
		{
			NSingleton::UnRegisterSingleton( id );
		}
	}

	void Mouse( const char *pszEvent, CVec2 pos )
	{
		VirtualToScreen( &pos );
		NInput::PostWinEvent( pszEvent, PackCoords( pos ), 0 );
		SGameMessage message;
		ASSERT_TRUE( NInput::GetEvent( &message ) );
		room->ProcessEvent( message );
	}

	void Apply( const std::vector<uint8_t> &order )
	{
		SMPUIManualPlacementMessage snapshot( order );
		ASSERT_TRUE( room->HandleMessage( &snapshot ) );
	}
};

TEST_F( ManualPlacementDrag, HostSwapsOccupiedAndEmptyStartsAndCanTurnThePreviewOff )
{
	CManualPlacementTestAccess::Prepare( room, panel, true );
	ASSERT_NE( panel->GetChild( "ManualPlacement", false ), nullptr );
	ASSERT_TRUE( room->Execute( "ManualPlacement", "ToggleManualPlacement" ) );
	ASSERT_NE( sink->message, nullptr );
	ASSERT_EQ( sink->message->manualPlacement.size(), 3u );
	auto order = sink->message->manualPlacement;
	Apply( order );
	auto *pSource = CManualPlacementTestAccess::Flag( room, 0 );
	sink->message = nullptr;
	const auto origin = Center( pSource );
	Mouse( "win_left_button_down", origin );
	Mouse( "win_left_button_up", origin );
	EXPECT_EQ( sink->message, nullptr );
	Mouse( "win_left_button_down", origin );
	Mouse( "win_mouse_move", CVec2( 1, 1 ) );
	Mouse( "win_left_button_up", CVec2( 1, 1 ) );
	EXPECT_EQ( sink->message, nullptr );
	EXPECT_EQ( Center( pSource ), origin );
	for ( int nTarget : { 1, 2 } )
	{
		sink->message = nullptr;
		auto *pTarget = CManualPlacementTestAccess::Flag( room, nTarget );
		const auto target = Center( pTarget );
		Mouse( "win_left_button_down", Center( pSource ) );
		Mouse( "win_mouse_move", target );
		EXPECT_NEAR( Center( pSource ).x, target.x, 1 );
		EXPECT_NEAR( Center( pSource ).y, target.y, 1 );
		Mouse( "win_left_button_up", target );
		ASSERT_NE( sink->message, nullptr );
		std::swap( order[0], order[nTarget] );
		EXPECT_EQ( sink->message->manualPlacement, order );
		Apply( order );
		EXPECT_NEAR( Center( pSource ).x, target.x, 1 );
		EXPECT_NEAR( Center( pSource ).y, target.y, 1 );
	}
	ASSERT_TRUE( room->Execute( "ManualPlacement", "ToggleManualPlacement" ) );
	ASSERT_NE( sink->message, nullptr );
	EXPECT_TRUE( sink->message->manualPlacement.empty() );
	Apply( {} );
	EXPECT_FALSE( pSource->IsVisible() );
}

TEST_F( ManualPlacementDrag, GuestReceivesSwapsAndCannotSubmitADragOrRoll )
{
	CManualPlacementTestAccess::Prepare( room, panel, false );
	EXPECT_EQ( panel->GetChild( "ManualPlacement", false ), nullptr );
	Apply( { 0, 1, 2 } );
	auto *pSource = CManualPlacementTestAccess::Flag( room, 0 );
	auto *pTarget = CManualPlacementTestAccess::Flag( room, 2 );
	const auto source = Center( pSource );
	const auto target = Center( pTarget );
	ASSERT_TRUE( pTarget->GetChild( "Empty", false )->IsVisible() );
	Mouse( "win_left_button_down", source );
	Mouse( "win_mouse_move", target );
	Mouse( "win_left_button_up", target );
	room->Execute( "ManualPlacement", "ToggleManualPlacement" );
	EXPECT_EQ( sink->message, nullptr );
	EXPECT_EQ( Center( pSource ), source );
	Apply( { 2, 1, 0 } );
	EXPECT_EQ( Center( pSource ), target );
	EXPECT_EQ( Center( pTarget ), source );
}
} // namespace
