#include "stdafx.h"

#include "FilePath.h"
#include "FileReaders.h"
#include "FileUtils.h"
#include "VFSOperations.h"
#include "WinCursor.h"

#if !BOOST_OS_WINDOWS
#include <SDL3/SDL.h>

#include <cstdint>
#include <vector>
#endif

namespace NWinCursor
{
#if BOOST_OS_WINDOWS

TCursor LoadCursor( const std::string &szFileName )
{
	const std::string szTempFile( NFile::GetTempFileName() );
	{
		CFileStream file( NVFS::GetMainVFS(), szFileName );
		if ( !file.IsOk() )
			return 0;

		CFileStream tempFile( szTempFile, CFileStream::WIN_CREATE );
		if ( !tempFile.IsOk() )
			return 0;

		file.ReadTo( &tempFile, file.GetSize() );
	}

	HCURSOR hCursor = ::LoadCursorFromFile( szTempFile.c_str() );
	::DeleteFile( szTempFile.c_str() );

	return hCursor;
}

void DestroyCursor( TCursor hCursor )
{
	if ( hCursor != 0 )
	{
		::DestroyCursor( hCursor );
	}
}

#else

namespace
{
// Both container formats below are little-endian whatever the host is, and
// nothing in them is aligned, so every field is assembled a byte at a time
// rather than read through a cast.
uint16_t ReadU16( const uint8_t *pData )
{
	return static_cast<uint16_t>( pData[0] ) | ( static_cast<uint16_t>( pData[1] ) << 8 );
}

uint32_t ReadU32( const uint8_t *pData )
{
	return static_cast<uint32_t>( pData[0] ) | ( static_cast<uint32_t>( pData[1] ) << 8 ) |
	       ( static_cast<uint32_t>( pData[2] ) << 16 ) | ( static_cast<uint32_t>( pData[3] ) << 24 );
}

// A .ani is a RIFF ACON whose "fram" list holds one "icon" chunk per frame, and
// each of those chunks is a complete .cur. Only the first frame is taken: SDL
// has no animated cursor, so animation is a separate piece of work.
bool FindFirstAniFrame( const uint8_t *pData, size_t nSize, size_t *pnOffset )
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

// Decode the first image of a .cur, which is the ICO container with the hotspot
// in the two fields the icon form leaves reserved. The image itself is a DIB:
// a BITMAPINFOHEADER, an optional palette, a bottom-up colour bitmap of double
// the stated height, and then a 1bpp AND mask covering the other half.
SDL_Surface *DecodeCursor( const uint8_t *pData, size_t nSize, size_t nBase, int *pnHotX, int *pnHotY )
{
	if ( nBase + 22 > nSize || ReadU16( pData + nBase ) != 0 )
	{
		return 0;
	}
	const uint16_t nType = ReadU16( pData + nBase + 2 );
	// 2 is a cursor, 1 an icon; an icon has no hotspot, so it lands at 0,0
	if ( ( nType != 1 && nType != 2 ) || ReadU16( pData + nBase + 4 ) < 1 )
	{
		return 0;
	}
	const uint8_t *pEntry = pData + nBase + 6;
	const int nWidth = pEntry[0] != 0 ? pEntry[0] : 256;
	const int nHeight = pEntry[1] != 0 ? pEntry[1] : 256;
	*pnHotX = nType == 2 ? ReadU16( pEntry + 4 ) : 0;
	*pnHotY = nType == 2 ? ReadU16( pEntry + 6 ) : 0;

	const size_t nImage = nBase + ReadU32( pEntry + 12 );
	if ( nImage + 40 > nSize || ReadU32( pData + nImage ) != 40 )
	{
		return 0;
	}
	const uint16_t nBpp = ReadU16( pData + nImage + 14 );
	// no BI_RLE or BI_BITFIELDS: the cursors this reads are all uncompressed
	if ( ReadU32( pData + nImage + 16 ) != 0 )
	{
		return 0;
	}
	uint32_t nPaletteEntries = ReadU32( pData + nImage + 32 );
	if ( nPaletteEntries == 0 && nBpp <= 8 )
	{
		nPaletteEntries = 1u << nBpp;
	}
	if ( nBpp != 8 && nBpp != 24 && nBpp != 32 )
	{
		return 0;
	}

	const uint8_t *pPalette = pData + nImage + 40;
	const size_t nColour = nImage + 40 + nPaletteEntries * 4;
	// DIB rows are padded to four bytes, and run bottom-up
	const size_t nColourStride = ( ( static_cast<size_t>( nWidth ) * nBpp + 31 ) / 32 ) * 4;
	const size_t nMaskStride = ( ( static_cast<size_t>( nWidth ) + 31 ) / 32 ) * 4;
	const size_t nMask = nColour + nColourStride * nHeight;
	if ( nMask + nMaskStride * nHeight > nSize )
	{
		return 0;
	}

	SDL_Surface *pSurface = SDL_CreateSurface( nWidth, nHeight, SDL_PIXELFORMAT_ARGB8888 );
	if ( pSurface == 0 )
	{
		return 0;
	}
	for ( int y = 0; y < nHeight; ++y )
	{
		const uint8_t *pSrc = pData + nColour + nColourStride * ( nHeight - 1 - y );
		const uint8_t *pSrcMask = pData + nMask + nMaskStride * ( nHeight - 1 - y );
		uint32_t *pDst = reinterpret_cast<uint32_t *>( static_cast<uint8_t *>( pSurface->pixels ) + y * pSurface->pitch );
		for ( int x = 0; x < nWidth; ++x )
		{
			uint8_t nRed = 0, nGreen = 0, nBlue = 0;
			if ( nBpp == 8 )
			{
				const uint8_t *pColour = pPalette + static_cast<size_t>( pSrc[x] ) * 4;
				nBlue = pColour[0];
				nGreen = pColour[1];
				nRed = pColour[2];
			}
			else
			{
				const uint8_t *pColour = pSrc + static_cast<size_t>( x ) * ( nBpp / 8 );
				nBlue = pColour[0];
				nGreen = pColour[1];
				nRed = pColour[2];
			}
			// the AND mask is transparency: a set bit leaves the screen alone.
			// It is used even at 32bpp, because these files carry no alpha.
			const bool bTransparent = ( pSrcMask[x >> 3] & ( 0x80 >> ( x & 7 ) ) ) != 0;
			const uint32_t nAlpha = bTransparent ? 0u : 0xFFu;
			pDst[x] = ( nAlpha << 24 ) | ( static_cast<uint32_t>( nRed ) << 16 ) |
			          ( static_cast<uint32_t>( nGreen ) << 8 ) | nBlue;
		}
	}
	return pSurface;
}
}

TCursor LoadCursor( const std::string &szFileName )
{
	CFileStream file( NVFS::GetMainVFS(), szFileName );
	if ( !file.IsOk() )
	{
		return 0;
	}
	const int nSize = file.GetSize();
	if ( nSize <= 0 )
	{
		return 0;
	}
	std::vector<uint8_t> buffer( nSize );
	file.Read( &buffer[0], nSize );

	// .ani carries its frames as complete .cur images, so both paths meet here
	size_t nBase = 0;
	if ( !FindFirstAniFrame( &buffer[0], buffer.size(), &nBase ) )
	{
		nBase = 0;
	}
	int nHotX = 0, nHotY = 0;
	SDL_Surface *pSurface = DecodeCursor( &buffer[0], buffer.size(), nBase, &nHotX, &nHotY );
	if ( pSurface == 0 )
	{
		return 0;
	}
	SDL_Cursor *pCursor = SDL_CreateColorCursor( pSurface, nHotX, nHotY );
	SDL_DestroySurface( pSurface );
	return pCursor;
}

void DestroyCursor( TCursor hCursor )
{
	if ( hCursor != 0 )
	{
		SDL_DestroyCursor( hCursor );
	}
}

#endif
}
