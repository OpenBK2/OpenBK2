#include "stdafx.h"

#include "FilePath.h"
#include "FileReaders.h"
#include "FileUtils.h"
#include "VFSOperations.h"
#include "WinCursor.h"
#include "WinImageFormats.h"

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

int GetStepCount( TCursor hCursor )
{
	// One handle for the whole file, animated by USER32, so there is never more
	// than the one step and nothing for a caller to drive.
	return hCursor != 0 ? 1 : 0;
}

int GetStepDelay( TCursor, int )
{
	return 0;
}

TFrame GetStepFrame( TCursor hCursor, int )
{
	return hCursor;
}

#else

// One frame of the ICO container at nBase, as an SDL cursor. 0 if it cannot be
// read. A .cur and one "icon" chunk of a .ani are the same container, which is
// why the two paths in LoadCursor meet here.
static SDL_Cursor *CreateFrame( const uint8_t *pData, size_t nSize, size_t nBase )
{
	// 32 is the size every cursor in the shipped data is drawn at. Each of those
	// frames holds exactly one image, so the choice only begins to matter for a
	// cursor that ships more than one.
	std::vector<NWinImage::SImageInfo> images;
	if ( !NWinImage::GetImages( &images, pData, nSize, nBase ) )
	{
		return 0;
	}
	const int nIndex = NWinImage::SelectImage( images, 32 );
	NWinImage::SImage image;
	if ( nIndex < 0 || !NWinImage::DecodeImage( &image, pData, nSize, nBase, nIndex ) )
	{
		return 0;
	}

	// SImage is tightly packed, so one row is one row and the pitch is the width
	SDL_Surface *pSurface = SDL_CreateSurfaceFrom( image.nWidth, image.nHeight,
		SDL_PIXELFORMAT_ARGB8888, &image.pixels[0],
		image.nWidth * static_cast<int>( sizeof( uint32_t ) ) );
	if ( pSurface == 0 )
	{
		return 0;
	}
	// SDL_CreateColorCursor copies what it is given, so the surface and the
	// pixels behind it can both go once it returns. The hotspot travels with the
	// cursor, so a file whose frames move it needs nothing else done about it.
	SDL_Cursor *pCursor = SDL_CreateColorCursor( pSurface, image.nHotX, image.nHotY );
	SDL_DestroySurface( pSurface );
	return pCursor;
}

// A loaded cursor: every frame the file stores, and the order and timing in
// which to show them. A .cur is the degenerate case of one frame and one step.
struct SCursor
{
	// One SDL cursor per stored frame, in the order the file stores them. An
	// entry is 0 where that frame would not decode.
	std::vector<SDL_Cursor *> frames;
	// One entry per step of the animation, indexing frames.
	std::vector<int> sequence;
	// How long each step is shown, in milliseconds, one per sequence entry.
	std::vector<int> delays;
};

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

	SCursor *pCursor = new SCursor;
	NWinImage::SAniInfo ani;
	if ( NWinImage::ReadAni( &ani, &buffer[0], buffer.size() ) )
	{
		for ( size_t i = 0; i < ani.frameOffsets.size(); ++i )
		{
			pCursor->frames.push_back( CreateFrame( &buffer[0], buffer.size(), ani.frameOffsets[i] ) );
		}
		// A step whose frame would not decode is dropped rather than shown as a
		// hole, so a partly unreadable file still animates over what is left.
		for ( size_t i = 0; i < ani.sequence.size(); ++i )
		{
			if ( pCursor->frames[ani.sequence[i]] == 0 )
			{
				continue;
			}
			pCursor->sequence.push_back( ani.sequence[i] );
			pCursor->delays.push_back( ani.delays[i] );
		}
	}
	else
	{
		// Not a .ani, so the file is a .cur: one frame, and no timing to run.
		SDL_Cursor *pFrame = CreateFrame( &buffer[0], buffer.size(), 0 );
		if ( pFrame != 0 )
		{
			pCursor->frames.push_back( pFrame );
			pCursor->sequence.push_back( 0 );
			pCursor->delays.push_back( 0 );
		}
	}
	if ( pCursor->sequence.empty() )
	{
		DestroyCursor( pCursor );
		return 0;
	}
	return pCursor;
}

void DestroyCursor( TCursor hCursor )
{
	if ( hCursor == 0 )
	{
		return;
	}
	for ( size_t i = 0; i < hCursor->frames.size(); ++i )
	{
		if ( hCursor->frames[i] != 0 )
		{
			SDL_DestroyCursor( hCursor->frames[i] );
		}
	}
	delete hCursor;
}

int GetStepCount( TCursor hCursor )
{
	return hCursor != 0 ? static_cast<int>( hCursor->sequence.size() ) : 0;
}

int GetStepDelay( TCursor hCursor, int nStep )
{
	if ( hCursor == 0 || nStep < 0 || nStep >= static_cast<int>( hCursor->delays.size() ) )
	{
		return 0;
	}
	return hCursor->delays[nStep];
}

TFrame GetStepFrame( TCursor hCursor, int nStep )
{
	if ( hCursor == 0 || nStep < 0 || nStep >= static_cast<int>( hCursor->sequence.size() ) )
	{
		return 0;
	}
	return hCursor->frames[hCursor->sequence[nStep]];
}

#endif
}
