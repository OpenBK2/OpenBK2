#include "UI/stdafx.h"
#include "UI/WindowTextView.h"
#include "UI/UIVisitor.h"
#include "UI/UIML.h"
#include "3Dmotor/DBScene.h"
#include "3Dmotor/Locale.h"
#include "Image/Image.h"
#include "Image/ImageDDS.h"
#include "Misc/2Darray.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "System/Text.h"
#include "FontFace.h"
#include "CodePages.h"

#include <boost/uuid/string_generator.hpp>
#include <fstream>
#include <gtest/gtest.h>

namespace
{
// Capture the real control's draw origin and clip without requiring a GPU.
struct TextVisitor : IUIVisitor
{
	CTPoint<float> position;
	CTRect<float> clip;
	CTPoint<int> size;
	CPtr<IML> text;
	bool clipped = false;
	int calls = 0;
	void ClipSet( const CTRect<float>& ) override {}
	void ClipRestore() override {}
	void VisitZClearRect( const CRectLayout&, const CTPoint<float>&, const CTRect<float>&, float ) override {}
	void VisitUIRect( const NDb::STexture*, int, const CRectLayout& ) override {}
	void VisitUIRect( const NDb::STexture*, int, const CVec2*, const NGfx::SPixel8888*, const CTRect<float>& ) override {}
	void VisitUITextureRect( CPtrFuncBase<NGfx::CTexture>*, int, const CRectLayout& ) override {}
	void VisitUIText( IML *value, const CTPoint<float> &origin, const CTRect<float> &window ) override
	{
		text = value;
		position = origin;
		clip = window;
		size = text->GetSize();
		clipped = text->HasVisibleTextOutside( origin, window );
		++calls;
	}
};

class TextVerticalAlignment : public testing::Test
{
	NGlobal::CValue oldHudScale = NGlobal::GetVar( "hud_font_scale", 1.0f );
	NGlobal::CValue oldScale = NGlobal::GetVar( "ui_font_scale", 1.2f );
	CObj<NVFS::IVFS> oldVFS = NVFS::GetMainVFS();
	CObj<NDb::SFont> numeric;
	CObj<NDb::STexture> texture;
	CObj<NDb::SFont> header;
	CObj<NDb::STexture> headerTexture;
protected:
	double capRatio = 0;
	void AddHeaderFont()
	{
		// The loading advice and menu buttons use Oswald Bold, not numeric.
		header = MakeObject<NDb::SFont>( NDb::SFont::typeID );
		header->uid = boost::uuids::string_generator()( "a64c37c3-495a-4c69-8602-6117fc6ecc70" );
		header->szName = "h2";
		header->szFontFile = "Fonts/Files/Oswald-Variable.ttf";
		header->nThickness = 700;
		headerTexture = MakeObject<NDb::STexture>( NDb::STexture::typeID );
		headerTexture->szDestName = "Fonts/Header2/Texture.dds";
		header->pTexture = headerTexture;
		NGScene::GetTextLocaleInfo()->AddFont( header );
	}
	CObj<IML> MakeText( const std::wstring &content, int width, IWindow *owner = nullptr )
	{
		CObj<IML> text = CreateML( owner );
		text->SetText( content, 0 );
		text->Generate( width );
		return text;
	}
	void SetUp() override
	{
		ASSERT_TRUE( std::ifstream( std::string( GAME_DATA_DIR ) + "/Fonts/Files/LiberationSans-Regular.ttf" ) );
		NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
		NVFS::SetMainVFS( NVFS::CreateWinVFS( std::string( GAME_DATA_DIR ) + "/" ) );
		numeric = MakeObject<NDb::SFont>( NDb::SFont::typeID );
		numeric->uid = boost::uuids::string_generator()( "da637b05-0923-47ee-b954-c2345dea71dd" );
		numeric->szName = "numeric";
		numeric->szFontFile = "Fonts/Files/LiberationSans-Regular.ttf";
		texture = MakeObject<NDb::STexture>( NDb::STexture::typeID );
		texture->szDestName = "Fonts/Numeric/Texture.dds";
		numeric->pTexture = texture;
		NGScene::GetTextLocaleInfo()->AddFont( numeric );
		CObj<NGScene::CFontInfo> baked = NGScene::GetTextLocaleInfo()->GetFont( NGScene::SFont( 1, "numeric" ) );
		ASSERT_TRUE( baked );
		CDGPtr<CPtrFuncBase<CFontFormatInfo>> bakedFormat( baked->GetFormatInfo() );
		bakedFormat.Refresh();
		const auto &capital = bakedFormat->GetValue()->GetChar( 'H' );
		CFileStream stream( NVFS::GetMainVFS(), texture->szDestName );
		CArray2D<uint32_t> pixels;
		ASSERT_TRUE( stream.IsOk() );
		// Match the renderer's reference sizing when it can read the baked
		// atlas; otherwise both use ink fitting.
		if ( NImage::LoadImageDDS( &pixels, &stream ) )
		{
			int top = capital.y2, bottom = capital.y1;
			for ( int y = capital.y1; y < capital.y2; ++y )
				for ( int x = capital.x1; x < capital.x2; ++x )
					if ( pixels[y][x] >> 24 )
					{
						top = (std::min)( top, y );
						bottom = (std::max)( bottom, y + 1 );
					}
			capRatio = double( bottom - top ) / bakedFormat->GetValue()->GetLineSpace();
		}
		NGlobal::SetVar( "ui_font_scale", 1.2f );
		NGlobal::SetVar( "hud_font_scale", 1.0f );
	}
	void TearDown() override
	{
		NGScene::GetTextLocaleInfo()->ClearAllFonts();
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( 1024, 768 );
		NSingleton::UnRegisterSingleton( IUIInitialization::tidTypeID );
		NGlobal::SetVar( "ui_font_scale", oldScale );
		NGlobal::SetVar( "hud_font_scale", oldHudScale );
		NVFS::SetMainVFS( oldVFS );
	}
	IScreen *MakeScreen( bool hud )
	{
		CPtr<NDb::SWindowScreenShared> shared = MakeObject<NDb::SWindowScreenShared>( NDb::SWindowScreenShared::typeID );
		CPtr<NDb::SWindowScreen> desc = MakeObject<NDb::SWindowScreen>( NDb::SWindowScreen::typeID );
		desc->nClassTypeID = 0x11075B80;
		desc->pShared = shared;
		desc->bVisible = true;
		desc->placement.position = VNULL2;
		desc->placement.size = CVec2( 1024, 768 );
		desc->placement.horAllign = NDb::EPA_LOW_END;
		desc->placement.verAllign = NDb::EPA_LOW_END;
		desc->placement.lowerMargin = VNULL2;
		desc->placement.upperMargin = VNULL2;
		IScreen *screen = dynamic_cast<IScreen*>( CUIFactory::MakeWindow( desc ) );
		screen->SetHudFontScale( hud );
		return screen;
	}
	CWindowTextView *MakeLabel( float width, float height, bool resize = false, const NDb::SWindowPlacement *captionPlacement = nullptr )
	{
		CPtr<NDb::SWindowTextViewShared> shared = MakeObject<NDb::SWindowTextViewShared>( NDb::SWindowTextViewShared::typeID );
		CPtr<NDb::SWindowTextView> desc = MakeObject<NDb::SWindowTextView>( NDb::SWindowTextView::typeID );
		desc->nClassTypeID = 0x11075B8C;
		desc->pShared = shared;
		desc->bVisible = true;
		desc->bResizeOnTextSet = resize;
		if ( captionPlacement )
		{
			CPtr<NDb::SForegroundTextStringShared> captionShared = MakeObject<NDb::SForegroundTextStringShared>( NDb::SForegroundTextStringShared::typeID );
			captionShared->position = *captionPlacement;
			CPtr<NDb::SForegroundTextString> caption = MakeObject<NDb::SForegroundTextString>( NDb::SForegroundTextString::typeID );
			caption->pShared = captionShared;
			desc->pTextString = caption;
		}
		desc->placement.position = CVec2( 55, 26 );
		desc->placement.size = CVec2( width, height );
		desc->placement.horAllign = NDb::EPA_LOW_END;
		desc->placement.verAllign = NDb::EPA_LOW_END;
		desc->placement.lowerMargin = VNULL2;
		desc->placement.upperMargin = VNULL2;
		CWindowTextView *label = dynamic_cast<CWindowTextView*>( CUIFactory::MakeWindow( desc ) );
		label->Reposition( CTRect<float>( 0, 0, 1024, 768 ) );
		return label;
	}
	CPtr<NDb::SWindowSimple> PanelDescription( float width, float height )
	{
		CPtr<NDb::SWindowSimpleShared> shared = MakeObject<NDb::SWindowSimpleShared>( NDb::SWindowSimpleShared::typeID );
		CPtr<NDb::SWindowSimple> desc = MakeObject<NDb::SWindowSimple>( NDb::SWindowSimple::typeID );
		desc->nClassTypeID = 0x110772C1;
		desc->pShared = shared;
		desc->bVisible = true;
		desc->placement.position = VNULL2;
		desc->placement.size = CVec2( width, height );
		desc->placement.horAllign = NDb::EPA_LOW_END;
		desc->placement.verAllign = NDb::EPA_LOW_END;
		desc->placement.lowerMargin = VNULL2;
		desc->placement.upperMargin = VNULL2;
		return desc;
	}
	CObj<CWindow> MakePanel( float width, float height )
	{
		return CUIFactory::MakeWindow( PanelDescription( width, height ) );
	}
	CObj<CWindow> MakeScrollPanel( float width, float height )
	{
		CPtr<NDb::SWindowSimple> border = PanelDescription( width, height );
		border->placement.horAllign = NDb::EPA_MARGIN;
		border->placement.verAllign = NDb::EPA_MARGIN;
		CPtr<NDb::SWindowScrollableContainerShared> shared = MakeObject<NDb::SWindowScrollableContainerShared>( NDb::SWindowScrollableContainerShared::typeID );
		shared->placement = border->placement;
		shared->pBorder = border;
		CPtr<NDb::SWindowScrollableContainer> desc = MakeObject<NDb::SWindowScrollableContainer>( NDb::SWindowScrollableContainer::typeID );
		desc->nClassTypeID = 0x170AF301;
		desc->pShared = shared;
		desc->bVisible = true;
		desc->placement = PanelDescription( width, height )->placement;
		return CUIFactory::MakeWindow( desc );
	}
};
}

