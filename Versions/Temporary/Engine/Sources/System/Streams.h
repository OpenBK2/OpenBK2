#pragma once
#include "System_export.h"

namespace NVFS
{
	struct IVFS;
	struct IFileCreator;
}

class SYSTEM_EXPORT CDataStream
{
	struct SYSTEM_EXPORT SData
	{
		unsigned char *pBuffer, *pBufferEnd, *pFileEnd, *pCurrent;
		int nFlags;

		SData() : pBuffer( 0 ), pBufferEnd( 0 ), pFileEnd( 0 ), pCurrent( 0 ) { }
	};

	SData data;
protected:
	enum EFlags
	{
		F_Broken   = 1,
		F_CanRead  = 2,
		F_CanWrite = 4,
		F_CanRW    = F_CanWrite | F_CanRead,
	};

	void SetBuffer( unsigned char *_pBuffer, int nBufSize, int nPos, int nSize, int _nFlags );
	bool FixupBuf( int nOldSize );
	void ReadOverflow( void *pDest, int nSize );
	unsigned char *GetBufferPtr() const { return data.pBuffer; }
	int GetBufferSize() const { return data.pBufferEnd - data.pBuffer; }
	bool Seek( unsigned char *_p )
	{
		ASSERT( _p >= data.pBuffer );
		data.pCurrent = _p;
		unsigned char *pOldFileEnd = data.pFileEnd;
		if ( data.pCurrent > pOldFileEnd )
		{
			data.pFileEnd = data.pCurrent;
			if ( data.pCurrent > data.pBufferEnd )
				return FixupBuf( pOldFileEnd - data.pBuffer );
		}
		return true;
	}

	int GetFlags() const { return data.nFlags; }
	void SetFlags( int _nFlags ) { data.nFlags = _nFlags; }
public:
	CDataStream( int _nFlags ) { data.nFlags = _nFlags; }
	virtual ~CDataStream() {}

	void SyncWith( const CDataStream &stream ) { data = stream.data; }
	virtual void AllocBuf( int nOldFileSize, int nSize ) = 0;

	void Clear() { ASSERT( CanWrite() ); data.pFileEnd = data.pBuffer; data.pCurrent = data.pBuffer; }
	bool Seek( int nPos ) { return Seek( data.pBuffer + nPos ); }
	void Trunc() { ASSERT( CanWrite() ); data.pFileEnd = data.pCurrent; }
	
	// обычные функции для чтения/записи из/в поток
	void Read( void *pDest, int nSize )
	{
		ASSERT( CanRead() );
		if ( data.pCurrent + nSize > data.pFileEnd )
			ReadOverflow( pDest, nSize );
		else
		{
			memcpy( pDest, data.pCurrent, nSize );
			data.pCurrent += nSize;
		}
	}
	void Write( const void *pSrc, int nSize )
	{
		ASSERT( CanWrite() );
		if ( Seek( data.pCurrent + nSize ) )
			memcpy( data.pCurrent - nSize, pSrc, nSize );
	}
	//
	int GetSize() const { return data.pFileEnd - data.pBuffer; }
	int GetPosition() const { return data.pCurrent - data.pBuffer; }
	void SetSize( int nSize )
	{
		ASSERT( CanWrite() );
		int nPos = GetPosition();
		Seek( nSize );
		Trunc();
		nPos = (std::min)( nPos, nSize );
		Seek( nPos );
	}
	// direct buffer access
	const unsigned char* GetBuffer() const { ASSERT( CanRead() ); return ( data.nFlags & F_Broken ) != 0 ? 0 : data.pBuffer; }
	unsigned char* GetBufferForWrite() const { ASSERT( CanWrite() ); return ( data.nFlags & F_Broken ) != 0 ? 0 : data.pBuffer; }
	//
	// стандартные операции ввода/вывода
	void ReadString( std::string &res, int nMaxSize = -1 );
	void WriteString( const std::string &res );
	template<class T> CDataStream& operator>>( T &res ) { Read( &res, sizeof(res) ); return *this; }
	template<class T> CDataStream& operator<<( const T &res ) { Write( &res, sizeof(res) ); return *this; }
	bool IsOk() const { return ( data.nFlags & F_Broken ) == 0; }
	bool CanRead() const { return ( data.nFlags & F_CanRead ) != 0; }
	bool CanWrite() const { return ( data.nFlags & F_CanWrite ) != 0; }
	virtual void Flush() {}
	void ReadTo( CDataStream *pDst, unsigned int nSize );
	void WriteFrom( CDataStream &src );
	void SetBroken() { data.nFlags |= F_Broken; }
};

