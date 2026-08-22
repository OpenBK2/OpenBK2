#include "stdafx.h"
#include "FileReaders.h"

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace
{
	namespace bip = boost::interprocess;

	//! The mapping constructors report failure by throwing; every caller here
	//! wants a broken stream instead, which is what the layer above expects
	//! from a file it cannot open.
	template < class T, class TMake >
	bool TryMake( T *pDest, TMake makeIt )
	{
		try
		{
			*pDest = makeIt();
			return true;
		}
		catch ( const bip::interprocess_exception & )
		{
			*pDest = T();
			return false;
		}
	}
}

// CMMFile

CMMFile::CMMFile( const char *pszName, EStreamAccess access ) : szFileName( pszName )
{
	const std::filesystem::path path( szFileName );
	std::error_code ec;

	if ( access == STREAM_ACCESS_READ_WRITE )
	{
		// CreateFile( OPEN_ALWAYS ) created the file when it was missing and
		// left it alone when it was not. Opening with std::ios::app rather
		// than the default does the same: the default truncates, which would
		// discard the contents of every file the VFS opens for writing.
		if ( !std::filesystem::exists( path, ec ) )
		{
			std::ofstream create( path, std::ios::binary | std::ios::app );
			if ( !create )
			{
				return;
			}
		}
		// The Win32 version stamped the last-access and last-write times with
		// the current time on opening, so opening a save or profile counted as
		// touching it whether or not anything was written. Only the write time
		// can be set portably; the access time is dropped.
		std::filesystem::last_write_time( path, std::filesystem::file_time_type::clock::now(), ec );
		ec.clear();
	}

	const std::uintmax_t nSize = std::filesystem::file_size( path, ec );
	if ( ec )
	{
		return;
	}
	nFileSize = static_cast< int >( nSize );
	bValid = true;
}

int CMMFile::GetFileSize()
{
	std::error_code ec;
	const std::uintmax_t nSize = std::filesystem::file_size( std::filesystem::path( szFileName ), ec );
	nFileSize = ec ? 0 : static_cast< int >( nSize );
	return nFileSize;
}

void CMMFile::SetFileSize( int nSize )
{
	// Resizing by name fails on Windows while a mapping object holds the file
	// open, so this has to happen between UnmapFile and the next MapFile.
	// CMappedStream::ReleaseBuf already unmaps first; the assert catches any
	// other caller that does not.
	ASSERT( !bMapped );
	if ( nSize < 0 )
	{
		return;
	}
	std::error_code ec;
	std::filesystem::resize_file( std::filesystem::path( szFileName ), static_cast< std::uintmax_t >( nSize ), ec );
	if ( !ec )
	{
		nFileSize = nSize;
	}
}

void CMMFile::MapFile( int nSize, bool bCanWrite )
{
	bMapped = false;
	if ( !bValid || nSize <= 0 )
	{
		return;
	}
	if ( bCanWrite && nSize > GetFileSize() )
	{
		// CreateFileMapping extended the file to the requested size on its own.
		// Nothing here does, and a mapping covers only what the file already
		// holds, so growth is now an explicit step that has to come first.
		SetFileSize( nSize );
		if ( nFileSize < nSize )
		{
			return;
		}
	}
	bMapped = TryMake( &mapping, [&]
	{
		return bip::file_mapping( szFileName.c_str(), bCanWrite ? bip::read_write : bip::read_only );
	} );
}

void CMMFile::UnmapFile()
{
	mapping = bip::file_mapping();
	bMapped = false;
}

// CMemoryMappedFile

void *CMemoryMappedFile::MapFile( int nSize )
{
	file.MapFile( nSize, CanWrite() );
	if ( !file.IsMapped() )
	{
		return 0;
	}
	const bool bMade = TryMake( &region, [&]
	{
		return bip::mapped_region( file.GetMapping(), CanWrite() ? bip::read_write : bip::read_only,
		                           0, static_cast< std::size_t >( nSize ) );
	} );
	if ( !bMade )
	{
		file.UnmapFile();
		return 0;
	}
	return region.get_address();
}

void CMemoryMappedFile::UnmapFile( void *p )
{
	// The region has to go before the mapping it was taken from, and both have
	// to go before the file can be resized.
	region = bip::mapped_region();
	file.UnmapFile();
}

void CMemoryMappedFile::FlushFile( void *p )
{
	if ( region.get_address() == 0 )
	{
		return;
	}
	const bool bTest = region.flush();
	ASSERT( bTest );
}

// CMemoryMappedFileFragment

void *CMemoryMappedFileFragment::MapFile( int nSize )
{
	if ( nSize <= 0 )
	{
		return 0;
	}
	if ( pFile->IsMapped() )
	{
		// nOffset is arbitrary and a view can only start at a multiple of the
		// allocation granularity. mapped_region rounds down itself and offsets
		// the address it returns, which is the arithmetic this used to do by
		// hand with dwAllocationGranularity.
		const bool bMade = TryMake( &region, [&]
		{
			return bip::mapped_region( pFile->GetMapping(), bip::read_only, nOffset,
			                           static_cast< std::size_t >( nSize ) );
		} );
		return bMade ? region.get_address() : 0;
	}

	// The file is not mapped, so read the window out of it instead. The Win32
	// version seeked a shared handle under a lock; opening a stream per call
	// carries its own position and needs no lock.
	ASSERT( !CanWrite() );
	std::ifstream f( std::filesystem::path( pFile->GetFileName() ), std::ios::binary );
	if ( !f.seekg( nOffset ) )
	{
		return 0;
	}
	pOwnedBuffer = new unsigned char[ nSize ];
	if ( !f.read( reinterpret_cast< char * >( pOwnedBuffer ), nSize ) )
	{
		ASSERT( 0 );
		delete[] pOwnedBuffer;
		pOwnedBuffer = 0;
		return 0;
	}
	return pOwnedBuffer;
}

void CMemoryMappedFileFragment::UnmapFile( void *p )
{
	if ( pOwnedBuffer )
	{
		delete[] pOwnedBuffer;
		pOwnedBuffer = 0;
		return;
	}
	region = bip::mapped_region();
}

void CMemoryMappedFileFragment::FlushFile( void *p )
{
	// A fragment is read-only, so there is never anything to flush; reaching
	// here means someone opened one for writing.
	ASSERT( pFile->IsMapped() );
	if ( region.get_address() != 0 )
	{
		const bool bTest = region.flush();
		ASSERT( bTest );
	}
}
