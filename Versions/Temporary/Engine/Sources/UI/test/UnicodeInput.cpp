#include "UI/stdafx.h"
#include "UI/Window.h"
#include "3Dmotor/DBScene.h"
#include "3Dmotor/Locale.h"
#include "3Dmotor/GLocale.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "port/virtualkey.h"

#include <clocale>
#include <fstream>
#include <gtest/gtest.h>

namespace
{
// Mixed scripts must survive together, regardless of the active CRT code page.
const std::wstring sample = L"ASCII \u010d\u0107\u017e\u0161\u0111\u00df\u00a4\u00d7\u00f7 "
	L"\u041f\u0440\u0438\u0432\u0435\u0442 \u0402\u0452\u0408\u0458";

void SetPlacement( NDb::SWindowPlacement *placement )
{
	placement->position = VNULL2;
	placement->size = CVec2( 1024, 80 );
	placement->lowerMargin = VNULL2;
	placement->upperMargin = VNULL2;
	placement->horAllign = NDb::EPA_LOW_END;
	placement->verAllign = NDb::EPA_LOW_END;
}

TEST( UnicodeInput, ConverterPreservesCharactersAndRepeatCount )
{
	for ( wchar_t character : sample )
	{
		SCOPED_TRACE( static_cast<int>( character ) );
		NWinFrame::SWindowsMsg source;
		source.msg = NWinFrame::SWindowsMsg::CHAR;
		source.nKey = character;
		source.nRep = 3;
		std::string name;
		int first = -1, second = -1, count = -1;
		NInput::EControlType type = NInput::CT_UNKNOWN;
		ASSERT_TRUE( NInput::ConvertMessage( source, &name, &first, &second, &count, &type ) );
		EXPECT_EQ( name, "win_char" );
		EXPECT_EQ( first, character );
		EXPECT_EQ( second, 0 );
		EXPECT_EQ( count, 3 );
	}
}

class UnicodeEditLine : public testing::Test
{
	std::string oldLocale;
	CObj<NVFS::IVFS> oldVFS;
	CObj<NDb::SFont> font;
	CObj<NDb::STexture> texture;
protected:
	CObj<CWindow> screen;
	CObj<CWindow> window;
	IEditLine *edit = nullptr;

	void SetUp() override
	{
		oldLocale = std::setlocale( LC_CTYPE, nullptr );
		std::setlocale( LC_CTYPE, "C" );
		oldVFS = NVFS::GetMainVFS();
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		NVFS::SetMainVFS( NVFS::CreateWinVFS( std::string( GAME_DATA_DIR ) + "/" ) );
		if ( !std::ifstream( std::string( GAME_DATA_DIR ) + "/Fonts/Files/LiberationSans-Regular.ttf" ) )
			GTEST_SKIP() << "Shipped font is unavailable";

		// Exercise the real edit control and layout, without a graphics device.
		font = MakeObject<NDb::SFont>( NDb::SFont::typeID );
		font->szName = "System";
		font->szFontFile = "Fonts/Files/LiberationSans-Regular.ttf";
		texture = MakeObject<NDb::STexture>( NDb::STexture::typeID );
		texture->szDestName = "Fonts/Numeric/Texture.dds";
		font->pTexture = texture;
		NGScene::GetTextLocaleInfo()->AddFont( font );

		CPtr<NDb::SWindowScreenShared> shared = MakeObject<NDb::SWindowScreenShared>( NDb::SWindowScreenShared::typeID );
		CPtr<NDb::SWindowScreen> desc = MakeObject<NDb::SWindowScreen>( NDb::SWindowScreen::typeID );
		desc->nClassTypeID = 0x11075B80;
		desc->pShared = shared;
		desc->bVisible = true;
		SetPlacement( &desc->placement );
		screen = CUIFactory::MakeWindow( desc );
	}

	void TearDown() override
	{
		if ( window )
			window->SetFocus( false );
		window = nullptr;
		screen = nullptr;
		NInput::PurgeEvents();
		NGScene::GetTextLocaleInfo()->ClearAllFonts();
		NSingleton::UnRegisterSingleton( IUIInitialization::tidTypeID );
		NVFS::SetMainVFS( oldVFS );
		std::setlocale( LC_CTYPE, oldLocale.c_str() );
	}

	void MakeEdit( NDb::ETextEntryType entryType = NDb::ETET_ALL )
	{
		CPtr<NDb::SWindowEditLineShared> shared = MakeObject<NDb::SWindowEditLineShared>( NDb::SWindowEditLineShared::typeID );
		CPtr<NDb::SWindowEditLine> desc = MakeObject<NDb::SWindowEditLine>( NDb::SWindowEditLine::typeID );
		desc->nClassTypeID = 0x11075B83;
		desc->pShared = shared;
		desc->bVisible = true;
		desc->bTextScroll = true;
		desc->eTextEntryType = entryType;
		SetPlacement( &desc->placement );
		window = CUIFactory::MakeWindow( desc );
		screen->AddChild( window, false );
		window->Reposition( CTRect<float>( 0, 0, 1024, 768 ) );
		window->SetFocus( true );
		edit = dynamic_cast<IEditLine*>( window.GetPtr() );
		ASSERT_NE( edit, nullptr );
	}

	void Send( const std::string &name, int character )
	{
		NInput::PostEvent( name, character, 0 );
		SGameMessage event;
		while ( NInput::GetEvent( &event ) )
			window->ProcessEvent( event );
	}

	void Type( const std::wstring &text )
	{
		for ( wchar_t character : text )
		{
			NWinFrame::SWindowsMsg source;
			source.msg = NWinFrame::SWindowsMsg::CHAR;
			source.nKey = character;
			source.nRep = 1;
			std::string name;
			int first = 0, second = 0, count = 0;
			NInput::EControlType type = NInput::CT_UNKNOWN;
			ASSERT_TRUE( NInput::ConvertMessage( source, &name, &first, &second, &count, &type ) );
			ASSERT_EQ( count, 1 );
			Send( name, first );
		}
	}
};

TEST_F( UnicodeEditLine, MixedScriptsCanBeTypedAndEditedInCLocale )
{
	MakeEdit();
	Type( sample );
	EXPECT_EQ( std::wstring( edit->GetText() ), sample );
	Send( "win_key", VK_BACK );
	EXPECT_EQ( std::wstring( edit->GetText() ), sample.substr( 0, sample.size() - 1 ) );
	Type( sample.substr( sample.size() - 1 ) );
	EXPECT_EQ( std::wstring( edit->GetText() ), sample );
}

TEST_F( UnicodeEditLine, UnicodeEventIsAcceptedWithoutByteConversion )
{
	MakeEdit();
	// Isolate the edit filter from the converter, including Linux's C locale.
	for ( wchar_t character : sample )
		Send( "win_char", character );
	EXPECT_EQ( std::wstring( edit->GetText() ), sample );
}

TEST_F( UnicodeEditLine, ControlsAndMarkupRemainRejected )
{
	MakeEdit();
	Type( L"OK" );
	for ( int control : { 0, 8, 9, 10, 13, 27, 31, 0x7f, 0x85, 0x9f, 0x2028, 0x2029 } )
		Send( "win_char", control );
	Type( L"<>" );
	EXPECT_EQ( std::wstring( edit->GetText() ), L"OK" );
}

TEST_F( UnicodeEditLine, NumericFieldsStillRestrictInput )
{
	MakeEdit( NDb::ETET_NUMERIC );
	Type( sample + L"0123456789" );
	EXPECT_EQ( std::wstring( edit->GetText() ), L"0123456789" );
}
}
