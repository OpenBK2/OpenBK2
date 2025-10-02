#include "StdAfx.h"
#include <stdio.h>
#include "Streams.h"

unsigned int CBitStream::nBitsMask[32] = {
	0x01,       0x03,       0x07,       0x0F,        0x1F,       0x3F,       0x7F,       0xFF,
	0x01FF,     0x03FF,     0x07FF,     0x0FFF,      0x1FFF,     0x3FFF,     0x7FFF,     0xFFFF,
	0x01FFFF,   0x03FFFF,   0x07FFFF,   0x0FFFFF,    0x1FFFFF,   0x3FFFFF,   0x7FFFFF,   0xFFFFFF,
	0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,  0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

// CDataStream

void CDataStream::ReadOverflow( void *pDest, unsigned int nSize )
{
	//SetFailed();
	memset( pDest, 0, nSize );
	if ( pCurrent < pFileEnd )
	{
		int nRes = pFileEnd - pCurrent;
		Read( pDest, nRes ); // return Read( pDest, nRes );
	}
	pCurrent = pFileEnd;
	throw SFileIOError( "read overflow" );
}

void CDataStream::ReadString( string &res, int nMaxSize )
{
	int nSize = 0;
	Read( &nSize, 1 );
	if ( nSize & 1 )
		Read( ((char*)&nSize) + 1, 3 );
	nSize >>= 1;
	if ( nMaxSize > 0 && nSize > nMaxSize )
	{
		throw SFileIOError( "string read error" );
		//SetFailed();
		//return false;
	}
	unsigned char *pData = ReserveR( nSize );
	res.assign( (const char*)pData, nSize );
	Free( pData + nSize );
}

void CDataStream::WriteString( const string &res )
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

// CBitLocker realization

void CBitLocker::LockRead( CDataStream &data, unsigned int nSize )
{
	ASSERT(!pData);
	pData = &data;
	pBuffer = data.ReserveR( nSize );
	Init( pBuffer, read, nSize );
}

void CBitLocker::LockWrite( CDataStream &data, unsigned int nSize )
{
	ASSERT(!pData);
	pData = &data;
	pBuffer = data.ReserveW( nSize );
	Init( pBuffer, write, nSize );
}

/*void CBitLocker::ReserveRead( unsigned int nSize )
{
	ASSERT(pData);
	int nNewSize = pCurrent - pBuffer + nSize;
	unsigned char *pNewBuf;
	pNewBuf = pData->ReserveR( nNewSize );
#ifdef _DEBUG
	pReservedEnd = pNewBuf + nNewSize;
#endif
	int nFixup = pNewBuf - pBuffer;
	pCurrent += nFixup;
	pBitPtr += nFixup;
	pBuffer = pNewBuf;
}

void CBitLocker::ReserveWrite( unsigned int nSize )
{
	ASSERT(pData);
	int nNewSize = pCurrent - pBuffer + nSize;
	unsigned char *pNewBuf;
	pNewBuf = pData->ReserveW( nNewSize );
#ifdef _DEBUG
	pReservedEnd = pNewBuf + nNewSize;
#endif
	int nFixup = pNewBuf - pBuffer;
	pCurrent += nFixup;
	pBitPtr += nFixup;
	pBuffer = pNewBuf;
}*/

// CFixedMemStream

void CFixedMemStream::AllocForDirectReadAccess( unsigned int nSize )
{
	// this amount can not be guaranteed to be read
	ASSERT(0);
}

void CFixedMemStream::AllocForDirectWriteAccess( unsigned int nSize )
{
	ASSERT(0);
}

void CFixedMemStream::DirectRead( void *pDest, unsigned int nSize )
{
	// this should never happen
	ASSERT(0);
}

void CFixedMemStream::DirectWrite( const void *pSrc, unsigned int nSize )
{
	// write is forbidden
	ASSERT(0); 
}

// CMemoryStream

unsigned char *CMemoryStream::AllocBuffer( int nSize )
{
	unsigned char *pRes;
	if ( nSize > 2 * 1024 * 1024 )
	{
		pRes = (unsigned char*)VirtualAlloc( 0, nSize, MEM_COMMIT, PAGE_READWRITE );
		if ( pRes == 0 )
		{
			FILE *pFile = fopen( "error.txt", "wt" );
			fprintf( pFile, "ERROR!: can't alloc %d bytes", nSize, pReservedEnd - pBuffer );
			fflush( pFile );
			fclose( pFile );
		}
	}
	else
		pRes = new unsigned char[ nSize ];
	return pRes;
}

void CMemoryStream::FreeBuffer()
{
	int nSize = pReservedEnd - pBuffer;
	if ( nSize > 2 * 1024 * 1024 )
	{
		BOOL bOk = VirtualFree( pBuffer, 0, MEM_RELEASE );
		ASSERT( bOk );
	}
	else
		delete[] pBuffer;
}

void CMemoryStream::RealFixupBufferSize( int nNewSize )
{
	unsigned char *pNewBuf = AllocBuffer( nNewSize );
	memcpy( pNewBuf, pBuffer, pReservedEnd - pBuffer );
	FreeBuffer();
	pCurrent = pNewBuf + ( pCurrent - pBuffer );
	pFileEnd = pNewBuf + ( pFileEnd - pBuffer );
	pReservedEnd = pNewBuf + nNewSize;
	pBuffer = pNewBuf;
}

void CMemoryStream::FixupBufferSize( int nNewSize )
{
	RealFixupBufferSize( nNewSize * 2 + 64 );
}

void CMemoryStream::AllocForDirectReadAccess( unsigned int nSize )
{
	RealFixupBufferSize( GetPosition() + nSize );
}

// должна сохранять текущее содержимое буфера в памяти
void CMemoryStream::AllocForDirectWriteAccess( unsigned int nSize )
{
	RealFixupBufferSize( GetPosition() + nSize );
}

// c) чтение/запись не укладывающиеся в текущий буфер
void CMemoryStream::DirectRead( void *pDest, unsigned int nSize )
{
	// should never happen
	ASSERT(0);
}

void CMemoryStream::DirectWrite( const void *pSrc, unsigned int nSize )
{
	FixupBufferSize( GetPosition() + nSize );
	RWrite( pSrc, nSize );
}

CMemoryStream::CMemoryStream() 
{ 
	nFlags = F_CanRead | F_CanWrite; 
	int nDefaultSize = 32;
	pBuffer = AllocBuffer( nDefaultSize );
	pReservedEnd = pBuffer + nDefaultSize;
	pFileEnd = pBuffer;
	pCurrent = pBuffer;
}

CMemoryStream::~CMemoryStream()
{
	FreeBuffer();
}

void CMemoryStream::SetSizeDiscard( int nSize )
{
	int nBufSize = nSize + 64;
	FreeBuffer();
	pBuffer = AllocBuffer( nBufSize );
	pReservedEnd = pBuffer + nBufSize;
	pFileEnd = pBuffer + nSize;
	pCurrent = pBuffer;
}

void CMemoryStream::CopyMemoryStream( const CMemoryStream &src )
{
	int nBufSize = src.pReservedEnd - src.pBuffer;
	pBuffer = AllocBuffer( nBufSize );
	pReservedEnd = pBuffer + nBufSize;
	memcpy( pBuffer, src.pBuffer, nBufSize );
	pFileEnd = pBuffer + ( src.pFileEnd - src.pBuffer );
	pCurrent = pBuffer + ( src.pCurrent - src.pBuffer );
	nFlags = src.nFlags;
}

CMemoryStream::CMemoryStream( const CMemoryStream &src )
{
	CopyMemoryStream( src );
}

CMemoryStream& CMemoryStream::operator=( const CMemoryStream &src )
{
	FreeBuffer();
	CopyMemoryStream( src );
	return *this;
}

// CBufferedStream

void CBufferedStream::FlushBuffer()
{
	if ( IsWasted() )
	{
		ASSERT( pBuffer );
		ClearWasted();
		if ( !pBuffer )
			return;
		int nSize = pCurrent - pBuffer;
		if ( DoWrite( nBufferStart, pBuffer, nSize ) != nSize )
			throw SFileIOError( "write error" );// SetFailed(); 
		FixupSize();
	} 
}

void CBufferedStream::SetNewBufferSize( unsigned int nSize )
{
	int nNewSize = nSize + 1024;
	if ( nNewSize < pReservedEnd - pBuffer )
		return;
	unsigned char *pNewBuf = new unsigned char [ nNewSize ];
	if ( pBuffer )
	{
		memcpy( pNewBuf, pBuffer, pReservedEnd - pBuffer );
		delete[] pBuffer;
	}
	int nFixup = pNewBuf - pBuffer;
	pCurrent += nFixup;
	pFileEnd += nFixup;
	pBuffer = pNewBuf;
	pReservedEnd = pNewBuf + nNewSize;
}

void CBufferedStream::StartAccess( unsigned int nFileSize, unsigned int nSize )
{
	ClearWasted();
	pBuffer = new unsigned char [ nSize ];
	pCurrent = pBuffer;
	pFileEnd = pBuffer + nFileSize;
	pReservedEnd = pBuffer + nSize;
	nBufferStart = 0;
	LoadBufferForced( 0 );
}

void CBufferedStream::FinishAccess()
{
	FlushBuffer();
	if ( pBuffer ) delete[] pBuffer; pBuffer = 0;
}

template<class T>
T __Min( const T &a, const T &b ) { return a < b ? a : b; }
//
void CBufferedStream::LoadBufferForced( int nPos )
{
	FlushBuffer();
	int nRead = __Min( pReservedEnd - pBuffer, pFileEnd - pCurrent );
	if ( DoRead( nPos, pBuffer, nRead ) != nRead )
		throw SFileIOError( "read error" );//SetFailed(); // throw
	pCurrent += nBufferStart - nPos;
	pFileEnd += nBufferStart - nPos;
	nBufferStart = nPos;
}

// shift buffer start to pCurrent
void CBufferedStream::ShiftBuffer()
{
	int nPos = GetPosition();
	if ( nBufferStart == nPos )
		return;
	LoadBufferForced( nPos ); // also shifts pCurrent to pBuffer
}

// a) недостаток размера буфера для операция прямого доступа
void CBufferedStream::AllocForDirectReadAccess( unsigned int nSize )
{
	if ( nSize > pReservedEnd - pBuffer )
	{
		SetNewBufferSize( nSize );
		LoadBufferForced( nBufferStart );
	}
	else
		ShiftBuffer();
}

void CBufferedStream::AllocForDirectWriteAccess( unsigned int nSize )
{
	SetNewBufferSize( nSize );
}

void CBufferedStream::NotifyFinishDirectAccess()
{
	ShiftBuffer();
}

void CBufferedStream::Seek( unsigned int nPos )
{
	FixupSize();
	unsigned char *pNewCurrent = pBuffer + nPos - nBufferStart; 
	if ( pNewCurrent >= pBuffer && pNewCurrent < pReservedEnd )
	{
		pCurrent = pNewCurrent;
		return;
	}
	FlushBuffer(); 
	pCurrent = pNewCurrent;
	ShiftBuffer();
}

void CBufferedStream::DirectRead( void *pDest, unsigned int nSize )
{
	if ( nSize > pReservedEnd - pBuffer )
	{
		FlushBuffer();
		unsigned int nRes = DoRead( GetPosition(), pDest, nSize );
		if ( nRes != nSize )
			throw SFileIOError( "read size mismatch" );//SetFailed(); // throw
		pCurrent += nSize;
		ShiftBuffer();
		return;
	}
	ShiftBuffer();
	Read( pDest, nSize );
}

void CBufferedStream::DirectWrite( const void *pSrc, unsigned int nSize )
{
	if ( nSize > pReservedEnd - pBuffer )
	{
		FlushBuffer();
		unsigned int nRes = DoWrite( GetPosition(), pSrc, nSize );
		if ( nRes != nSize )
			throw SFileIOError( "write size mismatch" );//SetFailed(); // throw
		pCurrent += nSize;
		FixupSize();
		ShiftBuffer();
		return;
	}
	ShiftBuffer();
	Write( pSrc, nSize );
}

// CFileStream

unsigned int CFileStream::DoRead( unsigned int nPos, void *pDest, unsigned int nSize )
{
	ASSERT( pFile );
	if ( !pFile )
		return 0;
	if ( fseek( pFile, nPos, SEEK_SET ) )
		return 0;
	return fread( pDest, 1, nSize, pFile );
}

unsigned int CFileStream::DoWrite( unsigned int nPos, const void *pSrc, unsigned int nSize )
{
	ASSERT( pFile );
	if ( !pFile )
		return 0;
	if ( fseek( pFile, nPos, SEEK_SET ) )
		return 0;
	return fwrite( pSrc, 1, nSize, pFile );
}

void CFileStream::CloseFile()
{
	FinishAccess();
	if ( pFile ) fclose( pFile ); pFile = 0;
}

void CFileStream::Open( const char *pszFName, const char *pszMode, int _nFlags )
{
	CloseFile();
	//SetOk();
	nFlags = _nFlags; 
	pFile = fopen( pszFName, pszMode ); 
	if ( pFile )
	{
		fseek( pFile, 0, SEEK_END );
		int nFileSize = ftell( pFile );
		StartAccess( nFileSize, 1024 );
	}
	else
		throw SFileIOError( string("error opening file ") + pszFName );//SetFailed();
}