TEST_F( TextVerticalAlignment, FixedHeightUnitStatsKeepTheirDigitsInsideTheClip )
{
	// The tutorial tank's WeaponBrief fields are 12 virtual pixels tall and
	// use the numeric font at 14 points.
	std::string error;
	auto face = NFontRaster::CFace::OpenFile( std::string( GAME_DATA_DIR ) + "/Fonts/Files/LiberationSans-Regular.ttf", 0, &error );
	ASSERT_TRUE( face ) << error;
	std::vector<uint32_t> sizing;
	for ( int charset : { NCodePages::CHARSET_ANSI, NCodePages::CHARSET_EASTEUROPE, NCodePages::CHARSET_RUSSIAN,
		NCodePages::CHARSET_GREEK, NCodePages::CHARSET_TURKISH, NCodePages::CHARSET_BALTIC } )
	{
		const auto chars = NCodePages::GetPrintableCodePoints( charset );
		sizing.insert( sizing.end(), chars.begin(), chars.end() );
	}
	std::sort( sizing.begin(), sizing.end() );
	sizing.erase( std::unique( sizing.begin(), sizing.end() ), sizing.end() );
	for ( auto resolution : { CTPoint<int>( 1024, 768 ), CTPoint<int>( 1920, 1080 ), CTPoint<int>( 2560, 1440 ), CTPoint<int>( 3840, 2160 ) } )
	{
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( resolution.x, resolution.y );
		CObj<CWindowTextView> label = MakeLabel( 100, 12 );
		for ( float scale : { 1.0f, 1.2f } )
		{
			SCOPED_TRACE( std::to_string( resolution.y ) + " scale " + std::to_string( scale ) );
			NGlobal::SetVar( "ui_font_scale", scale );
			NFontRaster::SOptions options;
			options.eCellMetrics = NFontRaster::CELL_INK;
			options.nCellHeight = std::lround( NGScene::FontPointsToPixels( 14, resolution.y ) * scale );
			options.nCapHeight = std::lround( options.nCellHeight * capRatio );
			ASSERT_TRUE( face->Fit( options, sizing, &error ) ) << error;
			// Check the independent bitmap measurements use the runtime cell size.
			NGScene::SFont request( NGScene::FontPointsToPixels( 14, resolution.y ), "numeric" );
			request.nWidth = NGScene::FontPointsToPixelWidth( 14, resolution.x );
			CDGPtr<CPtrFuncBase<CFontFormatInfo>> format( NGScene::GetTextLocaleInfo()->GetFont( request )->GetFormatInfo() );
			format.Refresh();
			ASSERT_EQ( format->GetValue()->GetHeight(), face->GetMetrics().nCellHeight );
			for ( const std::wstring digits : { L"65", L"80", L"84/84", L"3750/3750" } )
			{
				label->SetText( L"<font face=numeric size=14>" + digits );
				TextVisitor visitor;
				label->Visit( &visitor );
				ASSERT_EQ( visitor.calls, 1 );
				ASSERT_GT( visitor.size.y, visitor.clip.Height() );
				EXPECT_NEAR( visitor.position.y + visitor.size.y / 2.0f, visitor.clip.GetCenter().y, 0.5f );
				for ( wchar_t ch : digits )
				{
					NFontRaster::SGlyphBitmap glyph;
					ASSERT_TRUE( face->RenderGlyph( ch, &glyph, &error ) ) << error;
					const float top = visitor.position.y + face->GetMetrics().nAscent - glyph.nTop;
					EXPECT_GE( top, visitor.clip.y1 );
					EXPECT_LE( top + glyph.nRows, visitor.clip.y2 );
				}
			}
		}
	}
}

