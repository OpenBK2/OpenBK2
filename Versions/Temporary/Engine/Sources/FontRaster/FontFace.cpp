#include "FontFace.h"
#include "GposKerning.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H

#include <algorithm>
#include <cmath>

namespace NFontRaster
{

namespace
{

std::string DescribeError( const char *pszWhat, const FT_Error nError )
{
	return std::string( pszWhat ) + " failed with FreeType error " + std::to_string( nError );
}

// The design metrics GDI builds TEXTMETRIC from, in font units
struct SDesignMetrics
{
	int nUnitsPerEm = 0;
	int nWinAscent = 0;
	int nWinDescent = 0;
	int nExternalLeading = 0;
	int nAveCharWidth = 0;
	int nMaxAdvance = 0;
};

bool ReadDesignMetrics( const FT_Face pFace, SDesignMetrics *pMetrics, std::string *pszError )
{
	const TT_HoriHeader *pHhea = static_cast<const TT_HoriHeader*>( FT_Get_Sfnt_Table( pFace, FT_SFNT_HHEA ) );
	if ( pHhea == nullptr || pFace->units_per_EM == 0 )
	{
		*pszError = "the font has no hhea table, so it is not TrueType or OpenType";
		return false;
	}
	const TT_OS2 *pOS2 = static_cast<const TT_OS2*>( FT_Get_Sfnt_Table( pFace, FT_SFNT_OS2 ) );
	pMetrics->nUnitsPerEm = pFace->units_per_EM;
	// GDI sizes a TrueType font by usWinAscent + usWinDescent, the extent the
	// font promises no glyph exceeds, rather than by the typographic or hhea
	// values. Without an OS/2 table, which only very old Mac fonts lack, hhea is
	// the nearest thing.
	if ( pOS2 != nullptr && pOS2->version != 0xFFFF )
	{
		pMetrics->nWinAscent = pOS2->usWinAscent;
		pMetrics->nWinDescent = pOS2->usWinDescent;
		pMetrics->nAveCharWidth = pOS2->xAvgCharWidth;
	}
	else
	{
		pMetrics->nWinAscent = pHhea->Ascender;
		pMetrics->nWinDescent = -pHhea->Descender;
	}
	// GDI's tmExternalLeading: the hhea line gap, less whatever part of it the
	// win extent already covers beyond the hhea extent. Arial comes out at one
	// pixel at 28, the same as GDI reports.
	const int nWinExtent = pMetrics->nWinAscent + pMetrics->nWinDescent;
	const int nHheaExtent = pHhea->Ascender - pHhea->Descender;
	pMetrics->nExternalLeading = std::max( 0, pHhea->Line_Gap - ( nWinExtent - nHheaExtent ) );
	pMetrics->nMaxAdvance = pHhea->advance_Width_Max;
	if ( nWinExtent <= 0 )
	{
		*pszError = "the font reports no vertical extent";
		return false;
	}
	return true;
}

// CELL_INK's extent: the highest and lowest point of any of codePoints'
// outlines, in font units. Replaces the win ascent and descent, and zeroes the
// external leading, which is defined against the win extent and means nothing
// once the cell no longer is. A code point with no glyph adds nothing, since
// what it gets is .notdef, which fits any font's own metrics.
bool MeasureInk( const FT_Face pFace, const std::vector<uint32_t> &codePoints, SDesignMetrics *pMetrics, std::string *pszError )
{
	FT_Pos nTop = 0, nBottom = 0;
	for ( const uint32_t nCodePoint : codePoints )
	{
		const FT_UInt nGlyphIndex = FT_Get_Char_Index( pFace, nCodePoint );
		if ( nGlyphIndex == 0 )
		{
			continue;
		}
		const FT_Error nError = FT_Load_Glyph( pFace, nGlyphIndex, FT_LOAD_NO_SCALE );
		if ( nError != 0 )
		{
			*pszError = DescribeError( "FT_Load_Glyph( FT_LOAD_NO_SCALE )", nError );
			return false;
		}
		if ( pFace->glyph->format != FT_GLYPH_FORMAT_OUTLINE || pFace->glyph->outline.n_points == 0 )
		{
			continue;
		}
		FT_BBox box;
		FT_Outline_Get_CBox( &pFace->glyph->outline, &box );
		nTop = std::max( nTop, box.yMax );
		nBottom = std::min( nBottom, box.yMin );
	}
	if ( nTop - nBottom <= 0 )
	{
		*pszError = "none of the characters has any ink to size the cell by";
		return false;
	}
	pMetrics->nWinAscent = static_cast<int>( nTop );
	pMetrics->nWinDescent = static_cast<int>( -nBottom );
	pMetrics->nExternalLeading = 0;
	return true;
}

FT_Int32 LoadFlags( const SOptions &options )
{
	if ( !options.bAntialias )
	{
		// one bit per pixel wants the hinting made for it
		return FT_LOAD_TARGET_MONO;
	}
	switch ( options.eHinting )
	{
	case HINTING_NONE:
		return FT_LOAD_NO_HINTING;
	case HINTING_NORMAL:
		return FT_LOAD_TARGET_NORMAL;
	case HINTING_LIGHT:
	default:
		return FT_LOAD_TARGET_LIGHT;
	}
}

}

struct CFace::SImpl
{
	FT_Library pLibrary = nullptr;
	FT_Face pFace = nullptr;
	// the bytes of a face opened from memory, which FreeType reads from for as
	// long as the face is open
	std::shared_ptr<const std::vector<uint8_t>> data;
	SOptions options;
	SFaceMetrics metrics;
	std::vector<uint8_t> gammaTable;
	bool bFitted = false;
	// GPOS pair kerning, read only for a face with no legacy kern table
	std::unique_ptr<CGposKerning> pGpos;

