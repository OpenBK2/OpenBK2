#include "FontRaster.h"
#include "FontFace.h"

#include <algorithm>

namespace NFontRaster
{

namespace
{

// Lays the glyphs out in rows, left to right, nPadding apart and nPadding from
// the atlas edge. Returns the height used, or -1 if a glyph is wider than the
// atlas. Positions are written only when pGlyphs is not null, so the same walk
// both sizes the atlas and fills it.
int PackRows( const std::vector<SGlyphBitmap> &rendered, const int nCellHeight, const int nPadding, const int nAtlasWidth,
	std::vector<SGlyph> *pGlyphs )
{
	int x = nPadding, y = nPadding;
	for ( size_t i = 0; i < rendered.size(); ++i )
	{
		const SGlyphBitmap &glyph = rendered[i];
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
	std::unique_ptr<CFace> pFace = CFace::OpenFile( options.szFontFile, options.nFaceIndex, pszError );
	if ( pFace == nullptr || !pFace->Fit( options, codePoints, pszError ) )
	{
		return false;
	}
	const SFaceMetrics &metrics = pFace->GetMetrics();
	pResult->nCellHeight = metrics.nCellHeight;
	pResult->nAscent = metrics.nAscent;
	pResult->nDescent = metrics.nDescent;
	pResult->nExternalLeading = metrics.nExternalLeading;
	pResult->nAveCharWidth = metrics.nAveCharWidth;
	pResult->nMaxCharWidth = metrics.nMaxCharWidth;
	pResult->fPixelsPerEm = metrics.fPixelsPerEm;

	std::vector<SGlyphBitmap> rendered( codePoints.size() );
	for ( size_t i = 0; i < codePoints.size(); ++i )
	{
		if ( !pFace->RenderGlyph( codePoints[i], &rendered[i], pszError ) )
		{
			return false;
		}
		if ( rendered[i].bMissing )
		{
			pResult->missing.push_back( codePoints[i] );
		}
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
		const SGlyphBitmap &glyph = rendered[i];
		const SGlyph &placed = pResult->glyphs[i];
		const int nFirstRow = metrics.nAscent - glyph.nTop;
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

	// every ordered pair of characters the face has, in the order asked for
	if ( pFace->HasKerning() )
	{
		for ( const SGlyphBitmap &left : rendered )
		{
			if ( left.bMissing )
			{
				continue;
			}
			for ( const SGlyphBitmap &right : rendered )
			{
				if ( right.bMissing )
				{
					continue;
				}
				const int nAmount = pFace->GetKerning( left.nCodePoint, right.nCodePoint );
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