TEST_F( TextVerticalAlignment, MultilineAndAutosizedTextKeepTheirTopOrigin )
{
	for ( bool resize : { false, true } )
	{
		CObj<CWindowTextView> label = MakeLabel( 100, 12, resize );
		for ( const std::wstring text : { L"65\n80", L"65 80 84/84 3750/3750 65 80 84/84 3750/3750" } )
		{
			label->SetText( L"<font face=numeric size=14>" + text );
			TextVisitor visitor;
			label->Visit( &visitor );
			ASSERT_EQ( visitor.calls, 1 );
			EXPECT_FLOAT_EQ( visitor.position.y, visitor.clip.y1 );
		}
	}
}

TEST_F( TextVerticalAlignment, FittingAndAutosizedSingleLinesKeepTheirTopOrigin )
{
	for ( bool resize : { false, true } )
	{
		CObj<CWindowTextView> label = MakeLabel( 100, 40, resize );
		label->SetText( L"<font face=numeric size=14>84/84" );
		TextVisitor visitor;
		label->Visit( &visitor );
		ASSERT_EQ( visitor.calls, 1 );
		EXPECT_FLOAT_EQ( visitor.position.y, visitor.clip.y1 );
	}
}

TEST_F( TextVerticalAlignment, HudAndMenuLabelsRefreshIndependently )
{
	CObj<IScreen> hud = MakeScreen( true );
	CObj<IScreen> menu = MakeScreen( false );
	CObj<CWindowTextView> hudLabel = MakeLabel( 200, 12, true );
	CObj<CWindowTextView> menuLabel = MakeLabel( 200, 12, true );
	hud->AddChild( hudLabel, false );
	menu->AddChild( menuLabel, false );
	hudLabel->SetText( L"<font face=numeric size=14>84/84" );
	menuLabel->SetText( hudLabel->GetText() );
	TextVisitor hudBefore, menuBefore;
	hudLabel->Visit( &hudBefore );
	menuLabel->Visit( &menuBefore );
	ASSERT_GT( menuBefore.size.y, hudBefore.size.y );
	ASSERT_GT( menuBefore.size.x, hudBefore.size.x );

	NGlobal::SetVar( "hud_font_scale", 1.5f );
	TextVisitor hudAfter, menuAfter;
	menuLabel->Visit( &menuAfter );
	hudLabel->Visit( &hudAfter );
	EXPECT_EQ( menuAfter.size, menuBefore.size );
	EXPECT_GT( hudAfter.size.y, hudBefore.size.y );
	EXPECT_EQ( hudLabel->GetWindowRect().Height(), hudAfter.size.y );

	NGlobal::SetVar( "ui_font_scale", 1.0f );
	TextVisitor hudFinal, menuFinal;
	hudLabel->Visit( &hudFinal );
	menuLabel->Visit( &menuFinal );
	EXPECT_EQ( hudFinal.size, hudAfter.size );
	EXPECT_EQ( menuFinal.size, hudBefore.size );
}

