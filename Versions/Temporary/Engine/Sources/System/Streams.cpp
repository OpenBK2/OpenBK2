#include "stdafx.h"
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include "Streams.h"
#include "FileReaders.h"
#include "VFS.h"
#include "System/FilePath.h"

// ************************************************************************************************************************ //
// **
// ** CDataStream
// **
// **
// **
// ************************************************************************************************************************ //

void CDataStream::SetBuffer( unsigned char *_pBuffer, int nBufSize, int nPos, int nSize, int _nFlags )
{
	data.pBuffer = _pBuffer;
	data.pBufferEnd = data.pBuffer + nBufSize;
	data.pCurrent = data.pBuffer + nPos;
	data.pFileEnd = data.pBuffer + nSize;
	data.nFlags = _nFlags;
}

bool CDataStream::FixupBuf( int nOldSize )
{
	if ( ( data.nFlags & F_Broken ) || !CanWrite() )
	{
		unsigned char *pOldFileEnd = data.pBuffer + nOldSize;
		data.pCurrent = pOldFileEnd;
		data.pFileEnd = pOldFileEnd;
		SetBroken();
		return false;
	}
	ASSERT( data.pCurrent > data.pBufferEnd );
	ASSERT( data.pCurrent == data.pFileEnd );

	int nNewBufSize = data.pCurrent - data.pBuffer;
	nNewBufSize = (std::min)( nNewBufSize + 65536 * 16, nNewBufSize * 2 );
	nNewBufSize = (std::max)( nNewBufSize, 4096 );
	nNewBufSize &= ~4095;
	AllocBuf( nOldSize, nNewBufSize );
	return IsOk();
}

void CDataStream::ReadOverflow( void *pDest, int nSize )
{
	if ( data.pCurrent != data.pFileEnd )
	{
		int nLeft = data.pFileEnd - data.pCurrent;
		Read( pDest, nLeft );
		Read( ((char*)pDest ) + nLeft, nSize - nLeft );
	}
	else
	{
		SetBroken();
		memset( pDest, 0, nSize );
	}
}

void CDataStream::ReadString( std::string &res, int nMaxSize )
{
	int nSize = 0;
	Read( &nSize, 1 );
	if ( nSize & 1 )
		Read( ((char*)&nSize) + 1, 3 );
	nSize >>= 1;
	if ( ( nMaxSize > 0 && nSize > nMaxSize ) )
	{
		res = "";
		SetBroken();
		return;
	}
	if ( data.pCurrent + nSize > data.pFileEnd )
	{
		data.pCurrent = data.pFileEnd;
		res = "";
		SetBroken();
		return;
	}
	res.assign( (const char*)data.pCurrent, nSize );
	data.pCurrent += nSize;
}

void CDataStream::WriteString( const std::string &res )
{
	int nSize = res.size(), nVal;
	if ( nSize >= 128 )
	{
		nVal = nSize * 2 + 1;
		Write( &nVal, 4 );
	}
	else
	{
		nVal = nSize * 2;
		Write( &nVal, 1 );
	}
	Write( res.data(), nSize );
}

void CDataStream::ReadTo( CDataStream *pDst, unsigned int nSize )
{
	pDst->SetSize( 0 );
	pDst->SetSize( nSize );
	Read( pDst->GetBufferForWrite(), nSize );
	pDst->Seek(0);
}

void CDataStream::WriteFrom( CDataStream &src )
{
	src.Seek(0);
	int nSize = src.GetSize();
	Write( src.GetBuffer(), nSize );
}

// ************************************************************************************************************************ //
// **
// ** CMemoryStream
// **
// **
// **
// ************************************************************************************************************************ //

void CMemoryStream::AllocBuf( int nOldFileSize, int nSize )
{
	unsigned char *pBuf = new unsigned char[ nSize ];
	int nTransfer = (std::min)( nSize, nOldFileSize );
	unsigned char *pPrevBuffer = GetBufferPtr();
	if ( nTransfer != 0 && pBuf != 0 )
		memcpy( pBuf, pPrevBuffer, nTransfer );
	delete[] pPrevBuffer;
	if ( pBuf == 0 )
		SetBuffer( 0, 0, 0, 0, GetFlags() | F_Broken );
	else
		SetBuffer( pBuf, nSize, GetPosition(), GetSize(), GetFlags() );
}

CMemoryStream::~CMemoryStream()
{
	delete[] GetBufferPtr();
}

CMemoryStream::CMemoryStream( const CMemoryStream &src ) : CDataStream( 0 )
{
	CopyMemoryStream( src );
}

CMemoryStream& CMemoryStream::operator=( const CMemoryStream &src )
{
	delete[] GetBufferPtr();
	CopyMemoryStream( src );
	return *this;
}

void CMemoryStream::SetSizeDiscard( int nSize )
{
	delete[] GetBufferPtr();
	unsigned char *pBuf = new unsigned char[ nSize ];
	SetBuffer( pBuf, nSize, 0, nSize, F_CanRW );
}

void CMemoryStream::CopyMemoryStream( const CMemoryStream &src )
{
	int nBufSize = src.GetBufferSize();
	SetBuffer( new unsigned char[ nBufSize ], nBufSize, src.GetPosition(), src.GetSize(), src.GetFlags() );
	memcpy( GetBufferPtr(), src.GetBufferPtr(), nBufSize );
}

// ************************************************************************************************************************ //
// **
// ** CMappedStream
// **
// **
// **
// ************************************************************************************************************************ //

