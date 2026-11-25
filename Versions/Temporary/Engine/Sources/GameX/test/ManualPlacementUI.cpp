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
class ManualPlacementUI : public testing::Test
{
protected:
	CObj<NVFS::ICombinerVFS> vfs;

	void SetUp() override
	{
		// Mount the same combination as an installed game: stock data plus the optional archive.
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		vfs = NVFS::CreateCombinerVFS( 0 );
		vfs->SetVFSList(
				{ NVFS::CreateWinVFS( std::string( PLACEMENT_UI_GAME_DATA ) + "/" ), NVFS::CreateWinVFS( PLACEMENT_UI_PAK ) } );
		NVFS::SetMainVFS( vfs );
		ASSERT_TRUE( NDb::OpenDatabase( vfs, 0, NDb::DATABASE_MODE_GAME ) );
	}

	void TearDown() override
	{
		NDb::CloseDatabase();
		NVFS::SetMainVFS( 0 );
	}

	template <class T>
	const T *Load( const std::string &szName )
	{
		return NDb::Get<T>( CDBID( "UI/Game/Menu/MPGameRoom/Placement/" + szName + ".xdb" ) );
	}

	void CheckTexture( const NDb::SBackground *pBackground, const char *pszTexture )
	{
		ASSERT_NE( pBackground, nullptr );
		ASSERT_NE( pBackground->pTexture.GetPtr(), nullptr );
		EXPECT_EQ( pBackground->pTexture.GetPtr(), NDb::Get<NDb::STexture>( CDBID( pszTexture ) ) );
		EXPECT_TRUE( vfs->DoesFileExist( pBackground->pTexture->szDestName ) );
	}
};

TEST_F( ManualPlacementUI, DiceStatesSendTheSameReactionAndKeepTheSelectedTintOnHover )
{
	const auto *pToggle = Load<NDb::SWindowMSButton>( "Toggle_WindowMSButton" );
	ASSERT_NE( pToggle, nullptr );
	EXPECT_FALSE( pToggle->bAutoChangeState );
	EXPECT_FALSE( NText::GetText( pToggle->szTooltipFileRef ).empty() );
	const auto *pShared = dynamic_cast<const NDb::SWindowMSButtonShared *>( pToggle->pShared.GetPtr() );
	ASSERT_NE( pShared, nullptr );
	ASSERT_EQ( pShared->visualStates.size(), 2 );
	ASSERT_EQ( pToggle->buttonStates.size(), 2 );
	EXPECT_EQ( pShared->eTriggerMode, NDb::BCST_ON_RELEASE );
	for ( int i = 0; i < 2; ++i )
	{
		const auto &commands = pToggle->buttonStates[i].commandsOnEnterState.commands;
		ASSERT_EQ( commands.size(), 1 );
		const auto *pReaction = dynamic_cast<const NDb::SUISRunReaction *>( commands[0].GetPtr() );
		ASSERT_NE( pReaction, nullptr );
		EXPECT_EQ( pReaction->szReactionForward.Get(), "ToggleManualPlacement" );
		const auto &state = pShared->visualStates[i];
		const auto *pBackground = state.normal.pBackground.GetPtr();
		ASSERT_NE( pBackground, nullptr );
		CheckTexture( pBackground, "UI/Game/Menu/MPGameRoom/Countries/FlagRandom_Texture.xdb" );
		EXPECT_EQ( static_cast<uint32_t>( pBackground->nColor ), i == 0 ? 0xffffffffu : 0xff808080u );
		EXPECT_EQ( state.mouseOver.pBackground.GetPtr(), pBackground );
		EXPECT_EQ( state.pushed.pBackground.GetPtr(), pShared->visualStates[1].normal.pBackground.GetPtr() );
	}
}

TEST_F( ManualPlacementUI, FlagsCombineTheLoadingScreenTeamAndPlayerColourAndProvideANeutralStart )
{
	const auto *pFlag = Load<NDb::SWindowSimple>( "Flag_WindowSimple" );
	ASSERT_NE( pFlag, nullptr );
	const auto *pShared = dynamic_cast<const NDb::SWindowSimpleShared *>( pFlag->pShared.GetPtr() );
	ASSERT_NE( pShared, nullptr );
	EXPECT_TRUE( pShared->flags.bTransparent );
	const std::pair<const char *, const char *> layers[] = {
		{ "TeamBlock", nullptr },
		{ "Team1", "UI/Game/Menu/MPStatistics/StatsList/StatsItem/TeamColourGreen_Texture.xdb" },
		{ "Team2", "UI/Game/Menu/MPStatistics/StatsList/StatsItem/TeamColourRed_Texture.xdb" },
		{ "Empty", "Scene/TexAndMats/All/UI/Mission/Minimap/MP/MinimapFlagGrey_Texture.xdb" } };
	ASSERT_EQ( pShared->children.size(), 4 );
	for ( int i = 0; i < 4; ++i )
	{
		const auto *pLayer = dynamic_cast<const NDb::SWindowSimple *>( pShared->children[i].GetPtr() );
		ASSERT_NE( pLayer, nullptr );
		EXPECT_EQ( pLayer->szName, layers[i].first );
		const auto *pLayerShared = dynamic_cast<const NDb::SWindowSimpleShared *>( pLayer->pShared.GetPtr() );
		ASSERT_NE( pLayerShared, nullptr );
		if ( layers[i].second )
		{
			CheckTexture( pLayerShared->pBackground, layers[i].second );
			EXPECT_GT( pLayer->nPriority, 0 );
		}
	}
	const auto *pName = Load<NDb::SWindowTextView>( "Name_WindowTextView" );
	ASSERT_NE( pName, nullptr );
	ASSERT_NE( pName->pTextFormat.GetPtr(), nullptr );
	EXPECT_EQ( pName->pTextFormat.GetPtr(),
	           NDb::Get<NDb::STextFormat>( CDBID( "UI/Game/Menu/MPLoading2/Player_TextFormat.xdb" ) ) );
	EXPECT_FALSE( NText::GetText( pName->pTextFormat->szFormatStringFileRef ).empty() );
}
} // namespace