TEST_F( TextVerticalAlignment, TextFollowsItsWindowWhenMovedBetweenScreens )
{
	CObj<IScreen> hud = MakeScreen( true );
	CObj<IScreen> menu = MakeScreen( false );
	CObj<CWindowTextView> label = MakeLabel( 200, 12, true );
	label->SetText( L"<font face=numeric size=14>3750/3750" );
	const auto guiSize = label->GetSize();
	hud->AddChild( label, false );
	TextVisitor smaller;
	label->Visit( &smaller );
	EXPECT_LT( smaller.size.y, guiSize.y );
	menu->AddChild( label, false );
	TextVisitor restored;
	label->Visit( &restored );
	EXPECT_EQ( restored.size, guiSize );
}

TEST_F( TextVerticalAlignment, ScreenScaleAppliesBeforeChildrenAreAttached )
{
	CObj<IScreen> hud = MakeScreen( true );
	CObj<CWindowTextView> label = MakeLabel( 200, 12, true );
	// Screen loading lays out children before AddChild supplies their parent.
	CUIFactory::SetScreenDuringLoad( hud );
	label->SetText( L"<font face=numeric size=14>84/84" );
	const auto initialSize = label->GetSize();
	CUIFactory::SetScreenDuringLoad( nullptr );
	hud->AddChild( label, false );
	EXPECT_EQ( label->GetSize(), initialSize );
	CObj<CWindowTextView> menuLabel = MakeLabel( 200, 12, true );
	menuLabel->SetText( label->GetText() );
	EXPECT_GT( menuLabel->GetSize().y, initialSize.y );
}

TEST_F( TextVerticalAlignment, PlacedCaptionsUseTheirOwningScreen )
{
	CObj<IScreen> hud = MakeScreen( true );
	CObj<IScreen> menu = MakeScreen( false );
	CObj<CWindowTextView> hudLabel = MakeLabel( 200, 30 );
	CObj<CWindowTextView> menuLabel = MakeLabel( 200, 30 );
	hud->AddChild( hudLabel, false );
	menu->AddChild( menuLabel, false );
	// Button-style captions use CPlacedText rather than the TextView layout.
	hudLabel->CWindow::SetTextString( L"<font face=numeric size=14>84/84" );
	menuLabel->CWindow::SetTextString( L"<font face=numeric size=14>84/84" );
	EXPECT_GT( menuLabel->GetOptimalWidth(), hudLabel->GetOptimalWidth() );
	NGlobal::SetVar( "hud_font_scale", 1.2f );
	EXPECT_EQ( menuLabel->GetOptimalWidth(), hudLabel->GetOptimalWidth() );
}

