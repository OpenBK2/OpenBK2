#include "../stdafx.h"

#include "3Dmotor/DBScene.h"
#include "libdb/Db.h"
#include "System/Text.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "UI/DBUserInterface.h"
#include "UI/UI.h"

#include <filesystem>

#include <gtest/gtest.h>

namespace
{
class UnitTransferUI : public testing::Test
{
protected:
	// Database/main VFS pointers do not own the combiner; keep its CObj alive for the suite.
	static CObj<NVFS::ICombinerVFS> vfs;

	static void SetUpTestSuite()
	{
		// Sparse CI checkouts must include both the example archive and its stock UI dependencies.
		// Fail here with the missing path instead of reporting an empty archive and null DB records.
		ASSERT_TRUE( std::filesystem::is_regular_file( TRANSFER_UI_PAK ) )
				<< "Missing transfer UI archive; check the CI sparse checkout: " << TRANSFER_UI_PAK;
		const auto nativeButton =
				std::filesystem::path( TRANSFER_UI_GAME_DATA ) / "UI/Game/Menu/Common/Button_BackgroundTiledTexture.xdb";
		ASSERT_TRUE( std::filesystem::is_regular_file( nativeButton ) )
				<< "Missing stock transfer UI fixtures; check the CI sparse checkout: " << nativeButton.string();
		// Force the UI module to load and register its generated resource classes on Windows.
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		// Mount the actual ZIP through the game's VFS, with retail dependencies below it.
		// No generated index or extracted loose copy can hide archive/DB loading errors.
		vfs = NVFS::CreateCombinerVFS( 0 );
		std::vector<CObj<NVFS::IVFS>> sources;
		sources.push_back( NVFS::CreateWinVFS( std::string( TRANSFER_UI_GAME_DATA ) + "/" ) );
		sources.push_back( NVFS::CreateWinVFS( TRANSFER_UI_PAK ) );
		vfs->SetVFSList( sources );
		NVFS::SetMainVFS( vfs );
		ASSERT_TRUE( NDb::OpenDatabase( vfs, 0, NDb::DATABASE_MODE_GAME ) );
	}

	static void TearDownTestSuite()
	{
		NDb::CloseDatabase();
		NVFS::SetMainVFS( 0 );
		vfs = 0;
	}

