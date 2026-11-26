#include "../stdafx.h"

#include "3Dmotor/DBScene.h"
#include "GameX/DBMPConsts.h"
#include "GameX/GetConsts.h"
#include "GameX/InterfaceMPGameRoom.h"
#include "GameX/InterfaceState.h"
#include "GameX/MPCoopRules.h"
#include "Input/Input.h"
#include "libdb/Db.h"
#include "SceneB2/Camera.h"
#include "SceneB2/Cursor.h"
#include "SceneB2/Scene.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "System/VFSOperations.h"
#include "System/WinFrame.h"
#include "System/WinVFS.h"
#include "UI/BackgroundVectorImage.h"
#include "UI/DBUserInterface.h"
#include "UI/test/MakeDescription.h"
#include "UI/UIVisitor.h"
#include "UI/Window.h"
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

// Capture the stock bitmap path without a graphics device. Vector skins expose their source file directly.
class CIconVisitor : public IUIVisitor
{
public:
	const NDb::STexture *pTexture = nullptr;

	void ClipSet( const CTRect<float> & ) override {}

	void ClipRestore() override {}

	void VisitZClearRect( const CRectLayout &, const CTPoint<float> &, const CTRect<float> &, float ) override {}

	void VisitUIRect( const NDb::STexture *pDesc, int, const CRectLayout & ) override
	{
		pTexture = pDesc;
	}

	void
	VisitUIRect( const NDb::STexture *, int, const CVec2 *, const NGfx::SPixel8888 *, const CTRect<float> & ) override
	{
	}

	void VisitUITextureRect( CPtrFuncBase<NGfx::CTexture> *, int, const CRectLayout & ) override {}

	void VisitUIText( IML *, const CTPoint<float> &, const CTRect<float> & ) override {}
};

void CheckCountryIcon( IWindow *pIcon, const std::string &szImage, const NDb::STexture *pStockTexture )
{
	ASSERT_TRUE( pIcon->IsVisible() );
	const auto *pVector =
			dynamic_cast<const CBackgroundVectorImage *>( dynamic_cast<CWindow *>( pIcon )->GetBackground() );
	if ( !szImage.empty() )
	{
		ASSERT_NE( pVector, nullptr );
		EXPECT_EQ( pVector->GetFileName(), szImage );
	}
	else
	{
		ASSERT_EQ( pVector, nullptr );
		ASSERT_NE( pStockTexture, nullptr );
		CIconVisitor visitor;
		pIcon->Visit( &visitor );
		EXPECT_EQ( visitor.pTexture, pStockTexture );
	}
}
} // namespace

class CManualPlacementTestAccess
{
public:
	static void StepRoom( CInterfaceMPGameRoom *pRoom )
	{
		pRoom->StepLocal( true );
	}

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