TEST_F( TextVerticalAlignment, HorizontalClippingFallsBackOnlyForTheAffectedLabel )
{
	const std::wstring content = L"<font face=numeric size=14><nowrap>12345678901234567890";
	CObj<IScreen> hud = MakeScreen( true );
	CObj<IML> original = MakeText( content, 500, hud );
	const auto originalSize = original->GetSize();
	CObj<IML> preferred = MakeText( content, 500 );
	CObj<CWindowTextView> narrow = MakeLabel( originalSize.x + 2, 40 );
	CObj<CWindowTextView> wide = MakeLabel( 500, 40 );
	narrow->SetText( content );
	wide->SetText( content );
	TextVisitor narrowVisitor, large;
	narrow->Visit( &narrowVisitor );
	wide->Visit( &large );
	EXPECT_EQ( narrowVisitor.size, originalSize );
	EXPECT_FALSE( narrowVisitor.clipped );
	EXPECT_EQ( large.size, preferred->GetSize() );
	EXPECT_GT( large.size.x, narrowVisitor.size.x );
	EXPECT_FLOAT_EQ( NGlobal::GetVar( "ui_font_scale" ).GetFloat(), 1.2f );
	EXPECT_FLOAT_EQ( NGlobal::GetVar( "hud_font_scale" ).GetFloat(), 1.0f );

	// No per-frame relayout or size oscillation once fallback is selected.
	CObj<CMLStream> stream = narrowVisitor.text->GetStream();
	for ( int i = 0; i < 5; ++i )
	{
		narrow->Visit( &narrowVisitor );
		EXPECT_EQ( narrowVisitor.size, originalSize );
		EXPECT_EQ( narrowVisitor.text->GetStream(), stream.GetPtr() );
	}
	// More room permits the chosen scale again.
	narrow->SetWidth( 500 );
	narrow->Visit( &narrowVisitor );
	EXPECT_EQ( narrowVisitor.size, large.size );
	// Replacing the text also discards the previous clipping decision.
	narrow->SetWidth( originalSize.x + 2 );
	narrow->SetText( L"<font face=numeric size=14>65" );
	narrow->Visit( &narrowVisitor );
	EXPECT_EQ( narrowVisitor.size.y, large.size.y );
	EXPECT_FALSE( narrowVisitor.clipped );
}

TEST_F( TextVerticalAlignment, PlacedCaptionFallbackKeepsItsAlignment )
{
	const std::wstring content = L"<font face=numeric size=14><nowrap>12345678901234567890";
	CObj<IScreen> hud = MakeScreen( true );
	CObj<IML> original = MakeText( content, 500, hud );
	const auto originalSize = original->GetSize();
	NDb::SWindowPlacement placement;
	placement.position = VNULL2;
	placement.lowerMargin = VNULL2;
	placement.upperMargin = VNULL2;
	placement.horAllign = NDb::EPA_MARGIN;
	placement.verAllign = NDb::ERA_CENTER;
	CObj<CWindowTextView> label = MakeLabel( originalSize.x + 2, 50, false, &placement );
	label->CWindow::SetTextString( content );
	TextVisitor visitor;
	label->Visit( &visitor );
	ASSERT_EQ( visitor.calls, 1 );
	EXPECT_EQ( visitor.size, originalSize );
	EXPECT_FALSE( visitor.clipped );
	EXPECT_NEAR( visitor.position.y + visitor.size.y / 2.0f, label->GetWindowRect().GetCenter().y, 0.5f );
	CObj<CMLStream> stream = visitor.text->GetStream();
	label->Visit( &visitor );
	EXPECT_EQ( visitor.text->GetStream(), stream.GetPtr() );
}

TEST_F( TextVerticalAlignment, VerticalClippingReflowsTheWholeParagraphAtOne )
{
	CObj<IScreen> hud = MakeScreen( true );
	for ( const std::wstring content : {
		L"<font face=numeric size=14>65\n80\n84/84",
		L"<font face=numeric size=14>65 80 84/84 3750/3750 65 80 84/84 3750/3750" } )
	{
		CObj<IML> original = MakeText( content, 100, hud );
		const auto originalSize = original->GetSize();
		CObj<CWindowTextView> label = MakeLabel( 100, originalSize.y );
		label->SetText( content );
		TextVisitor visitor;
		label->Visit( &visitor );
		EXPECT_EQ( visitor.size, originalSize );
		EXPECT_FALSE( visitor.clipped );
		EXPECT_FLOAT_EQ( visitor.position.y, visitor.clip.y1 );
	}
}

TEST_F( TextVerticalAlignment, FallbackRealignsSingleLinesAndRetriesHeightAndSettingChanges )
{
	CObj<IScreen> hud = MakeScreen( true );
	const std::wstring content = L"<font face=numeric size=14>84/84";
	CObj<IML> original = MakeText( content, 200, hud );
	NGlobal::SetVar( "ui_font_scale", 2.0f );
	CObj<CWindowTextView> label = MakeLabel( 200, 12 );
	label->SetText( content );
	TextVisitor visitor;
	label->Visit( &visitor );
	EXPECT_EQ( visitor.size, original->GetSize() );
	EXPECT_FALSE( visitor.clipped );
	EXPECT_NEAR( visitor.position.y + visitor.size.y / 2.0f, visitor.clip.GetCenter().y, 0.5f );

	label->SetPlacement( 0, 0, 0, 50, EWPF_SIZE_Y );
	label->Visit( &visitor );
	EXPECT_GT( visitor.size.y, original->GetSize().y );
	EXPECT_FALSE( visitor.clipped );
	label->SetPlacement( 0, 0, 0, 12, EWPF_SIZE_Y );
	label->Visit( &visitor );
	EXPECT_EQ( visitor.size, original->GetSize() );
	NGlobal::SetVar( "ui_font_scale", 1.2f );
	label->Visit( &visitor );
	EXPECT_GT( visitor.size.y, original->GetSize().y );
	EXPECT_FALSE( visitor.clipped );
}

