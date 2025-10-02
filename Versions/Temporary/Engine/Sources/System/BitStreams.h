#pragma once

#include "System_export.h"


// класс для последовательной записи/считывания данных, включая возможность записи
// или считывания побитных данных, может использоваться на произовольных областях
// памяти
class CBitStream
{
public:
	enum Mode
	{
		read,
		write
	};

protected:
	unsigned char *pCurrent;
	unsigned char *pBitPtr;         // for bit writing
	unsigned int nBits;
	unsigned char nBitsCount; // bits and bit counter
	static unsigned int nBitsMask[32];

#ifdef _DEBUG
	Mode mode;
	unsigned char *pReservedEnd;
	void CheckCurrentR() { ASSERT( pCurrent <= pReservedEnd ); ASSERT( mode == read ); }
	void CheckCurrentW() { ASSERT( pCurrent <= pReservedEnd ); ASSERT( mode == write ); }
#else
	void CheckCurrentR() {}
	void CheckCurrentW() {}
#endif

	inline void Init( unsigned char *pData, Mode _mode, int nSize );
public:
	CBitStream( void *pData, Mode _mode, int nSize ) { Init( (unsigned char*)pData, _mode, nSize ); }
	// result of read/write beyond data range is not determined
	void Read( void *pDest, unsigned int nSize ) { memcpy( pDest, pCurrent, nSize ); pCurrent += nSize; CheckCurrentR(); }
	void Write( const void *pSrc, unsigned int nSize ) { memcpy( pCurrent, pSrc, nSize ); pCurrent += nSize; CheckCurrentW(); }
	void ReadCString( string &res ) { int nLeng = strlen( (char*)pCurrent ); res.assign( (char*)pCurrent, nLeng ); pCurrent += nLeng + 1; CheckCurrentR(); }
	void WriteCString( const char *pSrc ) { int nLeng = strlen( pSrc ); memcpy( pCurrent, pSrc, nLeng + 1 ); pCurrent += nLeng + 1; CheckCurrentW(); }
	void FlushBits() { if ( nBitsCount ) { nBitsCount = 0; if ( pBitPtr ) pBitPtr[0] = (char)nBits; } }
	// not more then 24 bits per call
	inline void WriteBits( unsigned int _nBits, unsigned int _nBitsCount );
	inline void WriteBit( unsigned int _nBits );
	inline unsigned int ReadBits( unsigned int _nBitsCount );
	inline unsigned int ReadBit();
	// even more direct access, try to not use it, read only
	const unsigned char* GetCurrentPtr() const { return pCurrent; }
	// get pointer to place to write to later (not later then this object will be destructed)
	unsigned char* WriteDelayed( int nSize ) { unsigned char *pRes = pCurrent; pCurrent += nSize; CheckCurrentW(); return pRes; }
	//
	template <class T> inline void Write( const T &a ) { Write( &a, sizeof(a) ); }
	template <class T> inline void Read( T &a ) { Read( &a, sizeof(a) ); }
	template<> inline void Write<string>( const string &a ) { WriteCString( a.c_str() ); }
	template<> inline void Read<string>( string &a ) { ReadCString( a ); }

	friend class CBitEmbedded;
};

// класс для выполнения побитного и скоростного ввода/вывода в поток общего назначения
// после того, как с CDataStream начинает работать CBitLocker прямые операции с 
// DataStream приведут к некорректному результату
class CDataStream;
class SYSTEM_EXPORT CBitLocker: public CBitStream
{
	CDataStream *pData;
	unsigned char *pBuffer;
public:
	CBitLocker(): CBitStream( 0, read, 0 ) { pData = 0; }
	~CBitLocker();
	// once per life of this object
	void LockRead( CDataStream &data, unsigned int nSize );
	void LockWrite( CDataStream &data, unsigned int nSize );
	// alloc additional buffer space, for better perfomance minimize number of this 
	// function calls
	void ReserveRead( unsigned int nSize );
	void ReserveWrite( unsigned int nSize );
	void Free();
};

class CBitEmbedded: public CBitStream
{
	CBitStream &bits;
public:
	CBitEmbedded( CBitStream &_bits ): 
#ifdef _DEBUG
			CBitStream( _bits.pCurrent, _bits.mode, _bits.pReservedEnd - _bits.pCurrent )
#else
			CBitStream( _bits.pCurrent, read, 0 )
#endif
				,bits(_bits) {}
				~CBitEmbedded() { bits.pCurrent = pCurrent; }
};


// CBitStream realization

inline void CBitStream::Init( unsigned char *pData, Mode _mode, int nSize )
{
	pCurrent = pData; nBitsCount = 0; pBitPtr = 0;
#ifdef _DEBUG
	mode = _mode;
	pReservedEnd = pCurrent + nSize;
#endif
}

inline void CBitStream::WriteBits( unsigned int _nBits, unsigned int _nBitsCount )
{
	if ( nBitsCount != 0 )
	{
		nBits += ( _nBits << nBitsCount );
		nBitsCount += _nBitsCount;
	}
	else
	{
		pBitPtr = pCurrent++;
		nBits = _nBits;
		nBitsCount = _nBitsCount;
	}
	while ( nBitsCount > 8 )
	{
		pBitPtr[0] = (unsigned char)nBits; //( nBits & 0xff );
		nBits >>= 8; nBitsCount -= 8;
		pBitPtr = pCurrent++;
	}
	CheckCurrentW();
}

inline void CBitStream::WriteBit( unsigned int _nBits )
{
	if ( nBitsCount == 0 )
	{
		pBitPtr = pCurrent++;
		nBits = _nBits;
		nBitsCount = 1;
	}
	else
	{
		nBits += ( _nBits << nBitsCount );
		nBitsCount++;
	}
	if ( nBitsCount > 8 )
	{
		pBitPtr[0] = (unsigned char)nBits; //( nBits & 0xff );
		nBits >>= 8; nBitsCount -= 8;
		pBitPtr = pCurrent++;
	}
	CheckCurrentW();
}

inline unsigned int CBitStream::ReadBits( unsigned int _nBitsCount )
{
	while ( nBitsCount < _nBitsCount )
	{
		nBits += ((unsigned int)*pCurrent++) << nBitsCount;
		nBitsCount += 8;
	}
	int nRes = nBits & nBitsMask[ _nBitsCount - 1 ];
	nBits >>= _nBitsCount;
	nBitsCount -= _nBitsCount;
	CheckCurrentR();
	return nRes;
}

inline unsigned int CBitStream::ReadBit()
{
	if ( nBitsCount < 1 )
	{
		nBits = ((unsigned int)*pCurrent++);
		nBitsCount = 8;
	}
	int nRes = nBits & 1;
	nBits >>= 1;
	nBitsCount--;
	CheckCurrentR();
	return nRes;
}

