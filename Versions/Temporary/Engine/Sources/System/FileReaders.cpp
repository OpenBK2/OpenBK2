#include "StdAfx.h"
#include "FileReaders.h"
#include "..\Misc\Win32Helper.h"


int CMMFile::GetFileSize()
{
	return ::GetFileSize( hFile, 0 );
}

void CMMFile::SetFileSize( int nSize )
{
	ASSERT( hMapping == 0 );
	SetFilePointer( hFile, nSize, 0, FILE_BEGIN );
	SetEndOfFile( hFile );
}

void CMMFile::MapFile( int nSize, bool bCanWrite )
{
	DWORD dwFFlags = 0;
	if ( bCanWrite )
		dwFFlags = PAGE_READWRITE;
	else
		dwFFlags = PAGE_READONLY;
	hMapping = CreateFileMapping( hFile, 0, dwFFlags|SEC_COMMIT, 0, nSize, 0 );
}

void CMMFile::UnmapFile()
{
	if ( hMapping != 0 )
	{
		CloseHandle( hMapping );
		hMapping = 0;
	}
}

CMMFile::CMMFile( const char *pszName, EStreamAccess access ) : hFile(INVALID_HANDLE_VALUE), hMapping(0)
{
	if ( access == STREAM_ACCESS_READ_WRITE )
	{
		hFile = ::CreateFile( pszName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
		FILETIME createTime, lastAccessTime, lastWriteTime;
		if ( hFile != INVALID_HANDLE_VALUE && GetFileTime( hFile, &createTime, &lastAccessTime, &lastWriteTime ) )
		{
			FILETIME currentTime;
			GetSystemTimeAsFileTime( &currentTime );
			SetFileTime( hFile, &createTime, &currentTime, &currentTime );
		}
	}
	else
		hFile = ::CreateFile( pszName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
}

CMMFile::~CMMFile()
{
	ASSERT( hMapping == 0 );
	if ( hFile != INVALID_HANDLE_VALUE )
		CloseHandle( hFile );
}

// CMemoryMappedFileFragment

static int GetAllocGranularity()
{
	static int nAllocGranularity;
	static int nCalced;
	if ( !nCalced )
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo( &sysInfo );
		nAllocGranularity = sysInfo.dwAllocationGranularity;
		nCalced = 1;
	}
	return nAllocGranularity;
}

void *CMemoryMappedFileFragment::MapFile( int nSize )
{
	if ( nSize == 0 )
		return 0;
	HANDLE hMapping = pFile->GetMapping();
	if ( hMapping )
	{
		int nAllocGranularity = GetAllocGranularity();
		int nBaseOffset = nOffset & ~( nAllocGranularity - 1 );
		int nShift = nOffset - nBaseOffset;
		unsigned char *p = (unsigned char*) MapViewOfFile( pFile->GetMapping(), FILE_MAP_READ, 0, nBaseOffset, nSize + nShift );
		return p + nShift;
	}
	else
	{
		static NWin32Helper::CCriticalSection directRead;
		NWin32Helper::CCriticalSectionLock dr( directRead );
		ASSERT( !CanWrite() );
		char *pszBuf = new char[ nSize ];
		HANDLE hFile = pFile->GetFile();
		SetFilePointer( hFile, nOffset, 0, FILE_BEGIN );
		DWORD dwRes;
		BOOL bRead = ReadFile( hFile, pszBuf, nSize, &dwRes, 0 );
		ASSERT( bRead && dwRes == nSize );
		return pszBuf;
	}
}

void CMemoryMappedFileFragment::UnmapFile( void *p )
{
	if ( p == 0 )
		return;
	HANDLE hMapping = pFile->GetMapping();
	if ( hMapping )
	{
		int nAllocGranularity = GetAllocGranularity();
		int nShift = nOffset & ( nAllocGranularity - 1 );
		void *pAligned = (void*)( ((int)p) - nShift );
		BOOL bTest = UnmapViewOfFile( pAligned );
		ASSERT( bTest );
	}
	else
	{
		delete[] (char*)p;
	}
}

void CMemoryMappedFileFragment::FlushFile( void *p )
{
	HANDLE hMapping = pFile->GetMapping();
	if ( hMapping )
	{
		BOOL bTest = FlushViewOfFile( p, 0 );
		ASSERT( bTest );
	}
	else
		ASSERT(0);
}