TEST_F( TextVerticalAlignment, DetectionIgnoresPaddingSpacesAndTransparentGlyphsButIncludesOutlines )
{
	const CTRect<float> box( 0, 0, 200, 12 );
	CObj<IML> digits = MakeText( L"<font face=numeric size=14>84/84", 200 );
	const auto preferredSize = digits->GetSize();
	ASSERT_GT( preferredSize.y, box.Height() );
	const auto origin = digits->FitToBox( box, true );
	EXPECT_EQ( digits->GetSize(), preferredSize );
	EXPECT_FALSE( digits->HasVisibleTextOutside( origin, box ) );

	CObj<IML> invisible = MakeText( L"<font face=numeric size=40><color=00ffffff>84/84   ", 200 );
	EXPECT_FALSE( invisible->HasVisibleTextOutside( CTPoint<float>( 0, 0 ), box ) );
	CObj<IML> spaces = MakeText( L"<font face=numeric size=40><nowrap>                  ", 200 );
	EXPECT_FALSE( spaces->HasVisibleTextOutside( CTPoint<float>( 0, 0 ), box ) );

	CObj<IML> outlined = MakeText( L"<font face=numeric size=14 outlinesize=3>84/84", 200 );
	const auto outlinedSize = outlined->GetSize();
	const auto outlineOrigin = CTPoint<float>( 0, std::floor( ( box.Height() - outlinedSize.y ) * 0.5f + 0.5f ) );
	EXPECT_TRUE( outlined->HasVisibleTextOutside( outlineOrigin, box ) );
	outlined->FitToBox( box, true );
	// The diagnostic can report a clipped border without automatically
	// shrinking the whole label when its letters still fit.
	EXPECT_EQ( outlined->GetSize(), outlinedSize );
}

TEST_F( TextVerticalAlignment, HudFallbackDoesNotChangeMenuScaleAndStopsAtOne )
{
	CObj<IScreen> hud = MakeScreen( true );
	const std::wstring content = L"<font face=numeric size=14>84/84";
	CObj<IML> text = MakeText( content, 200, hud );
	const auto originalSize = text->GetSize();
	NGlobal::SetVar( "hud_font_scale", 2.0f );
	CObj<IML> menu = MakeText( content, 200 );
	const auto menuSize = menu->GetSize();
	const CTRect<float> tooSmall( 0, 0, 2, 2 );
	text->FitToBox( tooSmall );
	EXPECT_EQ( text->GetSize(), originalSize );
	EXPECT_TRUE( text->HasVisibleTextOutside( tooSmall.GetLeftTop(), tooSmall ) );
	EXPECT_EQ( menu->GetSize(), menuSize );
	EXPECT_FLOAT_EQ( NGlobal::GetVar( "hud_font_scale" ).GetFloat(), 2.0f );
	NGlobal::SetVar( "hud_font_scale", 0.8f );
	const auto smallerSize = text->GetSize();
	text->FitToBox( tooSmall );
	EXPECT_EQ( text->GetSize(), smallerSize );
	EXPECT_LT( smallerSize.y, originalSize.y );
}

TEST_F( TextVerticalAlignment, ScrollingViewportDoesNotReduceAutosizedDescriptions )
{
	// Match the visitor's parent viewport: it is deliberately narrower/shorter
	// than the full content box passed by an autosized scrolling description.
	struct ScrollVisitor : TextVisitor
	{
		bool viewportClipped = false;
		void VisitUIText( IML *text, const CTPoint<float> &origin, const CTRect<float> &window ) override
		{
			TextVisitor::VisitUIText( text, origin, window );
			CTRect<float> viewport = window;
			viewport.y2 = viewport.y1 + 12;
			viewportClipped = text->HasVisibleTextOutside( origin, viewport );
		}
	};
	CObj<CWindowTextView> label = MakeLabel( 200, 12, true );
	label->SetText( L"<font face=numeric size=14>65\n80\n84/84\n3750/3750" );
	const auto preferredSize = label->GetSize();
	for ( int offset : { 0, -10, -30 } )
	{
		label->SetPlacement( 0, offset, 0, 0, EWPF_POS_Y );
		ScrollVisitor visitor;
		label->Visit( &visitor );
		EXPECT_TRUE( visitor.viewportClipped );
		EXPECT_FALSE( visitor.clipped );
		EXPECT_EQ( visitor.size, preferredSize );
	}
}

