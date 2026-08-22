#pragma once

#include <cstdint>

enum EStreamAccess
{
	STREAM_ACCESS_READ,
	STREAM_ACCESS_READ_WRITE
};

class CMMFile
{
	HANDLE hFile, hMapping;

	CMMFile( const CMMFile& ) { ASSERT(0); }
	void operator=( const CMMFile& ) { ASSERT(0); }
public:
	CMMFile() : hFile(INVALID_HANDLE_VALUE), hMapping(0) {}
	CMMFile( const char *pszName, EStreamAccess access );
	~CMMFile();
	int GetFileSize();
	void SetFileSize( int nSize );
	void MapFile( int nSize, bool bCanWrite );
	void UnmapFile();
	HANDLE GetMapping() const { return hMapping; }
	HANDLE GetFile() const { return hFile; }
	bool IsOk() const { return hFile != INVALID_HANDLE_VALUE; }
};

class CMemoryMappedFile : public CMappedStream
{
	CMMFile file;

	int GetFileSize() { return file.GetFileSize(); }
	void SetFileSize( int nSize )
	{
		ASSERT( CanWrite() );
		if ( !CanWrite() )
			return;
		file.SetFileSize( nSize );
	}
	void *MapFile( int nSize )
	{
		file.MapFile( nSize, CanWrite() );
		uint32_t dwVFlags = 0;
		if ( CanWrite() )
			dwVFlags = FILE_MAP_ALL_ACCESS;
		else
			dwVFlags = FILE_MAP_READ;
		return MapViewOfFile( file.GetMapping(), dwVFlags, 0, 0, 0 );
	}
	void UnmapFile( void *p )
	{
		if ( p )
		{
			bool bTest = UnmapViewOfFile( p );
			ASSERT( bTest );
		}
		file.UnmapFile();
	}
	void FlushFile( void *p )
	{
		bool bTest = FlushViewOfFile( p, 0 );
		ASSERT( bTest );
	}
	CMemoryMappedFile( const CMemoryMappedFile& );
	void operator=( const CMemoryMappedFile& );
public:
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

class CMemoryMappedFileFragment : public CMappedStream
{
	CMMFile *pFile;
	int nOffset, nSize;

	virtual int GetFileSize() { return nSize; }
	virtual void SetFileSize( int nSize ) { ASSERT(0); }
	virtual void *MapFile( int nSize );
	virtual void UnmapFile( void *p );
	virtual void FlushFile( void *p );
public:
	CMemoryMappedFileFragment( CMMFile *_pFile, int _nOffset, int _nSize ) : pFile(_pFile), nOffset(_nOffset), nSize(_nSize) { StartAccess( F_CanRead ); }
	~CMemoryMappedFileFragment() { FinishAccess(); }
};