	~SImpl()
	{
		if ( pFace != nullptr )
		{
			FT_Done_Face( pFace );
		}
		if ( pLibrary != nullptr )
		{
			FT_Done_FreeType( pLibrary );
		}
	}

	// Selects the character map. A symbol font has no Unicode cmap, only the
	// Microsoft symbol one, whose codes are U+F000 + byte; FontGen translates
	// SYMBOL_CHARSET to exactly those, so the same code points work either way.
	bool SelectCharmap( std::string *pszError )
	{
		const FT_Error nError = FT_Select_Charmap( pFace, FT_ENCODING_UNICODE );
		if ( nError != 0 && FT_Select_Charmap( pFace, FT_ENCODING_MS_SYMBOL ) != 0 )
		{
			*pszError = DescribeError( "FT_Select_Charmap( FT_ENCODING_UNICODE )", nError );
			return false;
		}
		return true;
	}

	// Reads the GPOS kerning of a face that has no kern table. The kern table,
	// where there is one, stays the only source, as it was for GDI, so that a
	// face FontGen baked before bakes the same.
	void ReadGpos()
	{
		if ( FT_HAS_KERNING( pFace ) )
			return;
		FT_ULong nLength = 0;
		if ( FT_Load_Sfnt_Table( pFace, TTAG_GPOS, 0, nullptr, &nLength ) != 0 || nLength == 0 )
			return;
		std::vector<uint8_t> table( nLength );
		if ( FT_Load_Sfnt_Table( pFace, TTAG_GPOS, 0, table.data(), &nLength ) != 0 )
			return;
		std::unique_ptr<CGposKerning> pKerning( new CGposKerning( std::move( table ) ) );
		if ( !pKerning->IsEmpty() )
			pGpos = std::move( pKerning );
	}
};

CFace::CFace() : pImpl( new SImpl ) {}
CFace::~CFace() = default;

std::unique_ptr<CFace> CFace::OpenFile( const std::string &szFile, int nFaceIndex, std::string *pszError )
{
	std::unique_ptr<CFace> pFace( new CFace );
	FT_Error nError = FT_Init_FreeType( &pFace->pImpl->pLibrary );
	if ( nError != 0 )
	{
		*pszError = DescribeError( "FT_Init_FreeType", nError );
		return nullptr;
	}
	nError = FT_New_Face( pFace->pImpl->pLibrary, szFile.c_str(), nFaceIndex, &pFace->pImpl->pFace );
	if ( nError != 0 )
	{
		*pszError = DescribeError( ( "FT_New_Face on \"" + szFile + "\"" ).c_str(), nError );
		return nullptr;
	}
	if ( !pFace->pImpl->SelectCharmap( pszError ) )
		return nullptr;
	pFace->pImpl->ReadGpos();
	return pFace;
}

std::unique_ptr<CFace> CFace::OpenMemory( std::shared_ptr<const std::vector<uint8_t>> data, int nFaceIndex, std::string *pszError )
{
	if ( data == nullptr || data->empty() )
	{
		*pszError = "no font data";
		return nullptr;
	}
	std::unique_ptr<CFace> pFace( new CFace );
	FT_Error nError = FT_Init_FreeType( &pFace->pImpl->pLibrary );
	if ( nError != 0 )
	{
		*pszError = DescribeError( "FT_Init_FreeType", nError );
		return nullptr;
	}
	pFace->pImpl->data = std::move( data );
	nError = FT_New_Memory_Face( pFace->pImpl->pLibrary, pFace->pImpl->data->data(),
		static_cast<FT_Long>( pFace->pImpl->data->size() ), nFaceIndex, &pFace->pImpl->pFace );
	if ( nError != 0 )
	{
		*pszError = DescribeError( "FT_New_Memory_Face", nError );
		return nullptr;
	}
	if ( !pFace->pImpl->SelectCharmap( pszError ) )
		return nullptr;
	pFace->pImpl->ReadGpos();
	return pFace;
}

bool CFace::Fit( const SOptions &options, const std::vector<uint32_t> &sizingCodePoints, std::string *pszError )
{
	SImpl &impl = *pImpl;
	if ( options.nCellHeight <= 0 )
	{
		*pszError = "the cell height must be positive";
		return false;
	}
	impl.options = options;
	impl.metrics = SFaceMetrics();
	impl.gammaTable.resize( 256 );
	for ( int i = 0; i < 256; ++i )
	{
		const double fGamma = options.fGamma > 0 ? options.fGamma : 1.0;
		impl.gammaTable[i] = static_cast<uint8_t>( std::lround( 255.0 * std::pow( i / 255.0, 1.0 / fGamma ) ) );
	}
	SDesignMetrics design;
	if ( !ReadDesignMetrics( impl.pFace, &design, pszError ) )
	{
		return false;
	}
	if ( options.eCellMetrics == CELL_INK && !MeasureInk( impl.pFace, sizingCodePoints, &design, pszError ) )
	{
		return false;
	}
	// The em size that makes the extent exactly nCellHeight pixels. GDI has to
	// round this to a whole pixel, which is why it cannot produce every cell
	// height; FreeType takes it in 26.6 fixed point. That is honoured even for
	// fonts whose head table asks for integer ppem, which all of Windows' do:
	// measured on Impact, 64.769 asked for and 64.766 applied.
	double fPixelsPerUnit = static_cast<double>( options.nCellHeight ) / ( design.nWinAscent + design.nWinDescent );
	impl.metrics.nCellHeight = options.nCellHeight;
	impl.bFitted = true;

	// Once for CELL_WIN. For CELL_INK the rendered ink decides where the
	// baseline goes: hinting moves outlines by up to a pixel, so ink sized to fit
	// in font units can still come out a row too tall. The baseline then moves as
	// little as fits it, and only when the rendered ink is taller than the cell
	// itself is the em shrunk to match and the sizing set rendered again.
	std::vector<SGlyphBitmap> sizing;
	for ( int nAttempt = 0; ; ++nAttempt )
	{
		const double fPixelsPerEm = fPixelsPerUnit * design.nUnitsPerEm;
		const FT_Error nError = FT_Set_Char_Size( impl.pFace, 0, static_cast<FT_F26Dot6>( std::lround( fPixelsPerEm * 64.0 ) ), 72, 72 );
		if ( nError != 0 )
		{
			*pszError = DescribeError( "FT_Set_Char_Size", nError );
			return false;
		}
		impl.metrics.fPixelsPerEm = fPixelsPerEm;
		// Ascent rounds on its own and descent takes the rest, so the two always
		// sum to the cell exactly
		impl.metrics.nAscent = static_cast<int>( std::lround( design.nWinAscent * fPixelsPerUnit ) );
		// the sizing set is rendered only when something needs it
		const bool bNeedRendered = options.eCellMetrics == CELL_INK || design.nAveCharWidth <= 0;
		if ( !bNeedRendered )
		{
			break;
		}
		sizing.assign( sizingCodePoints.size(), SGlyphBitmap() );
		for ( size_t i = 0; i < sizingCodePoints.size(); ++i )
		{
			if ( !RenderGlyph( sizingCodePoints[i], &sizing[i], pszError ) )
			{
				return false;
			}
		}
		if ( options.eCellMetrics != CELL_INK )
		{
			break;
		}
		// rows above the baseline and below it that the ink really uses
		int nAbove = 0, nBelow = 0;
		for ( const SGlyphBitmap &glyph : sizing )
		{
			if ( glyph.nRows > 0 )
			{
				nAbove = std::max( nAbove, glyph.nTop );
				nBelow = std::max( nBelow, glyph.nRows - glyph.nTop );
			}
		}
		if ( nAbove + nBelow <= options.nCellHeight )
		{
			impl.metrics.nAscent = std::clamp( impl.metrics.nAscent, nAbove, options.nCellHeight - nBelow );
			break;
		}
		if ( nAttempt == 3 )
		{
			// what still does not fit is clipped wherever the glyphs are placed
			break;
		}
		fPixelsPerUnit *= static_cast<double>( options.nCellHeight ) / ( nAbove + nBelow );
	}
	impl.metrics.nDescent = options.nCellHeight - impl.metrics.nAscent;
	impl.metrics.nExternalLeading = static_cast<int>( std::lround( design.nExternalLeading * fPixelsPerUnit ) );
	impl.metrics.nMaxCharWidth = static_cast<int>( std::lround( design.nMaxAdvance * fPixelsPerUnit ) );
	// Without xAvgCharWidth, the mean advance of the sizing set is as close as
	// GDI's own fallback gets
	if ( design.nAveCharWidth > 0 )
	{
		impl.metrics.nAveCharWidth = static_cast<int>( std::lround( design.nAveCharWidth * fPixelsPerUnit ) );
	}
	else if ( !sizing.empty() )
	{
		long long nTotal = 0;
		for ( const SGlyphBitmap &glyph : sizing )
		{
			nTotal += glyph.nA + glyph.nB + glyph.nC;
		}
		impl.metrics.nAveCharWidth = static_cast<int>( nTotal / static_cast<long long>( sizing.size() ) );
	}
	return true;
}

const SFaceMetrics &CFace::GetMetrics() const
{
	return pImpl->metrics;
}

bool CFace::HasGlyph( uint32_t nCodePoint ) const
{
	return FT_Get_Char_Index( pImpl->pFace, nCodePoint ) != 0;
}

bool CFace::RenderGlyph( uint32_t nCodePoint, SGlyphBitmap *pGlyph, std::string *pszError )
{
	SImpl &impl = *pImpl;
	*pGlyph = SGlyphBitmap();
	if ( !impl.bFitted )
	{
		*pszError = "the face has not been fitted to a size";
		return false;
	}
	pGlyph->nCodePoint = nCodePoint;
	const FT_UInt nGlyphIndex = FT_Get_Char_Index( impl.pFace, nCodePoint );
	// Index 0 is the font's .notdef glyph, the box GDI draws for a character
	// the font does not have, so a missing code point still gets a glyph.
	pGlyph->bMissing = ( nGlyphIndex == 0 );
	FT_Error nError = FT_Load_Glyph( impl.pFace, nGlyphIndex, LoadFlags( impl.options ) );
	if ( nError != 0 )
	{
		*pszError = DescribeError( "FT_Load_Glyph", nError );
		return false;
	}
	FT_GlyphSlot pSlot = impl.pFace->glyph;
	nError = FT_Render_Glyph( pSlot, impl.options.bAntialias ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO );
	if ( nError != 0 )
	{
		*pszError = DescribeError( "FT_Render_Glyph", nError );
		return false;
	}
	// Whole pixels, as GDI's ABC widths are. Hinted advances already are;
	// unhinted ones are rounded, and the linear advance is used for them because
	// it is the unrounded design value rather than one FreeType has already
	// adjusted.
	const int nAdvance = impl.options.eHinting == HINTING_NONE && impl.options.bAntialias ?
		static_cast<int>( std::lround( pSlot->linearHoriAdvance / 65536.0 ) ) :
		static_cast<int>( std::lround( pSlot->advance.x / 64.0 ) );
	const FT_Bitmap &bitmap = pSlot->bitmap;
	if ( bitmap.width == 0 || bitmap.rows == 0 )
	{
		// a blank glyph such as the space is all advance
		pGlyph->nC = nAdvance;
		return true;
	}
	pGlyph->nA = pSlot->bitmap_left;
	pGlyph->nB = static_cast<int>( bitmap.width );
	pGlyph->nC = nAdvance - pGlyph->nA - pGlyph->nB;
	pGlyph->nTop = pSlot->bitmap_top;
	pGlyph->nRows = static_cast<int>( bitmap.rows );
	pGlyph->coverage.assign( static_cast<size_t>( pGlyph->nB ) * pGlyph->nRows, 0 );
	// pitch is signed: negative means the rows are stored bottom up
	const int nPitch = bitmap.pitch;
	for ( int y = 0; y < pGlyph->nRows; ++y )
	{
		const unsigned char *pRow = nPitch >= 0 ?
			bitmap.buffer + static_cast<ptrdiff_t>( y ) * nPitch :
			bitmap.buffer + static_cast<ptrdiff_t>( pGlyph->nRows - 1 - y ) * -nPitch;
		uint8_t *pDst = &pGlyph->coverage[static_cast<size_t>( y ) * pGlyph->nB];
		for ( int x = 0; x < pGlyph->nB; ++x )
		{
			if ( bitmap.pixel_mode == FT_PIXEL_MODE_MONO )
			{
				pDst[x] = ( pRow[x >> 3] & ( 0x80 >> ( x & 7 ) ) ) != 0 ? 255 : 0;
			}
			else
			{
				pDst[x] = impl.gammaTable[pRow[x]];
			}
		}
	}
	return true;
}

bool CFace::HasKerning() const
{
	return FT_HAS_KERNING( pImpl->pFace ) || pImpl->pGpos != nullptr;
}

int CFace::GetKerning( uint32_t nLeft, uint32_t nRight ) const
{
	const FT_UInt nLeftIndex = FT_Get_Char_Index( pImpl->pFace, nLeft );
	const FT_UInt nRightIndex = FT_Get_Char_Index( pImpl->pFace, nRight );
	if ( nLeftIndex == 0 || nRightIndex == 0 )
	{
		return 0;
	}
	// Without a kern table, the pair kerning in GPOS, which FreeType does not
	// read: font units scaled to pixels at the fitted size and rounded to whole
	// pixels, as FreeType rounds its own
	if ( !FT_HAS_KERNING( pImpl->pFace ) )
	{
		if ( pImpl->pGpos == nullptr || nLeftIndex > 0xFFFF || nRightIndex > 0xFFFF )
		{
			return 0;
		}
		const int nUnits = pImpl->pGpos->GetAdjustment( static_cast<uint16_t>( nLeftIndex ), static_cast<uint16_t>( nRightIndex ) );
		const FT_Pos nScaled = FT_MulFix( nUnits, pImpl->pFace->size->metrics.x_scale );
		return static_cast<int>( std::lround( nScaled / 64.0 ) );
	}
	// Kerning from the font's kern table, which is also all GDI's
	// GetKerningPairs reads. Grid fitted to whole pixels when hinting, as GDI's
	// are.
	const FT_UInt nMode = pImpl->options.eHinting == HINTING_NONE ? FT_KERNING_UNFITTED : FT_KERNING_DEFAULT;
	FT_Vector delta = { 0, 0 };
	if ( FT_Get_Kerning( pImpl->pFace, nLeftIndex, nRightIndex, nMode, &delta ) != 0 )
	{
		return 0;
	}
	return static_cast<int>( std::lround( delta.x / 64.0 ) );
}

}