TEST_F( TextVerticalAlignment, LoadingAdviceKeepsItsScaleWhenTheLettersFit )
{
	AddHeaderFont();
	const std::wstring style = NText::GetText( "Consts/Game/Tags/h2_Text.txt" );
	ASSERT_FALSE( style.empty() );
	for ( auto resolution : { CTPoint<int>( 1024, 768 ), CTPoint<int>( 1920, 1080 ), CTPoint<int>( 2560, 1440 ) } )
	{
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( resolution.x, resolution.y );
		int fittingAdvice = 0;
		for ( int i = 1; i <= 28; ++i )
		{
			SCOPED_TRACE( std::to_string( resolution.y ) + " citation " + std::to_string( i ) );
			const std::string path = "Citations/" + std::string( i < 10 ? "0" : "" ) + std::to_string( i ) + ".txt";
			const std::wstring advice = NText::GetText( path );
			// The shipped citation list includes unused empty files (23, 28).
			if ( advice.empty() )
				continue;
			CObj<CWindowTextView> label = MakeLabel( 948, 65 );
			CTRect<float> box;
			VirtualToScreen( label->GetWindowRect(), &box );
			// Same font/advance/wrapping, with only the decorative border removed.
			CObj<IML> letters = MakeText( L"<font face=h2 size=20>" + advice, box.Width() + 0.5f );
			if ( letters->HasVisibleTextOutside( box.GetLeftTop(), box ) )
				continue;
			++fittingAdvice;
			label->SetText( style + advice );
			CObj<IML> preferred = MakeText( style + advice, box.Width() + 0.5f );
			TextVisitor visitor;
			label->Visit( &visitor );
			EXPECT_EQ( visitor.size, preferred->GetSize() );
		}
		EXPECT_GE( fittingAdvice, 10 );
	}
}

TEST_F( TextVerticalAlignment, MenuButtonHeadingsDoNotShrinkForTheirBorder )
{
	AddHeaderFont();
	const std::wstring style = NText::GetText( "Consts/Game/Tags/h2_Text.txt" ) + L"<center>";
	for ( auto resolution : { CTPoint<int>( 1024, 768 ), CTPoint<int>( 1920, 1080 ), CTPoint<int>( 2560, 1440 ) } )
	{
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( resolution.x, resolution.y );
		for ( const std::wstring caption : { L"Resume", L"Options", L"Save Game", L"Exit To Windows" } )
		{
			SCOPED_TRACE( std::to_string( resolution.y ) + " " + std::string( caption.begin(), caption.end() ) );
			CObj<CWindowTextView> label = MakeLabel( 250, 40 );
			label->CWindow::SetTextString( style + caption );
			const int before = label->GetOptimalWidth();
			TextVisitor visitor;
			label->Visit( &visitor );
			EXPECT_EQ( label->GetOptimalWidth(), before );
		}
	}
}

TEST_F( TextVerticalAlignment, OutlinedAdviceStillFallsBackWhenLettersAreCutOff )
{
	AddHeaderFont();
	const std::wstring style = NText::GetText( "Consts/Game/Tags/h2_Text.txt" );
	const std::wstring advice = NText::GetText( "Citations/02.txt" );
	ASSERT_FALSE( advice.empty() );
	CObj<IScreen> hud = MakeScreen( true );
	for ( auto resolution : { CTPoint<int>( 1024, 768 ), CTPoint<int>( 1920, 1080 ), CTPoint<int>( 2560, 1440 ) } )
	{
		SCOPED_TRACE( resolution.y );
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( resolution.x, resolution.y );
		const int width = NGScene::FontPointsToPixelWidth( 948, resolution.x );
		CObj<IML> original = MakeText( style + advice, width, hud );
		const auto originalSize = original->GetSize();
		const CTRect<float> box( 0, 0, width, originalSize.y );
		CObj<IML> letters = MakeText( L"<font face=h2 size=20>" + advice, width );
		ASSERT_TRUE( letters->HasVisibleTextOutside( box.GetLeftTop(), box ) );
		CObj<IML> outlined = MakeText( style + advice, width );
		outlined->FitToBox( box );
		EXPECT_EQ( outlined->GetSize(), originalSize );
	}
}

TEST_F( TextVerticalAlignment, LoadingAdviceFitsItsActualParentClipAtEveryResolution )
{
	AddHeaderFont();
	const std::wstring style = NText::GetText( "Consts/Game/Tags/h2_Text.txt" );
	struct ClippedVisitor : TextVisitor
	{
		std::vector<CTRect<float>> clips;
		bool lettersClipped = false;
		void ClipSet( const CTRect<float> &clip ) override { clips.push_back( clip ); }
		void ClipRestore() override { clips.pop_back(); }
		void VisitUIText( IML *text, const CTPoint<float> &origin, const CTRect<float> &box ) override
		{
			CTRect<float> actualClip = box;
			for ( const auto &clip : clips )
				actualClip.Intersect( clip );
			TextVisitor::VisitUIText( text, origin, actualClip );
			lettersClipped = text->HasVisibleTextOutside( origin, actualClip, false );
		}
	};
	for ( auto resolution : { CTPoint<int>( 1024, 768 ), CTPoint<int>( 1920, 1080 ), CTPoint<int>( 2560, 1440 ) } )
	{
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( resolution.x, resolution.y );
		// Shipped BottomPanel is 100 high; Citation starts at y=38 and claims
		// 65 pixels, but its parent's render clip leaves only 62 available.
		CObj<CWindow> panel = MakePanel( 1002, 100 );
		panel->SetPlacement( 11, 657, 0, 0, EWPF_POS_X | EWPF_POS_Y );
		CObj<CWindowTextView> label = MakeLabel( 948, 65 );
		label->SetPlacement( 26, 38, 0, 0, EWPF_POS_X | EWPF_POS_Y );
		panel->AddChild( label, true );
		for ( int i = 1; i <= 28; ++i )
		{
			SCOPED_TRACE( std::to_string( resolution.y ) + " citation " + std::to_string( i ) );
			const std::string path = "Citations/" + std::string( i < 10 ? "0" : "" ) + std::to_string( i ) + ".txt";
			const std::wstring advice = NText::GetText( path );
			if ( advice.empty() )
				continue;
			label->SetText( style + advice );
			ClippedVisitor visitor;
			panel->Visit( &visitor );
			ASSERT_EQ( visitor.calls, 1 );
			EXPECT_FALSE( visitor.lettersClipped );
		}
	}
}

