#include "FontRaster.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace NFontRaster
{

namespace
{

// FreeType handles released on every path out of Rasterise
struct SLibrary
{
	FT_Library pLibrary = nullptr;
	~SLibrary() { if ( pLibrary != nullptr ) { FT_Done_FreeType( pLibrary ); } }
};

struct SFace
{
	FT_Face pFace = nullptr;
	~SFace() { if ( pFace != nullptr ) { FT_Done_Face( pFace ); } }
};

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

// A rendered glyph before it is placed: its ink as coverage, and where that ink
// sits relative to the pen and the baseline
struct SRendered
{
	uint32_t nCodePoint = 0;
	FT_UInt nGlyphIndex = 0;
	int nA = 0, nB = 0, nC = 0;
	int nTop = 0;										// rows from the ink's top down to the baseline
	int nRows = 0;
	std::vector<uint8_t> coverage;		// nB * nRows
};

bool RenderGlyph( const FT_Face pFace, const SOptions &options, const uint32_t nCodePoint, const std::vector<uint8_t> &gammaTable,
	SRendered *pGlyph, bool *pbMissing, std::string *pszError )
{
	pGlyph->nCodePoint = nCodePoint;
	pGlyph->nGlyphIndex = FT_Get_Char_Index( pFace, nCodePoint );
	// Index 0 is the font's .notdef glyph, the box GDI draws for a character
	// the font does not have, so a missing code point still gets a glyph.
	*pbMissing = ( pGlyph->nGlyphIndex == 0 );
	FT_Error nError = FT_Load_Glyph( pFace, pGlyph->nGlyphIndex, LoadFlags( options ) );
	if ( nError != 0 )
	{
		*pszError = DescribeError( "FT_Load_Glyph", nError );
		return false;
	}
	FT_GlyphSlot pSlot = pFace->glyph;
	nError = FT_Render_Glyph( pSlot, options.bAntialias ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO );
	if ( nError != 0 )
	{
		*pszError = DescribeError( "FT_Render_Glyph", nError );
		return false;
	}
	// Whole pixels, as GDI's ABC widths are. Hinted advances already are;
	// unhinted ones are rounded, and the linear advance is used for them because
	// it is the unrounded design value rather than one FreeType has already
	// adjusted.
	const int nAdvance = options.eHinting == HINTING_NONE && options.bAntialias ?
		static_cast<int>( std::lround( pSlot->linearHoriAdvance / 65536.0 ) ) :
		static_cast<int>( std::lround( pSlot->advance.x / 64.0 ) );
	const FT_Bitmap &bitmap = pSlot->bitmap;
	if ( bitmap.width == 0 || bitmap.rows == 0 )
	{
		// a blank glyph such as the space is all advance
		pGlyph->nA = 0;
		pGlyph->nB = 0;
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
				pDst[x] = gammaTable[pRow[x]];
			}
		}
	}
	return true;
}

// Lays the glyphs out in rows, left to right, nPadding apart and nPadding from
// the atlas edge. Returns the height used, or -1 if a glyph is wider than the
// atlas. Positions are written only when pGlyphs is not null, so the same walk
// both sizes the atlas and fills it.
int PackRows( const std::vector<SRendered> &rendered, const int nCellHeight, const int nPadding, const int nAtlasWidth,
	std::vector<SGlyph> *pGlyphs )
{
	int x = nPadding, y = nPadding;
	for ( size_t i = 0; i < rendered.size(); ++i )
	{
		const SRendered &glyph = rendered[i];
		const int nWidth = glyph.nB + std::max( glyph.nC, 0 );
		if ( nWidth + 2 * nPadding > nAtlasWidth )
		{
			return -1;
		}
		if ( x + nWidth + nPadding > nAtlasWidth )
		{
			x = nPadding;
			y += nCellHeight + nPadding;
		}
		if ( pGlyphs != nullptr )
		{
			SGlyph &out = ( *pGlyphs )[i];
			out.nCodePoint = glyph.nCodePoint;
			out.x1 = x;
			out.y1 = y;
			out.x2 = x + nWidth;
			out.y2 = y + nCellHeight;
			out.nA = glyph.nA;
			out.nBC = glyph.nB + glyph.nC;
			out.nWidth = nWidth;
		}
		x += nWidth + nPadding;
	}
	return y + nCellHeight + nPadding;
}

int NextPowerOfTwo( const int n )
{
	int nResult = 1;
	while ( nResult < n )
	{
		nResult <<= 1;
	}
	return nResult;
}

}

