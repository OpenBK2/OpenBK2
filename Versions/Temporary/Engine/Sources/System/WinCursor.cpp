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

#else

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
	if ( !NWinImage::FindFirstAniFrame( &nBase, &buffer[0], buffer.size() ) )
	{
		nBase = 0;
	}
	NWinImage::SImage image;
	if ( !NWinImage::DecodeFirstImage( &image, &buffer[0], buffer.size(), nBase ) )
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
	// pixels behind it can both go once it returns
	SDL_Cursor *pCursor = SDL_CreateColorCursor( pSurface, image.nHotX, image.nHotY );
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