TEST_F( TextVerticalAlignment, RealScrollViewportExcludesAncestorsButKeepsContentPanelClips )
{
	CObj<CWindow> outer = MakePanel( 200, 20 );
	CObj<CWindow> scroll = MakeScrollPanel( 200, 30 );
	outer->AddChild( scroll, true );
	IScrollableContainer *container = dynamic_cast<IScrollableContainer*>( scroll.GetPtr() );
	ISliderNotify *slider = dynamic_cast<ISliderNotify*>( scroll.GetPtr() );
	ASSERT_NE( container, nullptr );
	ASSERT_NE( slider, nullptr );
	CObj<CWindowTextView> label = MakeLabel( 200, 12, true );
	label->SetPlacement( 0, 0, 0, 0, EWPF_POS_X | EWPF_POS_Y );
	label->SetText( L"<font face=numeric size=14>65\n80\n84/84\n3750/3750" );
	const auto preferredSize = label->GetSize();
	container->PushBack( label, false );
	container->Update();
	for ( int offset : { 0, 10, 30 } )
	{
		slider->SliderPosition( offset, nullptr );
		TextVisitor visitor;
		outer->Visit( &visitor );
		ASSERT_EQ( visitor.calls, 1 );
		EXPECT_EQ( visitor.size, preferredSize );
		// Visiting the tree must leave no layout clip behind on the visitor.
		const CTRect<float> probe( 0, 0, 200, 100 );
		EXPECT_FLOAT_EQ( visitor.GetTextClip( probe ).y2, 100 );
	}
	// Clipping inside the scrollable content is still a real layout limit.
	container->RemoveItems();
	CObj<CWindow> contentPanel = MakePanel( 200, 30 );
	CObj<CWindowTextView> constrained = MakeLabel( 200, 70 );
	constrained->SetPlacement( 0, 0, 0, 0, EWPF_POS_X | EWPF_POS_Y );
	constrained->SetText( L"<font face=numeric size=14>65\n80" );
	contentPanel->AddChild( constrained, true );
	container->PushBack( contentPanel, false );
	container->Update();
	NGlobal::SetVar( "ui_font_scale", 2.0f );
	CObj<IML> preferred = MakeText( constrained->GetText(), 200 );
	EXPECT_FALSE( preferred->HasVisibleTextOutside( CTPoint<float>( 0, 0 ), CTRect<float>( 0, 0, 200, 70 ), false ) );
	EXPECT_TRUE( preferred->HasVisibleTextOutside( CTPoint<float>( 0, 0 ), CTRect<float>( 0, 0, 200, 30 ), false ) );
	CObj<IScreen> hud = MakeScreen( true );
	CObj<IML> original = MakeText( constrained->GetText(), 200, hud );
	TextVisitor visitor;
	outer->Visit( &visitor );
	ASSERT_EQ( visitor.calls, 1 );
	EXPECT_EQ( visitor.size, original->GetSize() );
}

TEST_F( TextVerticalAlignment, LayoutClipScopesRestoreAfterNestedScrollViewports )
{
	TextVisitor visitor;
	const CTRect<float> probe( 0, 0, 200, 200 );
	{
		CClipStore panel( &visitor, CTRect<float>( 0, 0, 100, 50 ) );
		EXPECT_FLOAT_EQ( visitor.GetTextClip( probe ).y2, 50 );
		{
			CClipStore viewport( &visitor, CTRect<float>( 0, 0, 100, 30 ), true );
			EXPECT_FLOAT_EQ( visitor.GetTextClip( probe ).y2, 200 );
			CClipStore contentPanel( &visitor, CTRect<float>( 0, 0, 100, 120 ) );
			EXPECT_FLOAT_EQ( visitor.GetTextClip( probe ).y2, 120 );
		}
		EXPECT_FLOAT_EQ( visitor.GetTextClip( probe ).y2, 50 );
	}
	EXPECT_FLOAT_EQ( visitor.GetTextClip( probe ).y2, 200 );
}