bool Rasterise( const SOptions &options, const std::vector<uint32_t> &codePoints, SFont *pResult, std::string *pszError )
{
	*pResult = SFont();
	if ( options.nCellHeight <= 0 )
	{
		*pszError = "the cell height must be positive";
		return false;
	}
	SLibrary library;
	FT_Error nError = FT_Init_FreeType( &library.pLibrary );
	if ( nError != 0 )
	{
		*pszError = DescribeError( "FT_Init_FreeType", nError );
		return false;
	}
	SFace face;
	nError = FT_New_Face( library.pLibrary, options.szFontFile.c_str(), options.nFaceIndex, &face.pFace );
	if ( nError != 0 )
	{
		*pszError = DescribeError( ( "FT_New_Face on \"" + options.szFontFile + "\"" ).c_str(), nError );
		return false;
	}
	// A symbol font has no Unicode cmap, only the Microsoft symbol one, whose
	// codes are U+F000 + byte; FontGen translates SYMBOL_CHARSET to exactly
	// those, so the same code points work through either.
	nError = FT_Select_Charmap( face.pFace, FT_ENCODING_UNICODE );
	if ( nError != 0 && FT_Select_Charmap( face.pFace, FT_ENCODING_MS_SYMBOL ) != 0 )
	{
		*pszError = DescribeError( "FT_Select_Charmap( FT_ENCODING_UNICODE )", nError );
		return false;
	}
	SDesignMetrics design;
	if ( !ReadDesignMetrics( face.pFace, &design, pszError ) )
	{
		return false;
	}
	if ( options.eCellMetrics == CELL_INK && !MeasureInk( face.pFace, codePoints, &design, pszError ) )
	{
		return false;
	}
	// The em size that makes the win extent exactly nCellHeight pixels. GDI has
	// to round this to a whole pixel, which is why it cannot produce every cell
	// height; FreeType takes it in 26.6 fixed point. That is honoured even for
	// fonts whose head table asks for integer ppem, which all of Windows' do:
	// measured on Impact, 64.769 asked for and 64.766 applied.
	double fPixelsPerUnit = static_cast<double>( options.nCellHeight ) / ( design.nWinAscent + design.nWinDescent );
	pResult->nCellHeight = options.nCellHeight;

	std::vector<uint8_t> gammaTable( 256 );
	for ( int i = 0; i < 256; ++i )
	{
		const double fGamma = options.fGamma > 0 ? options.fGamma : 1.0;
		gammaTable[i] = static_cast<uint8_t>( std::lround( 255.0 * std::pow( i / 255.0, 1.0 / fGamma ) ) );
	}

	// Rasterised once for CELL_WIN. For CELL_INK the rendered ink decides where
	// the baseline goes: hinting moves outlines by up to a pixel, so ink sized to
	// fit in font units can still come out a row too tall. The baseline then
	// moves as little as fits it, and only when the rendered ink is taller than
	// the cell itself is the em shrunk to match and everything rendered again.
	std::vector<SRendered> rendered( codePoints.size() );
	for ( int nAttempt = 0; ; ++nAttempt )
	{
		const double fPixelsPerEm = fPixelsPerUnit * design.nUnitsPerEm;
		nError = FT_Set_Char_Size( face.pFace, 0, static_cast<FT_F26Dot6>( std::lround( fPixelsPerEm * 64.0 ) ), 72, 72 );
		if ( nError != 0 )
		{
			*pszError = DescribeError( "FT_Set_Char_Size", nError );
			return false;
		}
		pResult->fPixelsPerEm = fPixelsPerEm;
		// Ascent rounds on its own and descent takes the rest, so the two always
		// sum to the cell exactly
		pResult->nAscent = static_cast<int>( std::lround( design.nWinAscent * fPixelsPerUnit ) );
		pResult->missing.clear();
		for ( size_t i = 0; i < codePoints.size(); ++i )
		{
			bool bMissing = false;
			if ( !RenderGlyph( face.pFace, options, codePoints[i], gammaTable, &rendered[i], &bMissing, pszError ) )
			{
				return false;
			}
			if ( bMissing )
			{
				pResult->missing.push_back( codePoints[i] );
			}
		}
		if ( options.eCellMetrics != CELL_INK )
		{
			break;
		}
		// rows above the baseline and below it that the ink really uses
		int nAbove = 0, nBelow = 0;
		for ( const SRendered &glyph : rendered )
		{
			if ( glyph.nRows > 0 )
			{
				nAbove = std::max( nAbove, glyph.nTop );
				nBelow = std::max( nBelow, glyph.nRows - glyph.nTop );
			}
		}
		if ( nAbove + nBelow <= options.nCellHeight )
		{
			pResult->nAscent = std::clamp( pResult->nAscent, nAbove, options.nCellHeight - nBelow );
			break;
		}
		if ( nAttempt == 3 )
		{
			// what still does not fit is clipped and reported below
			break;
		}
		fPixelsPerUnit *= static_cast<double>( options.nCellHeight ) / ( nAbove + nBelow );
	}
	pResult->nDescent = options.nCellHeight - pResult->nAscent;
	pResult->nExternalLeading = static_cast<int>( std::lround( design.nExternalLeading * fPixelsPerUnit ) );
	pResult->nMaxCharWidth = static_cast<int>( std::lround( design.nMaxAdvance * fPixelsPerUnit ) );
	// Without xAvgCharWidth, the mean advance of what was rendered is as close
	// as GDI's own fallback gets
	if ( design.nAveCharWidth > 0 )
	{
		pResult->nAveCharWidth = static_cast<int>( std::lround( design.nAveCharWidth * fPixelsPerUnit ) );
	}
	else if ( !rendered.empty() )
	{
		long long nTotal = 0;
		for ( const SRendered &glyph : rendered )
		{
			nTotal += glyph.nA + glyph.nB + glyph.nC;
		}
		pResult->nAveCharWidth = static_cast<int>( nTotal / static_cast<long long>( rendered.size() ) );
	}

	// The smallest power of two atlas that holds every cell, square or twice as
	// wide as tall, the two shapes FontGen's own estimate chose between. Among
	// equal areas the squarer one wins: every width here is a power of two, so
	// 512x512 and 4096x64 tie, and only one of them is a sensible texture.
	// Should no square or 2:1 atlas fit, which only a very small font could
	// cause, the least area of any shape is taken instead.
	const int nMaxAtlas = 4096;
	int nBestWidth = 0, nBestHeight = 0;
	for ( int nPass = 0; nPass < 2 && nBestWidth == 0; ++nPass )
	{
		const bool bShapeMatters = ( nPass == 0 );
		for ( int nWidth = 64; nWidth <= nMaxAtlas; nWidth <<= 1 )
		{
			const int nUsed = PackRows( rendered, options.nCellHeight, options.nPadding, nWidth, nullptr );
			if ( nUsed < 0 )
			{
				continue;
			}
			const int nHeight = NextPowerOfTwo( nUsed );
			if ( nHeight > nMaxAtlas || ( bShapeMatters && ( nHeight > nWidth || nWidth > 2 * nHeight ) ) )
			{
				continue;
			}
			// widths are tried narrowest first, so a strict comparison keeps the
			// squarer of two equal areas
			if ( nBestWidth == 0 || static_cast<long long>( nWidth ) * nHeight < static_cast<long long>( nBestWidth ) * nBestHeight )
			{
				nBestWidth = nWidth;
				nBestHeight = nHeight;
			}
		}
	}
	if ( nBestWidth == 0 )
	{
		*pszError = "the glyphs do not fit a " + std::to_string( nMaxAtlas ) + " pixel atlas";
		return false;
	}
	pResult->nAtlasWidth = nBestWidth;
	pResult->nAtlasHeight = nBestHeight;
	pResult->glyphs.resize( rendered.size() );
	PackRows( rendered, options.nCellHeight, options.nPadding, nBestWidth, &pResult->glyphs );

	// Copy each glyph's ink into its cell, with the baseline nAscent rows down.
	// Ink above the ascent line or below the descent line has nowhere to go, the
	// same as it had in GDI's cells, and is counted rather than drawn over a
	// neighbour.
	pResult->atlas.assign( static_cast<size_t>( nBestWidth ) * nBestHeight, 0 );
	for ( size_t i = 0; i < rendered.size(); ++i )
	{
		const SRendered &glyph = rendered[i];
		const SGlyph &placed = pResult->glyphs[i];
		const int nFirstRow = pResult->nAscent - glyph.nTop;
		bool bClipped = false;
		for ( int y = 0; y < glyph.nRows; ++y )
		{
			const int nCellRow = nFirstRow + y;
			if ( nCellRow < 0 || nCellRow >= options.nCellHeight )
			{
				bClipped = true;
				continue;
			}
			uint8_t *pDst = &pResult->atlas[static_cast<size_t>( placed.y1 + nCellRow ) * nBestWidth + placed.x1];
			std::copy_n( &glyph.coverage[static_cast<size_t>( y ) * glyph.nB], glyph.nB, pDst );
		}
		if ( bClipped )
		{
			pResult->clipped.push_back( glyph.nCodePoint );
		}
	}

	// Kerning from the font's kern table, which is also all GDI's
	// GetKerningPairs reads; pairs defined only in GPOS are not seen by either.
	// Grid fitted to whole pixels when hinting, as GDI's are.
	if ( FT_HAS_KERNING( face.pFace ) )
	{
		const FT_UInt nMode = options.eHinting == HINTING_NONE ? FT_KERNING_UNFITTED : FT_KERNING_DEFAULT;
		for ( const SRendered &left : rendered )
		{
			if ( left.nGlyphIndex == 0 )
			{
				continue;
			}
			for ( const SRendered &right : rendered )
			{
				if ( right.nGlyphIndex == 0 )
				{
					continue;
				}
				FT_Vector delta = { 0, 0 };
				if ( FT_Get_Kerning( face.pFace, left.nGlyphIndex, right.nGlyphIndex, nMode, &delta ) != 0 )
				{
					continue;
				}
				const int nAmount = static_cast<int>( std::lround( delta.x / 64.0 ) );
				if ( nAmount != 0 )
				{
					SKerningPair pair;
					pair.nLeft = left.nCodePoint;
					pair.nRight = right.nCodePoint;
					pair.nAmount = nAmount;
					pResult->kerns.push_back( pair );
				}
			}
		}
	}
	return true;
}

}
