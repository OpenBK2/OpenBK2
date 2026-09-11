#pragma once

// How much memory a surface of a given format and size takes, and how its rows
// are laid out, which is all a Lock has to get right.
//
// Nothing is drawn, so the contents never matter, but the layout does: the
// engine and D3DX both write through what LockRect hands back, row by row at
// the pitch it names, and D3DX's DDS export reads it back the same way. A
// buffer a byte short is a heap overrun in someone else's code.

#include <d3d9.h>

#include <algorithm>
#include <cstddef>

namespace ND3D9Stub
{
	inline bool IsBlockCompressed( D3DFORMAT eFormat )
	{
		return eFormat == D3DFMT_DXT1 || eFormat == D3DFMT_DXT2 || eFormat == D3DFMT_DXT3 ||
					 eFormat == D3DFMT_DXT4 || eFormat == D3DFMT_DXT5;
	}

	// Bytes per 4x4 block for the compressed formats.
	inline unsigned BlockBytes( D3DFORMAT eFormat )
	{
		return eFormat == D3DFMT_DXT1 ? 8 : 16;
	}

	// Bits per pixel for everything else. What is not listed gets 32, and the
	// caller notes it, so a format the engine uses that is missing here shows
	// up in the trace rather than as an overrun.
	inline unsigned BitsPerPixel( D3DFORMAT eFormat, bool *pbKnown = nullptr )
	{
		if ( pbKnown != nullptr )
		{
			*pbKnown = true;
		}
		switch ( eFormat )
		{
		case D3DFMT_A8: case D3DFMT_L8: case D3DFMT_P8: case D3DFMT_R3G3B2: case D3DFMT_A4L4:
			return 8;
		case D3DFMT_R5G6B5: case D3DFMT_X1R5G5B5: case D3DFMT_A1R5G5B5: case D3DFMT_A4R4G4B4:
		case D3DFMT_X4R4G4B4: case D3DFMT_A8R3G3B2: case D3DFMT_A8P8: case D3DFMT_A8L8: case D3DFMT_V8U8:
		case D3DFMT_L6V5U5: case D3DFMT_L16: case D3DFMT_R16F: case D3DFMT_D16: case D3DFMT_D16_LOCKABLE:
		case D3DFMT_D15S1: case D3DFMT_INDEX16:
			return 16;
		case D3DFMT_R8G8B8:
			return 24;
		case D3DFMT_A8R8G8B8: case D3DFMT_X8R8G8B8: case D3DFMT_A8B8G8R8: case D3DFMT_X8B8G8R8:
		case D3DFMT_A2R10G10B10: case D3DFMT_A2B10G10R10: case D3DFMT_G16R16: case D3DFMT_X8L8V8U8:
		case D3DFMT_Q8W8V8U8: case D3DFMT_V16U16: case D3DFMT_A2W10V10U10: case D3DFMT_R32F:
		case D3DFMT_G16R16F: case D3DFMT_D32: case D3DFMT_D24S8: case D3DFMT_D24X8: case D3DFMT_D24X4S4:
		case D3DFMT_D24FS8: case D3DFMT_D32F_LOCKABLE: case D3DFMT_D32_LOCKABLE: case D3DFMT_INDEX32:
			return 32;
		case D3DFMT_A16B16G16R16: case D3DFMT_Q16W16V16U16: case D3DFMT_A16B16G16R16F:
		case D3DFMT_G32R32F:
			return 64;
		case D3DFMT_A32B32G32R32F:
			return 128;
		default:
			if ( pbKnown != nullptr )
			{
				*pbKnown = false;
			}
			return 32;
		}
	}

	// Bytes from one row to the next: of blocks for the compressed formats, of
	// pixels otherwise, rounded up to four bytes as every driver does at least.
	inline size_t Pitch( D3DFORMAT eFormat, unsigned nWidth )
	{
		if ( IsBlockCompressed( eFormat ) )
		{
			return static_cast<size_t>( ( std::max )( 1u, ( nWidth + 3 ) / 4 ) ) * BlockBytes( eFormat );
		}
		const size_t nBytes = ( static_cast<size_t>( nWidth ) * BitsPerPixel( eFormat ) + 7 ) / 8;
		return ( nBytes + 3 ) & ~static_cast<size_t>( 3 );
	}

	// Rows in memory: block rows for the compressed formats.
	inline unsigned Rows( D3DFORMAT eFormat, unsigned nHeight )
	{
		return IsBlockCompressed( eFormat ) ? ( std::max )( 1u, ( nHeight + 3 ) / 4 ) : nHeight;
	}

	// Where (x, y) starts inside a locked surface, for a LockRect with a rect.
	inline size_t Offset( D3DFORMAT eFormat, size_t nPitch, unsigned nX, unsigned nY )
	{
		if ( IsBlockCompressed( eFormat ) )
		{
			return ( nY / 4 ) * nPitch + ( nX / 4 ) * BlockBytes( eFormat );
		}
		return nY * nPitch + ( static_cast<size_t>( nX ) * BitsPerPixel( eFormat ) ) / 8;
	}

	// Mip levels in a full chain down to 1x1, which is what Levels = 0 asks for.
	inline unsigned FullChain( unsigned nWidth, unsigned nHeight, unsigned nDepth = 1 )
	{
		unsigned nLevels = 1;
		unsigned nSize = ( std::max )( nWidth, ( std::max )( nHeight, nDepth ) );
		while ( nSize > 1 )
		{
			nSize >>= 1;
			++nLevels;
		}
		return nLevels;
	}
}