	static void AddStartButton( CInterfaceMPGameRoom *pRoom, IWindow *pPanel )
	{
		// A slot update reevaluates the real Begin Game button after refreshing the preview.
		pRoom->pButtonBeginGame = dynamic_cast<IButton *>(
				AddWindowCopy( pPanel, LoadControl<NDb::SWindowMSButton>( "Toggle_WindowMSButton" ) ) );
		pRoom->pButtonBeginGame->SetName( "ButtonBeginGame" );
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
		NGlobal::RemoveVar( "mp_consts" );
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

TEST_F( ManualPlacementDrag, DiceKeepsEqualScreenMarginsWhenTheAspectRatioChanges )
{
	CManualPlacementTestAccess::Prepare( room, panel, true );
	const IWindow *pButton = panel->GetChild( "ManualPlacement", false );
	ASSERT_NE( pButton, nullptr );
	for ( int nWidth : { 2048, 2560 } )
	{
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( nWidth, 1440 );
		CManualPlacementTestAccess::StepRoom( room );
		const auto origin = panel->GetWindowRect();
		// Straight inner edges of the stock MinimapBack frame.
		CVec2 corner( origin.left + 19, origin.top + 260 );
		VirtualToScreen( &corner );
		auto button = pButton->GetWindowRect();
		VirtualToScreen( &button );
		EXPECT_GT( button.left, corner.x );
		EXPECT_NEAR( button.left - corner.x, corner.y - button.bottom, 0.01f );
	}
}

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

TEST_F( ManualPlacementDrag, LosingMouseFocusRestoresFlagAndIgnoresTheLateRelease )
{
	CManualPlacementTestAccess::Prepare( room, panel, true );
	Apply( { 0, 1, 2 } );
	auto *pFlag = CManualPlacementTestAccess::Flag( room, 0 );
	const CVec2 origin = Center( pFlag );
	const CVec2 target = Center( CManualPlacementTestAccess::Flag( room, 1 ) );
	Mouse( "win_left_button_down", origin );
	Mouse( "win_mouse_move", target );
	ASSERT_NE( Center( pFlag ), origin );

	// Both native window backends send this before input resumes in the game window.
	NWinFrame::SWindowsMsg cancel;
	cancel.msg = NWinFrame::SWindowsMsg::MOUSE_CANCEL;
	std::string event;
	int first = 0, second = 0, count = 0;
	NInput::EControlType type = NInput::CT_UNKNOWN;
	ASSERT_TRUE( NInput::ConvertMessage( cancel, &event, &first, &second, &count, &type ) );
	ASSERT_EQ( type, NInput::CT_WINDOWS );
	ASSERT_EQ( count, 1 );
	NInput::PostWinEvent( event, first, second );
	SGameMessage message;
	ASSERT_TRUE( NInput::GetEvent( &message ) );
	room->ProcessEvent( message );
	EXPECT_EQ( Center( pFlag ), origin );

	Mouse( "win_mouse_move", target );
	EXPECT_EQ( Center( pFlag ), origin );
	Mouse( "win_left_button_up", target );
	EXPECT_EQ( sink->message, nullptr );
}

class ManualPlacementCountries : public ManualPlacementDrag, public testing::WithParamInterface<bool>
{
};

TEST_P( ManualPlacementCountries, CountryOnlyUpdatesRedrawTheFlagAndRandomKeepsItsRolledNationPrivate )
{
	const char *pszConsts = "Consts/Test/Test_MultiplayerConsts.xdb";
	NGlobal::SetVar( "mp_consts", pszConsts );
	const auto *pConsts = NDb::Get<NDb::SMultiplayerConsts>( CDBID( pszConsts ) );
	ASSERT_NE( pConsts, nullptr );
	ASSERT_GE( pConsts->sides.size(), 2u );
	CManualPlacementTestAccess::Prepare( room, panel, GetParam() );
	CManualPlacementTestAccess::AddStartButton( room, panel );
	Apply( { 2, 1, 0 } );
	auto *pFlag = CManualPlacementTestAccess::Flag( room, 0 );
	auto *pIcon = pFlag->GetChild( "Icon", false );
	ASSERT_NE( pIcon, nullptr );
	auto *pVacant = CManualPlacementTestAccess::Flag( room, 2 )->GetChild( "Icon", false );
	ASSERT_NE( pVacant, nullptr );
	EXPECT_FALSE( pVacant->IsVisible() );

	SMPUIUpdateSlotMessage update;
	update.nSlot = 0;
	update.info.bPresent = true;
	update.info.bRandomCountry = false;
	for ( int nCountry : { 0, 1 } )
	{
		update.info.nCountry = nCountry;
		ASSERT_TRUE( room->HandleMessage( &update ) );
		CheckCountryIcon( pIcon, NGameX::GetSideFlagImage( pConsts, nCountry, 0 ), pConsts->sides[nCountry].pListItemIcon );
	}
	update.info.bRandomCountry = true;
	ASSERT_TRUE( room->HandleMessage( &update ) );
	CheckCountryIcon( pIcon, NGameX::GetRandomCountryImage( pConsts ), pConsts->pRandomCountryIcon );
	update.info.bPresent = false;
	ASSERT_TRUE( room->HandleMessage( &update ) );
	EXPECT_FALSE( pIcon->IsVisible() );
	EXPECT_TRUE( pFlag->GetChild( "Empty", false )->IsVisible() );
}

INSTANTIATE_TEST_SUITE_P( HostAndGuest, ManualPlacementCountries, testing::Bool() );
} // namespace
