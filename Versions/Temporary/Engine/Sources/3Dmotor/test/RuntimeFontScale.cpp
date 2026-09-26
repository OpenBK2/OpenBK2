// Exercise the public NGlobal setting and real atlas cache without a GPU.
#include "3Dmotor/stdafx.h"
#include "3Dmotor/DBScene.h"
#include "3Dmotor/GLocale.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <fstream>
#include <limits>
#include <gtest/gtest.h>

namespace
{
class RuntimeFontScale : public testing::Test
{
	NGlobal::CValue savedScale = NGlobal::GetVar( "ui_font_scale", 1.2f );
	CObj<NVFS::IVFS> savedVFS = NVFS::GetMainVFS();
protected:
	void TearDown() override
	{
		NGlobal::SetVar( "ui_font_scale", savedScale );
		NVFS::SetMainVFS( savedVFS );
	}
};

int CellHeight( NGScene::CFontInfo *font )
{
	CDGPtr<CPtrFuncBase<CFontFormatInfo>> format( font->GetFormatInfo() );
	format.Refresh();
	return format->GetValue()->GetHeight();
}
}

TEST_F( RuntimeFontScale, ConsoleAndSetVarChangeTheUserSetting )
{
	EXPECT_FLOAT_EQ( NGlobal::GetVar( "ui_font_scale" ).GetFloat(), 1.2f );
	NGlobal::ProcessCommand( L"ui_font_scale 1.4" );
	EXPECT_FLOAT_EQ( NGScene::GetRuntimeFontScale(), 1.4f );
	NGlobal::SetVar( "ui_font_scale", 1.0f );
	EXPECT_FLOAT_EQ( NGlobal::GetVar( "ui_font_scale" ).GetFloat(), 1.0f );
	EXPECT_FLOAT_EQ( NGScene::GetRuntimeFontScale(), 1.0f );

	std::vector<std::pair<std::string, NGlobal::CValue>> userVariables;
	NGlobal::GetVarsByClass( &userVariables, STORAGE_USER );
	const auto setting = std::find_if( userVariables.begin(), userVariables.end(),
		[]( const auto &entry ) { return entry.first == "ui_font_scale"; } );
	ASSERT_NE( setting, userVariables.end() );
	EXPECT_FLOAT_EQ( setting->second.GetFloat(), 1.0f );
}

TEST_F( RuntimeFontScale, InvalidInputCannotBecomeAnAtlasAllocation )
{
	for ( float value : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() } )
	{
		NGlobal::SetVar( "ui_font_scale", value );
		EXPECT_FLOAT_EQ( NGScene::GetRuntimeFontScale(), 1.2f );
	}
	NGlobal::SetVar( "ui_font_scale", 1000000.0f );
	EXPECT_FLOAT_EQ( NGScene::GetRuntimeFontScale(), 4.0f );
	NGlobal::SetVar( "ui_font_scale", 0.001f );
	EXPECT_FLOAT_EQ( NGScene::GetRuntimeFontScale(), 0.25f );
}

TEST_F( RuntimeFontScale, ChangingScaleReplacesCachedFontsWithoutResizingOldAtlases )
{
	std::ifstream file( std::string( FONT_DATA_DIR ) + "/PTSans-Regular.ttf", std::ios::binary );
	if ( !file )
		GTEST_SKIP() << "Missing shipped PT Sans font";
	NVFS::SetMainVFS( NVFS::CreateWinVFS( std::string( FONT_DATA_DIR ) + "/" ) );
	CObj<NDb::SFont> record = MakeObject<NDb::SFont>( NDb::SFont::typeID );
	ASSERT_TRUE( record );
	record->uid = boost::uuids::uuid();
	record->szName = "runtime_scale_test";
	record->szFontFile = "PTSans-Regular.ttf";
	CObj<NGScene::CTextLocaleInfo> locale = new NGScene::CTextLocaleInfo;
	locale->AddFont( record );
	NGScene::SFont request( 20, record->szName );
	request.nWidth = 27;

	NGlobal::SetVar( "ui_font_scale", 1.0f );
	CObj<NGScene::CFontInfo> original = locale->GetFont( request );
	ASSERT_TRUE( original );
	EXPECT_EQ( CellHeight( original ), 20 );
	EXPECT_EQ( original.GetPtr(), locale->GetFont( request ) );

	NGlobal::SetVar( "ui_font_scale", 1.5f );
	CObj<NGScene::CFontInfo> larger = locale->GetFont( request );
	ASSERT_TRUE( larger );
	EXPECT_NE( original.GetPtr(), larger.GetPtr() );
	EXPECT_EQ( CellHeight( larger ), 30 );
	EXPECT_EQ( CellHeight( original ), 20 );
	EXPECT_EQ( larger.GetPtr(), locale->GetFont( request ) );

	NGlobal::SetVar( "ui_font_scale", 1.0f );
	EXPECT_EQ( CellHeight( locale->GetFont( request ) ), 20 );
}
