#include "stdafx.h"

#include "WinImageFormats.h"

#include <cstring>

namespace NWinImage
{
namespace
{

const size_t DIRECTORY_HEADER_SIZE = 6;
const size_t DIRECTORY_ENTRY_SIZE = 16;
const uint32_t BITMAPINFOHEADER_SIZE = 40;

uint16_t ReadU16( const uint8_t *pData )
{
	return static_cast<uint16_t>( pData[0] ) | ( static_cast<uint16_t>( pData[1] ) << 8 );
}

uint32_t ReadU32( const uint8_t *pData )
{
	return static_cast<uint32_t>( pData[0] ) | ( static_cast<uint32_t>( pData[1] ) << 8 ) |
	       ( static_cast<uint32_t>( pData[2] ) << 16 ) | ( static_cast<uint32_t>( pData[3] ) << 24 );
}

bool IsPalette( int nBpp )
{
	return nBpp == 1 || nBpp == 4 || nBpp == 8;
}

bool IsReadableDepth( int nBpp )
{
	return IsPalette( nBpp ) || nBpp == 24 || nBpp == 32;
}

// One index out of a row packed at 1, 4 or 8 bits per pixel, most significant
// bits first, which is the order the DIB stores them in.
uint8_t ReadPaletteIndex( const uint8_t *pRow, int x, int nBpp )
{
	if ( nBpp == 8 )
	{
		return pRow[x];
	}
	if ( nBpp == 4 )
	{
		const uint8_t nByte = pRow[x >> 1];
		return ( x & 1 ) != 0 ? ( nByte & 0x0F ) : ( nByte >> 4 );
	}
	return ( pRow[x >> 3] >> ( 7 - ( x & 7 ) ) ) & 1;
}

// Everything about one entry that decoding needs, once the offsets have been
// checked against the size of the data.
struct SEntryLayout
{
	int nWidth;
	int nHeight;
	int nBpp;
	int nHotX;
	int nHotY;
	size_t nPalette;
	size_t nColour;
	size_t nMask;
	size_t nColourStride;
	size_t nMaskStride;
};

// Locate entry nIndex. bLayout asks for the full layout, which needs the image
// to be readable; without it only the directory fields are filled in, which is
// what listing needs and which works even for an entry nothing can decode.
bool ReadEntry( SEntryLayout *pLayout, bool *pbSupported, const uint8_t *pData,
                size_t nSize, size_t nBase, int nIndex )
{
	if ( nBase + DIRECTORY_HEADER_SIZE > nSize || ReadU16( pData + nBase ) != 0 )
	{
		return false;
	}
	const uint16_t nType = ReadU16( pData + nBase + 2 );
	// 2 is a cursor, 1 an icon; an icon has no hotspot, so it lands at 0,0
	if ( nType != 1 && nType != 2 )
	{
		return false;
	}
	const uint16_t nCount = ReadU16( pData + nBase + 4 );
	if ( nIndex < 0 || nIndex >= nCount )
	{
		return false;
	}
	const size_t nEntry = nBase + DIRECTORY_HEADER_SIZE + static_cast<size_t>( nIndex ) * DIRECTORY_ENTRY_SIZE;
	if ( nEntry + DIRECTORY_ENTRY_SIZE > nSize )
	{
		return false;
	}
	const uint8_t *pEntry = pData + nEntry;
	// zero in the size byte means 256, which is how the container spells a size
	// that does not fit in a byte
	pLayout->nWidth = pEntry[0] != 0 ? pEntry[0] : 256;
	pLayout->nHeight = pEntry[1] != 0 ? pEntry[1] : 256;
	pLayout->nHotX = nType == 2 ? ReadU16( pEntry + 4 ) : 0;
	pLayout->nHotY = nType == 2 ? ReadU16( pEntry + 6 ) : 0;
	pLayout->nBpp = 0;
	*pbSupported = false;

	const size_t nImage = nBase + ReadU32( pEntry + 12 );
	if ( nImage + BITMAPINFOHEADER_SIZE > nSize )
	{
		return true;
	}
	// Vista and later allow a PNG in place of the bitmap. Reporting it as an
	// entry nothing here reads beats parsing its header as a BITMAPINFOHEADER.
	static const uint8_t PNG_SIGNATURE[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
	if ( memcmp( pData + nImage, PNG_SIGNATURE, sizeof( PNG_SIGNATURE ) ) == 0 )
	{
		return true;
	}
	if ( ReadU32( pData + nImage ) != BITMAPINFOHEADER_SIZE )
	{
		return true;
	}
	const int nBpp = ReadU16( pData + nImage + 14 );
	pLayout->nBpp = nBpp;
	// no BI_RLE or BI_BITFIELDS: the images this reads are all uncompressed
	if ( ReadU32( pData + nImage + 16 ) != 0 || !IsReadableDepth( nBpp ) )
	{
		return true;
	}

	uint32_t nPaletteEntries = ReadU32( pData + nImage + 32 );
	if ( nPaletteEntries == 0 && IsPalette( nBpp ) )
	{
		nPaletteEntries = 1u << nBpp;
	}
	pLayout->nPalette = nImage + BITMAPINFOHEADER_SIZE;
	pLayout->nColour = pLayout->nPalette + static_cast<size_t>( nPaletteEntries ) * 4;
	// DIB rows are padded to four bytes, and run bottom-up
	pLayout->nColourStride = ( ( static_cast<size_t>( pLayout->nWidth ) * nBpp + 31 ) / 32 ) * 4;
	pLayout->nMaskStride = ( ( static_cast<size_t>( pLayout->nWidth ) + 31 ) / 32 ) * 4;
	pLayout->nMask = pLayout->nColour + pLayout->nColourStride * pLayout->nHeight;
	if ( pLayout->nMask + pLayout->nMaskStride * pLayout->nHeight > nSize )
	{
		return true;
	}
	*pbSupported = true;
	return true;
}

}

bool FindFirstAniFrame( size_t *pnOffset, const uint8_t *pData, size_t nSize )
{
	if ( nSize < 12 || memcmp( pData, "RIFF", 4 ) != 0 || memcmp( pData + 8, "ACON", 4 ) != 0 )
	{
		return false;
	}
	size_t nPos = 12;
	while ( nPos + 8 <= nSize )
	{
		const uint32_t nChunk = ReadU32( pData + nPos + 4 );
		if ( memcmp( pData + nPos, "LIST", 4 ) == 0 && nPos + 12 <= nSize &&
		     memcmp( pData + nPos + 8, "fram", 4 ) == 0 )
		{
			size_t nSub = nPos + 12;
			const size_t nListEnd = (std::min)( nSize, nPos + 8 + static_cast<size_t>( nChunk ) );
			while ( nSub + 8 <= nListEnd )
			{
				const uint32_t nSubSize = ReadU32( pData + nSub + 4 );
				if ( memcmp( pData + nSub, "icon", 4 ) == 0 )
				{
					*pnOffset = nSub + 8;
					return true;
				}
				// RIFF chunks are padded to an even length
				nSub += 8 + nSubSize + ( nSubSize & 1 );
			}
			return false;
		}
		nPos += 8 + nChunk + ( nChunk & 1 );
	}
	return false;
}

bool GetImages( std::vector<SImageInfo> *pImages, const uint8_t *pData, size_t nSize, size_t nBase )
{
	pImages->clear();
	if ( nBase + DIRECTORY_HEADER_SIZE > nSize || ReadU16( pData + nBase ) != 0 )
	{
		return false;
	}
	const uint16_t nType = ReadU16( pData + nBase + 2 );
	if ( nType != 1 && nType != 2 )
	{
		return false;
	}
	const uint16_t nCount = ReadU16( pData + nBase + 4 );
	for ( int i = 0; i < nCount; ++i )
	{
		SEntryLayout layout;
		bool bSupported = false;
		if ( !ReadEntry( &layout, &bSupported, pData, nSize, nBase, i ) )
		{
			break;
		}
		SImageInfo info;
		info.nWidth = layout.nWidth;
		info.nHeight = layout.nHeight;
		info.nBpp = layout.nBpp;
		info.nHotX = layout.nHotX;
		info.nHotY = layout.nHotY;
		info.bSupported = bSupported;
		pImages->push_back( info );
	}
	return !pImages->empty();
}

int SelectImage( const std::vector<SImageInfo> &images, int nDesiredSize )
{
	int nBest = -1;
	for ( int i = 0; i < images.size(); ++i )
	{
		if ( !images[i].bSupported )
		{
			continue;
		}
		if ( nBest < 0 )
		{
			nBest = i;
			continue;
		}
		const SImageInfo &best = images[nBest];
		const SImageInfo &candidate = images[i];
		if ( candidate.nWidth == best.nWidth )
		{
			// same size, so the only thing left to prefer is more colour
			if ( candidate.nBpp > best.nBpp )
			{
				nBest = i;
			}
			continue;
		}
		// an exact match ends it
		if ( candidate.nWidth == nDesiredSize )
		{
			nBest = i;
			continue;
		}
		if ( best.nWidth == nDesiredSize )
		{
			continue;
		}
		const bool bBestIsLarger = best.nWidth > nDesiredSize;
		const bool bCandidateIsLarger = candidate.nWidth > nDesiredSize;
		if ( bBestIsLarger != bCandidateIsLarger )
		{
			// shrinking keeps detail, growing invents it, so larger wins
			if ( bCandidateIsLarger )
			{
				nBest = i;
			}
			continue;
		}
		// both on the same side: the closest one
		if ( bCandidateIsLarger ? candidate.nWidth < best.nWidth : candidate.nWidth > best.nWidth )
		{
			nBest = i;
		}
	}
	return nBest;
}

bool DecodeImage( SImage *pResult, const uint8_t *pData, size_t nSize, size_t nBase, int nIndex )
{
	SEntryLayout layout;
	bool bSupported = false;
	if ( !ReadEntry( &layout, &bSupported, pData, nSize, nBase, nIndex ) || !bSupported )
	{
		return false;
	}

	const int nWidth = layout.nWidth;
	const int nHeight = layout.nHeight;
	const uint8_t *pPalette = pData + layout.nPalette;

	// A 32bpp entry may carry a real alpha channel, in which case the AND mask is
	// left zeroed and using it would make the image opaque. There is no flag for
	// which of the two is meant, so the file is asked: any non-zero alpha and the
	// channel is real.
	bool bHasAlpha = false;
	if ( layout.nBpp == 32 )
	{
		for ( int y = 0; y < nHeight && !bHasAlpha; ++y )
		{
			const uint8_t *pRow = pData + layout.nColour + layout.nColourStride * y;
			for ( int x = 0; x < nWidth; ++x )
			{
				if ( pRow[static_cast<size_t>( x ) * 4 + 3] != 0 )
				{
					bHasAlpha = true;
					break;
				}
			}
		}
	}

	pResult->nWidth = nWidth;
	pResult->nHeight = nHeight;
	pResult->nHotX = layout.nHotX;
	pResult->nHotY = layout.nHotY;
	pResult->pixels.resize( static_cast<size_t>( nWidth ) * nHeight );
	for ( int y = 0; y < nHeight; ++y )
	{
		const uint8_t *pSrc = pData + layout.nColour + layout.nColourStride * ( nHeight - 1 - y );
		const uint8_t *pSrcMask = pData + layout.nMask + layout.nMaskStride * ( nHeight - 1 - y );
		uint32_t *pDst = &pResult->pixels[static_cast<size_t>( y ) * nWidth];
		for ( int x = 0; x < nWidth; ++x )
		{
			uint8_t nRed = 0, nGreen = 0, nBlue = 0, nAlpha = 0xFF;
			if ( IsPalette( layout.nBpp ) )
			{
				const uint8_t *pColour = pPalette + static_cast<size_t>( ReadPaletteIndex( pSrc, x, layout.nBpp ) ) * 4;
				nBlue = pColour[0];
				nGreen = pColour[1];
				nRed = pColour[2];
			}
			else
			{
				const uint8_t *pColour = pSrc + static_cast<size_t>( x ) * ( layout.nBpp / 8 );
				nBlue = pColour[0];
				nGreen = pColour[1];
				nRed = pColour[2];
				if ( bHasAlpha )
				{
					nAlpha = pColour[3];
				}
			}
			if ( !bHasAlpha )
			{
				// the AND mask is transparency: a set bit leaves the screen alone
				const bool bTransparent = ( pSrcMask[x >> 3] & ( 0x80 >> ( x & 7 ) ) ) != 0;
				nAlpha = bTransparent ? 0u : 0xFFu;
			}
			pDst[x] = ( static_cast<uint32_t>( nAlpha ) << 24 ) | ( static_cast<uint32_t>( nRed ) << 16 ) |
			          ( static_cast<uint32_t>( nGreen ) << 8 ) | nBlue;
		}
	}
	return true;
}

}