	template <class T>
	const T *Load( const std::string &name )
	{
		return NDb::Get<T>( CDBID( "UI/Game/Mission/UnitTransfer/" + name + ".xdb" ) );
	}
};

CObj<NVFS::ICombinerVFS> UnitTransferUI::vfs;

TEST_F( UnitTransferUI, ArchiveLoadsDescriptorsAndUnicodeLabelsThroughGameDatabase )
{
	ASSERT_NE( NVFS::GetMainVFS(), nullptr );
	CObj<NVFS::IVFS> archive = NVFS::CreateWinVFS( TRANSFER_UI_PAK );
	std::vector<std::string> files;
	archive->GetAllFileNames( &files, "" );
	ASSERT_FALSE( files.empty() );
	for ( const std::string &file : files )
	{
		EXPECT_EQ( file.find( "TransferClose" ), std::string::npos );
		if ( file.size() >= 4 && file.substr( file.size() - 4 ) == ".xdb" )
		{
			EXPECT_NE( NDb::GetResource( CDBID( file ) ), nullptr ) << file;
		}
	}

	const auto *pCaption = Load<NDb::SWindowTextView>( "ToggleLabel_WindowTextView" );
	ASSERT_NE( pCaption, nullptr );
	EXPECT_EQ( NText::GetText( pCaption->szTextFileRef ), L"Transfer" );
	// Captions follow the game's regular text palette; player colors are applied at runtime.
	EXPECT_EQ( pCaption->pTextFormat.GetPtr(),
	           NDb::Get<NDb::STextFormat>( CDBID( "UI/Game/Common/Normal_center_TextFormat.xdb" ) ) );
	const auto *pPlayerName = Load<NDb::SWindowTextView>( "TransferName_WindowTextView" );
	ASSERT_NE( pPlayerName, nullptr );
	EXPECT_EQ( pPlayerName->pTextFormat.GetPtr(),
	           NDb::Get<NDb::STextFormat>( CDBID( "UI/Game/Common/Normal_TextFormat.xdb" ) ) );
	for ( const auto *pLabel : { pCaption, pPlayerName } )
	{
		// Measure one line at the actual HUD font scale, then center that height in its parent.
		EXPECT_TRUE( pLabel->bResizeOnTextSet );
		EXPECT_EQ( pLabel->placement.verAllign.Get(), NDb::ERA_CENTER );
		EXPECT_EQ( pLabel->placement.position.Get().y, 0 );
	}
}

TEST_F( UnitTransferUI, ButtonsUseProgrammedReactionsAndScrollableRowsFitTheirViewport )
{
	const std::pair<const char *, const char *> buttons[] = { { "Toggle", "MissionTransferToggle" },
	                                                          { "TransferConfirm", "MissionTransferPlayer" } };
	for ( const auto &button : buttons )
	{
		const auto *pButton = Load<NDb::SWindowMSButton>( std::string( button.first ) + "_WindowMSButton" );
		ASSERT_NE( pButton, nullptr );
		ASSERT_EQ( pButton->buttonStates.size(), 1 );
		const auto *pShared = dynamic_cast<const NDb::SWindowMSButtonShared *>( pButton->pShared.GetPtr() );
		ASSERT_NE( pShared, nullptr );
		EXPECT_EQ( pShared->visualStates.size(), pButton->buttonStates.size() );
		EXPECT_EQ( pShared->eTriggerMode, NDb::BCST_ON_RELEASE );
		ASSERT_EQ( pButton->buttonStates[0].commandsOnEnterState.commands.size(), 1 );
		const auto *pReaction = dynamic_cast<const NDb::SUISRunReaction *>(
				pButton->buttonStates[0].commandsOnEnterState.commands[0].GetPtr() );
		ASSERT_NE( pReaction, nullptr );
		EXPECT_EQ( pReaction->szReactionForward.Get(), button.second );
	}

	const auto *pList = Load<NDb::SWindowScrollableContainer>( "Players_WindowScrollableContainer" );
	const auto *pRow = Load<NDb::SWindowSimple>( "Row_WindowSimple" );
	ASSERT_NE( pList, nullptr );
	ASSERT_NE( pRow, nullptr );
	const auto *pShared = dynamic_cast<const NDb::SWindowScrollableContainerShared *>( pList->pShared.GetPtr() );
	ASSERT_NE( pShared, nullptr );
	ASSERT_NE( pShared->pBorder.GetPtr(), nullptr );
	ASSERT_NE( pShared->pScrollBar.GetPtr(), nullptr );
	const auto *pBarShared = dynamic_cast<const NDb::SWindowScrollBarShared *>( pShared->pScrollBar->pShared.GetPtr() );
	ASSERT_NE( pBarShared, nullptr );
	ASSERT_NE( pBarShared->pSlider.GetPtr(), nullptr );
	EXPECT_EQ( pShared->pScrollBar->placement.size.Get().x, 16 );
	ASSERT_NE( pBarShared->pButtonLower.GetPtr(), nullptr );
	ASSERT_NE( pBarShared->pButtonGreater.GetPtr(), nullptr );
	EXPECT_EQ( pBarShared->pButtonLower->placement.size.Get().x, 16 );
	EXPECT_EQ( pBarShared->pButtonGreater->placement.size.Get().x, 16 );
	EXPECT_EQ( pBarShared->pSlider->placement.size.Get().x, 16 );
	const auto *pSliderShared = dynamic_cast<const NDb::SWindowSliderShared *>( pBarShared->pSlider->pShared.GetPtr() );
	ASSERT_NE( pSliderShared, nullptr );
	// A background remains visible even when SetRange hides the thumb for a short player list.
	ASSERT_NE( pSliderShared->pBackground.GetPtr(), nullptr );
	EXPECT_EQ(
			pSliderShared->pBackground.GetPtr(),
			NDb::Get<NDb::SBackground>( CDBID( "UI/Game/Menu/EscapeMenu/EscMenu_Slider_BackgroundTiledTexture.xdb" ) ) );
	ASSERT_NE( pSliderShared->pBackground->pTexture.GetPtr(), nullptr );
	EXPECT_TRUE( NVFS::GetMainVFS()->DoesFileExist( pSliderShared->pBackground->pTexture->szDestName ) );
	EXPECT_LE( pRow->placement.size.Get().x,
	           pList->placement.size.Get().x - pShared->pBorder->placement.upperMargin.Get().x );
	EXPECT_GT( pRow->placement.size.Get().y * 6, pList->placement.size.Get().y );
	const auto *pPanel = Load<NDb::SWindowSimple>( "Panel_WindowSimple" );
	ASSERT_NE( pPanel, nullptr );
	EXPECT_EQ( pPanel->placement.size.Get().x, 208 );
	EXPECT_EQ( pPanel->placement.size.Get().x - pList->placement.position.Get().x - pList->placement.size.Get().x,
	           pList->placement.position.Get().x );

	const auto *pCheck = Load<NDb::SBackgroundSimpleScallingTexture>( "Check_Background" );
	ASSERT_NE( pCheck, nullptr );
	ASSERT_NE( pCheck->pTexture.GetPtr(), nullptr );
	EXPECT_TRUE( NVFS::GetMainVFS()->DoesFileExist( pCheck->pTexture->szDestName ) );
}

TEST_F( UnitTransferUI, ControlsReuseNativeTexturesAndStateColors )
{
	// These copies resize native backgrounds, but must never replace their art with flat fills or custom tints.
	const std::pair<const char *, const char *> backgrounds[] = {
		{ "ToggleNormal", "UI/Game/Menu/Common/Button_BackgroundTiledTexture.xdb" },
		{ "TogglePushed", "UI/Game/Menu/Common/Button_Pressed_BackgroundTiledTexture.xdb" },
		{ "ToggleDisabled", "UI/Game/Menu/Common/Button_Forbidden_BackgroundTiledTexture.xdb" },
		{ "ConfirmNormal", "UI/Game/Mission/NewActions/TemplateNormal_BackgroundSimpleTexture.xdb" },
		{ "ConfirmPushed", "UI/Game/Mission/NewActions/TemplatePressed_BackgroundSimpleTexture.xdb" },
		{ "ConfirmDisabled", "UI/Game/Mission/NewActions/TemplateDisabled_BackgroundSimpleTexture.xdb" } };
	for ( const auto &background : backgrounds )
	{
		SCOPED_TRACE( background.first );
		const auto *pBackground = Load<NDb::SBackground>( std::string( background.first ) + "_Background" );
		const auto *pNative = NDb::Get<NDb::SBackground>( CDBID( background.second ) );
		ASSERT_NE( pBackground, nullptr );
		ASSERT_NE( pNative, nullptr );
		ASSERT_NE( pBackground->pTexture.GetPtr(), nullptr );
		EXPECT_EQ( pBackground->pTexture.GetPtr(), pNative->pTexture.GetPtr() );
		EXPECT_EQ( pBackground->nColor, pNative->nColor );
		EXPECT_TRUE( NVFS::GetMainVFS()->DoesFileExist( pBackground->pTexture->szDestName ) );
	}

	const std::pair<const char *, const char *> buttons[] = { { "Toggle", "Toggle" }, { "TransferConfirm", "Confirm" } };
	for ( const auto &button : buttons )
	{
		const auto *pShared = Load<NDb::SWindowMSButtonShared>( std::string( button.first ) + "_WindowMSButtonShared" );
		ASSERT_NE( pShared, nullptr );
		ASSERT_EQ( pShared->visualStates.size(), 1 );
		const auto &state = pShared->visualStates[0];
		const std::string prefix = button.second;
		EXPECT_EQ( state.normal.pBackground.GetPtr(), Load<NDb::SBackground>( prefix + "Normal_Background" ) );
		EXPECT_EQ( state.mouseOver.pBackground.GetPtr(), state.normal.pBackground.GetPtr() );
		EXPECT_EQ( state.pushed.pBackground.GetPtr(), Load<NDb::SBackground>( prefix + "Pushed_Background" ) );
		EXPECT_EQ( state.disabled.pBackground.GetPtr(), Load<NDb::SBackground>( prefix + "Disabled_Background" ) );
	}

	const std::pair<const char *, const char *> panels[] = {
		{ "Panel", "UI/Game/Menu/EscapeMenu/EscMenu_Bgr_4_BackgroundTiledTexture.xdb" },
		{ "Row", "UI/Game/Menu/EscapeMenu/HeaderTable_BackgroundTiledTexture.xdb" } };
	for ( const auto &panel : panels )
	{
		const auto *pShared = Load<NDb::SWindowSimpleShared>( std::string( panel.first ) + "_WindowSimpleShared" );
		ASSERT_NE( pShared, nullptr );
		ASSERT_NE( pShared->pBackground.GetPtr(), nullptr );
		EXPECT_EQ( pShared->pBackground.GetPtr(), NDb::Get<NDb::SBackground>( CDBID( panel.second ) ) );
		ASSERT_NE( pShared->pBackground->pTexture.GetPtr(), nullptr );
		EXPECT_TRUE( NVFS::GetMainVFS()->DoesFileExist( pShared->pBackground->pTexture->szDestName ) );
	}

	const auto *pToggle = Load<NDb::SBackgroundTiledTexture>( "ToggleNormal_Background" );
	ASSERT_NE( pToggle, nullptr );
	EXPECT_EQ( pToggle->rL.ptSize.y, 34 );
	EXPECT_EQ( pToggle->rR.ptSize.y, 34 );
	EXPECT_EQ( pToggle->rF.ptSize.y, 34 );
	const auto *pCheck = Load<NDb::SBackground>( "Check_Background" );
	ASSERT_NE( pCheck, nullptr );
	EXPECT_EQ( pCheck->nColor, -1 );
}

TEST_F( UnitTransferUI, ToggleDirectionUsesNativeTransparentChildIcons )
{
	const auto *pToggle = Load<NDb::SWindowMSButton>( "Toggle_WindowMSButton" );
	ASSERT_NE( pToggle, nullptr );
	const auto *pToggleShared = dynamic_cast<const NDb::SWindowMSButtonShared *>( pToggle->pShared.GetPtr() );
	ASSERT_NE( pToggleShared, nullptr );
	for ( const std::string direction : { "Right", "Left" } )
	{
		const std::string name = "TransferArrow" + direction;
		const auto *pIcon = Load<NDb::SWindowSimple>( name + "_WindowSimple" );
		ASSERT_NE( pIcon, nullptr );
		EXPECT_EQ( pIcon->szName, name );
		EXPECT_EQ( pIcon->bVisible, direction == "Right" );
		bool bChildFound = false;
		for ( const auto &pChild : pToggleShared->children )
		{
			bChildFound = bChildFound || pChild.GetPtr() == pIcon;
		}
		EXPECT_TRUE( bChildFound );
		const auto *pShared = dynamic_cast<const NDb::SWindowSimpleShared *>( pIcon->pShared.GetPtr() );
		ASSERT_NE( pShared, nullptr );
		EXPECT_TRUE( pShared->flags.bTransparent );
		ASSERT_NE( pShared->pBackground.GetPtr(), nullptr );
		const std::string texture = "UI/Game/Mission/Panels/RightPanel/ToggleReinf" + direction + "Normal_Texture.xdb";
		EXPECT_EQ( pShared->pBackground->pTexture.GetPtr(), NDb::Get<NDb::STexture>( CDBID( texture ) ) );
		EXPECT_EQ( pShared->pBackground->nColor, -1 );
		EXPECT_LE( pIcon->placement.position.Get().x + pIcon->placement.size.Get().x, pToggle->placement.size.Get().x );
		EXPECT_EQ( pIcon->placement.position.Get().y * 2 + pIcon->placement.size.Get().y, pToggle->placement.size.Get().y );
	}
}
} // namespace
