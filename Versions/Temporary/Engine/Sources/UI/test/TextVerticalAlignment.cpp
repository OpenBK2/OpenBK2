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
	int calls = 0;
	void ClipSet( const CTRect<float>& ) override {}
	void ClipRestore() override {}
	void VisitZClearRect( const CRectLayout&, const CTPoint<float>&, const CTRect<float>&, float ) override {}
	void VisitUIRect( const NDb::STexture*, int, const CRectLayout& ) override {}
	void VisitUIRect( const NDb::STexture*, int, const CVec2*, const NGfx::SPixel8888*, const CTRect<float>& ) override {}
	void VisitUITextureRect( CPtrFuncBase<NGfx::CTexture>*, int, const CRectLayout& ) override {}
	void VisitUIText( IML *text, const CTPoint<float> &origin, const CTRect<float> &window ) override
	{
		position = origin;
		clip = window;
		size = text->GetSize();
		++calls;
	}
};

class TextVerticalAlignment : public testing::Test
{
	NGlobal::CValue oldScale = NGlobal::GetVar( "ui_font_scale", 1.2f );
	CObj<NVFS::IVFS> oldVFS = NVFS::GetMainVFS();
	CObj<NDb::SFont> numeric;
	CObj<NDb::STexture> texture;
protected:
	double capRatio = 0;
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
	}
	void TearDown() override
	{
		NGScene::GetTextLocaleInfo()->ClearAllFonts();
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( 1024, 768 );
		NSingleton::UnRegisterSingleton( IUIInitialization::tidTypeID );
		NGlobal::SetVar( "ui_font_scale", oldScale );
		NVFS::SetMainVFS( oldVFS );
	}
	CWindowTextView *MakeLabel( float width, float height, bool resize = false )
	{
		CPtr<NDb::SWindowTextViewShared> shared = MakeObject<NDb::SWindowTextViewShared>( NDb::SWindowTextViewShared::typeID );
		CPtr<NDb::SWindowTextView> desc = MakeObject<NDb::SWindowTextView>( NDb::SWindowTextView::typeID );
		desc->nClassTypeID = 0x11075B8C;
		desc->pShared = shared;
		desc->bVisible = true;
		desc->bResizeOnTextSet = resize;
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
