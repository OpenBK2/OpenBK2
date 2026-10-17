#include "../stdafx.h"

#include "3Dmotor/DBScene.h"
#include "libdb/Db.h"
#include "System/Text.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "UI/DBUserInterface.h"
#include "UI/UI.h"

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
		if ( file.size() >= 4 && file.substr( file.size() - 4 ) == ".xdb" )
		{
			EXPECT_NE( NDb::GetObject( CDBID( file ) ), nullptr ) << file;
		}
	}

	const auto *pCaption = Load<NDb::SWindowTextView>( "ToggleLabel_WindowTextView" );
	ASSERT_NE( pCaption, nullptr );
	EXPECT_EQ( NText::GetText( pCaption->szTextFileRef ), L"Transfer >\n" );
	// Captions follow the game's regular text palette; player colors are applied at runtime.
	EXPECT_EQ( pCaption->pTextFormat.GetPtr(),
	           NDb::Get<NDb::STextFormat>( CDBID( "UI/Game/Common/Normal_center_TextFormat.xdb" ) ) );
	const auto *pPlayerName = Load<NDb::SWindowTextView>( "TransferName_WindowTextView" );
	ASSERT_NE( pPlayerName, nullptr );
	EXPECT_EQ( pPlayerName->pTextFormat.GetPtr(),
	           NDb::Get<NDb::STextFormat>( CDBID( "UI/Game/Common/Normal_TextFormat.xdb" ) ) );
}

TEST_F( UnitTransferUI, ButtonsUseProgrammedReactionsAndScrollableRowsFitTheirViewport )
{
	const std::pair<const char *, const char *> buttons[] = { { "Toggle", "MissionTransferToggle" },
	                                                          { "TransferClose", "MissionTransferClose" },
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
	EXPECT_LE( pRow->placement.size.Get().x,
	           pList->placement.size.Get().x - pShared->pBorder->placement.upperMargin.Get().x );
	EXPECT_GT( pRow->placement.size.Get().y * 6, pList->placement.size.Get().y );

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
		{ "ConfirmDisabled", "UI/Game/Mission/NewActions/TemplateDisabled_BackgroundSimpleTexture.xdb" },
		{ "CloseNormal", "UI/Game/Mission/Objectives/ScrollDownNormal_BackgroundSimpleTexture.xdb" },
		{ "ClosePushed", "UI/Game/Mission/Objectives/ScrollDownPressed_BackgroundSimpleTexture.xdb" },
		{ "CloseDisabled", "UI/Game/Mission/Objectives/ScrollDownDisabled_BackgroundSimpleTexture.xdb" } };
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

	const std::pair<const char *, const char *> buttons[] = {
		{ "Toggle", "Toggle" }, { "TransferClose", "Close" }, { "TransferConfirm", "Confirm" } };
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
} // namespace
