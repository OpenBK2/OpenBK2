#pragma once

#include "System_export.h"

// CMemoryMappedFile and CMemoryMappedFileFragment derive from
// CMappedStream, so this header does not stand on its own without it
#include "Streams.h"

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <cstdint>
#include <string>

enum EStreamAccess
{
	STREAM_ACCESS_READ,
	STREAM_ACCESS_READ_WRITE
};

// A file that can be mapped, identified by name rather than by an open handle.
//
// Holding a handle open would be the obvious thing, and the Win32 version did,
// resizing through it with SetEndOfFile. That is not available here: growing
// the file is a separate step now, and resizing a file by name fails on Windows
// while anything else holds it open. So the name is the handle, and the
// invariant that used to be "no mapping object exists" is kept explicitly.
class SYSTEM_EXPORT CMMFile
{
	boost::interprocess::file_mapping mapping;
	std::string szFileName;
	int nFileSize = 0;
	bool bValid = false;
	bool bMapped = false;
public:
	CMMFile( const CMMFile& ) = delete;
	void operator=( const CMMFile& ) = delete;

	CMMFile() = default;
	CMMFile( const char *pszName, EStreamAccess access );
	~CMMFile() = default;

	int GetFileSize();
	void SetFileSize( int nSize );
	void MapFile( int nSize, bool bCanWrite );
	void UnmapFile();

	boost::interprocess::file_mapping& GetMapping() { return mapping; }
	//! Whether MapFile last succeeded. A fragment reads the file directly when
	//! this is false, which is how archives were read before mapping them.
	bool IsMapped() const { return bMapped; }
	bool IsOk() const { return bValid; }
	const std::string& GetFileName() const { return szFileName; }
};

class SYSTEM_EXPORT CMemoryMappedFile : public CMappedStream
{
	CMMFile file;
	boost::interprocess::mapped_region region;

	int GetFileSize() { return file.GetFileSize(); }
	void SetFileSize( int nSize )
	{
		ASSERT( CanWrite() );
		if ( !CanWrite() )
			return;
		file.SetFileSize( nSize );
	}
	void *MapFile( int nSize );
	void UnmapFile( void *p );
	void FlushFile( void *p );
public:
	CMemoryMappedFile( const CMemoryMappedFile& ) = delete;
	void operator=( const CMemoryMappedFile& ) = delete;

	CMemoryMappedFile( const char *pszName, EStreamAccess access = STREAM_ACCESS_READ_WRITE ) : file( pszName, access )
	{
		if ( !file.IsOk() )
			SetBuffer( 0, 0, 0, 0, F_Broken );
		else
		{
			if ( access == STREAM_ACCESS_READ_WRITE )
				StartAccess( F_CanRW );
			else
				StartAccess( F_CanRead );
		}
	}
	~CMemoryMappedFile() { FinishAccess(); }
};

// A window onto part of a file. The offset is arbitrary while a mapping can
// only start at a multiple of the allocation granularity; mapped_region keeps
// that difference itself and hands back an address inside the region, so there
// is no alignment arithmetic here.
class SYSTEM_EXPORT CMemoryMappedFileFragment : public CMappedStream
{
	CMMFile *pFile;
	int nOffset, nSize;

	boost::interprocess::mapped_region region;
	//! Set when the file is not mapped and the window was read into memory
	//! instead, in which case the buffer is owned rather than borrowed.
	unsigned char *pOwnedBuffer = nullptr;

	virtual int GetFileSize() { return nSize; }
	virtual void SetFileSize( int nSize ) { ASSERT(0); }
	virtual void *MapFile( int nSize );
	virtual void UnmapFile( void *p );
	virtual void FlushFile( void *p );
public:
	CMemoryMappedFileFragment( CMMFile *_pFile, int _nOffset, int _nSize ) : pFile(_pFile), nOffset(_nOffset), nSize(_nSize) { StartAccess( F_CanRead ); }
	~CMemoryMappedFileFragment() { FinishAccess(); }
};