void CMappedStream::AllocBufImpl( int nBufSize, int nPos, int nSize )
{
	ASSERT( nPos <= nSize );
	ASSERT( nBufSize >= nSize );
	unsigned char *pBuf = 0;
	if ( nBufSize != 0 )
		pBuf = (unsigned char *) MapFile( nBufSize );
	if ( pBuf == 0 && nBufSize != 0 )
		SetBuffer( 0, 0, 0, 0, GetFlags() | F_Broken );
	else
		SetBuffer( pBuf, nBufSize, nPos, nSize, GetFlags() );
}

void CMappedStream::ReleaseBuf()
{
	UnmapFile( GetBufferPtr() );
	if ( GetSize() != GetBufferSize() && IsOk() )
		SetFileSize( GetSize() );
}

void CMappedStream::AllocBuf( int nOldFileSize, int nSize )
{
	ReleaseBuf();
	AllocBufImpl( nSize, GetPosition(), GetSize() );
}

void CMappedStream::StartAccess( int _nFlags )
{
	SetFlags( _nFlags );
	int nFileSize = GetFileSize();
	AllocBufImpl( nFileSize, 0, nFileSize );
}

void CMappedStream::Flush()
{
	if ( IsOk() )
		FlushFile( GetBufferPtr() );
}

// ************************************************************************************************************************ //
// **
// ** CFileStream
// **
// **
// **
// ************************************************************************************************************************ //

namespace
{

//! Whether to write a line for every file this process opens.
//!
//! Off by default and read once. The engine probes for files that are allowed
//! to be missing - a mod config, a user profile - so a permanent record of every
//! failure would be noise. It is the failures that are not allowed to happen
//! that are worth an hour, and until now none of them said anything at all: a
//! config that could not be opened produced no line in the log, no message on
//! the console, and no clue that the path was wrong.
//!
//! Set OPENBK2_FILE_TRACE=1 in the environment to turn it on. Every open is
//! reported, successes included, because knowing which of two candidate paths
//! was tried is usually the question.
bool IsFileTraceEnabled()
{
	static const bool bEnabled = getenv( "OPENBK2_FILE_TRACE" ) != 0;
	return bEnabled;
}

//! One line per open: how it was opened, what was asked for, and what happened.
//!
//! pszWhy is the reason a failure failed, where there is one to give. The VFS
//! has none beyond "no such entry"; an operating system file has errno, which
//! separates a missing file from a directory that cannot be searched, and those
//! want different fixes.
void TraceFileOpen( const char *pszHow, const std::string &szFileName, const bool bOk, const char *pszWhy )
{
	if ( !IsFileTraceEnabled() )
	{
		return;
	}
	if ( bOk )
	{
		csSystem << "file: opened  " << pszHow << " " << szFileName.c_str() << endl;
	}
	else
	{
		csSystem << CC_ORANGE << "file: FAILED  " << pszHow << " " << szFileName.c_str()
		         << " (" << ( pszWhy != 0 ? pszWhy : "no reason given" ) << ")" << endl;
	}
}

}

CFileStream::CFileStream( NVFS::IVFS *pVFS, const std::string &szFileName )
: CDataStream( F_Broken )
{
	//	NI_ASSERT( szFileName.find( ":" ) == string::npos, StrFmt( "\":\" found in file name %s passed to VFS! WIN_READ_ONLY should've been used", szFileName.c_str() ) );
	pStream = pVFS->OpenFile( szFileName );
	if ( pStream )
		SyncWith( *pStream );
	TraceFileOpen( "vfs", szFileName, pStream != 0, "no such entry in the VFS" );
}

CFileStream::CFileStream( NVFS::IFileCreator *pFileCreator, const std::string &szFileName )
: CDataStream( F_Broken )
{
	//	NI_ASSERT( szFileName.find( ":" ) == string::npos, StrFmt( "\":\" found in file name %s passed to VFS! WIN_CREATE should've been used", szFileName.c_str() ) );
	pStream = pFileCreator->CreateFile( szFileName );
	if ( pStream )
		SyncWith( *pStream );
	TraceFileOpen( "create", szFileName, pStream != 0, "the file creator refused it" );
}

CFileStream::CFileStream( const std::string &szFileName, const EWinMode eWinMode )
: CDataStream( F_Broken )
{
	// Captured before anything else can overwrite it, since the checks below and
	// the tracing itself both make calls of their own.
	errno = 0;
	bool bOk = false;
	if ( eWinMode == WIN_READ_ONLY )
	{
		pStream = new CMemoryMappedFile( szFileName.c_str(), STREAM_ACCESS_READ );
		bOk = pStream->CanRead();
		if ( !bOk )
			pStream->SetBroken();
	}
	else
	{
		NFile::CreatePath( NFile::GetFilePath(szFileName) );
		pStream = new CMemoryMappedFile( szFileName.c_str(), STREAM_ACCESS_READ_WRITE );
		bOk = pStream->CanWrite();
		if ( bOk )
			pStream->Trunc();
		else
			pStream->SetBroken();
	}
	const int nErrno = errno;

	SyncWith( *pStream );
	TraceFileOpen( eWinMode == WIN_READ_ONLY ? "read" : "create",
	               szFileName, bOk, nErrno != 0 ? strerror( nErrno ) : 0 );
}

CFileStream::~CFileStream()
{
	if ( pStream )
	{
		pStream->SyncWith( *this );
		delete pStream;
	}
}

void CFileStream::AllocBuf( int nOldFileSize, int nSize )
{
	if ( pStream )
	{
		pStream->SyncWith( *this );
		pStream->AllocBuf( nOldFileSize, nSize );
		SyncWith( *pStream );
	}
}

void CFileStream::Flush()
{
	if ( pStream )
	{
		pStream->SyncWith( *this );
		pStream->Flush();
		SyncWith( *pStream );
	}
}