template<> inline CDataStream& CDataStream::operator>>( std::string &res ) { ReadString( res ); return *this; }
template<> inline CDataStream& CDataStream::operator<<( const std::string &res ) { WriteString( res ); return *this; }

class SYSTEM_EXPORT CMemoryStream : public CDataStream
{
protected:
	void CopyMemoryStream( const CMemoryStream &src );
	void AllocBuf( int nOldFileSize, int nSize );
public:
	CMemoryStream() : CDataStream( F_CanRW ) { AllocBuf( 0, 32 ); }
	~CMemoryStream();
	CMemoryStream( const CMemoryStream &src );
	CMemoryStream& operator=( const CMemoryStream &src );
	// old contents of memory stream are not preserved, current position is reset to start
	void SetSizeDiscard( int nSize );
};

class CMappedStream : public CDataStream
{
	CMappedStream( const CMappedStream &a ) : CDataStream(0) { ASSERT(0); }
	void operator=( const CMappedStream &a ) { ASSERT(0); }
	void AllocBufImpl( int nBufSize, int nPos, int nSize );
	virtual void AllocBuf( int nOldFileSize, int nSize );
	void ReleaseBuf();
protected:
	virtual int GetFileSize() = 0;
	virtual void SetFileSize( int nSize ) = 0;
	virtual void *MapFile( int nSize ) = 0;
	virtual void UnmapFile( void *p ) = 0;
	virtual void FlushFile( void *p ) = 0;

	void StartAccess( int _nFlags );
	void FinishAccess() { ReleaseBuf(); }
public:
	CMappedStream() : CDataStream(0) {}
	void Flush();
	//void CloseFile();
};

class SYSTEM_EXPORT CFileStream : public CDataStream
{
public:
	enum EWinMode { WIN_READ_ONLY, WIN_CREATE };
private:
	CDataStream *pStream;

	//
	CFileStream( const CFileStream &stream );
	void operator=( const CFileStream &stream );
public:
	// open file
	CFileStream( NVFS::IVFS *pVFS, const std::string &szFileName );
	// create file
	CFileStream( NVFS::IFileCreator *pFileCreator, const std::string &szFileName );
	// open/create win file
	CFileStream( const std::string &szFileName, const EWinMode eWinMode );
	virtual ~CFileStream();
	virtual void AllocBuf( int nOldFileSize, int nSize );
	virtual void Flush();
};

class CTextStream
{
	CDataStream &f;
public:
	typedef CTextStream& (*OpFunc)( CTextStream& );
	CTextStream( CDataStream &_f ) : f(_f) {}
	CTextStream& operator<<( const char *p ) { f.Write( p, strlen(p) ); return *this; }
	CTextStream& operator<<( int n ) { std::string tmp = std::to_string(n); f.Write( tmp.c_str(), tmp.length() ); return *this; }
	CTextStream& operator<<( double n ) { char buf[128]; gcvt( n, 7, buf ); f.Write( buf, strlen(buf) ); return *this; }
	CTextStream& operator<<( const std::string &s ) { f.Write( s.c_str(), s.length() ); return *this; }
	CTextStream& operator<<( OpFunc func ) { return func(*this); }
};
inline CTextStream& endl( CTextStream& sStream ) { sStream << "\n"; return sStream; }

class CStreamHolder
{
	CDataStream *pStream;

	CStreamHolder( const CStreamHolder &a ) {ASSERT(0);}
	void operator=( const CStreamHolder &a ) {ASSERT(0);}
public:
	CStreamHolder( CDataStream *_pStream ) : pStream(_pStream) {}
	~CStreamHolder() { delete pStream; }
	CDataStream* operator->() { return pStream; }
	const CDataStream* operator->() const { return pStream; }
	operator CDataStream*() { return pStream; }
};


