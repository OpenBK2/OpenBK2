// Cruncher.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "Cruncher.h"

int CNetCompressor::nLengthBits[34], CNetCompressor::nLengthBitsNum[34];
int CNetCompressor::nShiftBits[34], CNetCompressor::nShiftBitsNum[34];

struct SNetCompressorInit
{
	SNetCompressorInit()
	{
		int i;
		// initialize length codes
		for ( i = 0; i < 34; i++ )
		{
			int nLeng;
			int &nBits = CNetCompressor::nLengthBits[i];
			int &nBitsNum = CNetCompressor::nLengthBitsNum[i];
			nBits = 0;
			nLeng = i + 1;
			if ( nLeng <= 1 )
			{ nBits = 0; nBitsNum = 1; }
			else if ( nLeng <= 2 )
			{ nBits = 1; nBitsNum = 3; }
			else if ( nLeng <= 3 )
			{ nBits = 3; nBitsNum = 5; }
			else if ( nLeng <= 4 )
			{ nBits = 5; nBitsNum = 3; }
			else if ( nLeng <= 5 )
			{ nBits = 7; nBitsNum = 5; }
			else if ( nLeng <= 6 )
			{ nBits = 15; nBitsNum = 5; }
			else if ( nLeng <= 10 )
			{ nBits = 11; nBitsNum = 4 + 2; nBits += (nLeng - 7) << 4;}
			else if ( nLeng <= 18 )
			{ nBits = 19; nBitsNum = 5 + 3; nBits += (nLeng - 11) << 5;}
			else if ( nLeng <= 34 )
			{ nBits = 31; nBitsNum = 6 + 4; nBits += (nLeng - 19) << 6;}
			/*else if ( nLeng <= 34+64 )
			else if ( nLeng <= 34+64+128 )*/
		}
		// initialize shift codes
		for ( i = 0; i < 34; i++ )
		{
			int nShift = i;
			int &nBits = CNetCompressor::nShiftBits[i];
			int &nBitsNum = CNetCompressor::nShiftBitsNum[i];
			nBits = 0;
			if ( nShift < 2 )
			{ nBits = 28; nBitsNum = 5 + 1; nBits += (nShift - 0) << 5; }
			else if ( nShift < 6 )
			{ nBits = 8;  nBitsNum = 4 + 2; nBits += (nShift - 2) << 4; }
			else if ( nShift < 10 )
			{ nBits = 4;  nBitsNum = 4 + 2; nBits += (nShift - 6) << 4; }
			else if ( nShift < 18 )
			{ nBits = 0;  nBitsNum = 4 + 3; nBits += (nShift - 10) << 4; }
			else if ( nShift < 34 )
			{ nBits = 12; nBitsNum = 5 + 4; nBits += (nShift - 18) << 5; }
		}
	}
};
static SNetCompressorInit sNetCompressorInit;

CNetCompressor::CNetCompressor() 
{ 
	cPrevLetter = 0; nBlockStart = -1; 
	nCurrent = 0; 
	nNext[0] = N_COMPRESS_HISTORY;
	memset( nHashTable, 0, sizeof(nHashTable) ); 
	cBuffer[0] = 0; cBuffer[N_COMPRESS_HISTORY] = 0; 
	cBuffer[N_COMPRESS_HISTORY - 1] = 0;
	cBuffer[2*N_COMPRESS_HISTORY - 1] = 0;

	/*memset( nBlocks, 0, sizeof( nBlocks ) );
	memset( nLengths, 0, sizeof( nLengths ) );
	nCSize = 0;*/
}

void CNetCompressor::EmitBlock( int nStart )
{
	int nShift = nCurrent - nStart - nBlockLength - 1;
	nShift &= N_COMPRESS_HISTORY - 1;
	//cout << "Block, shift = " << nShift << "   length = " << nBlockLength << endl;
	nBlockStart = -1;
	
	if ( nBlockLength <= 34 )
		data.WriteBits( nLengthBits[ nBlockLength - 1 ], nLengthBitsNum[ nBlockLength - 1 ] );
	else
	{
		if ( nBlockLength <= 34+64 )
		{
			data.WriteBits( 23, 5 );
			data.WriteBits( (nBlockLength - 35), 6 );
		}
		else if ( nBlockLength <= 34+64+128 )
		{
			data.WriteBits( 63, 6 );
			data.WriteBits( (nBlockLength - 99), 7 );
		}
		else
			ASSERT(0);
	}
	if ( nShift < 34 )
		data.WriteBits( nShiftBits[ nShift ], nShiftBitsNum[ nShift ] );
	else
	{
		if ( nShift < 34 + 64 )
		{
			data.WriteBits( 2, 3 );
			data.WriteBits( (nShift - 34), 6 );
		}
		else if ( nShift < 34 + 64 + 256 )
		{
			data.WriteBits( 6, 3 );
			data.WriteBits( (nShift - (34 + 64) ), 8 );
		}
		else if ( nShift < 34 + 64 + 256 + 1024 )
		{
			data.WriteBits( 3, 2 );
			data.WriteBits( (nShift - (34 + 64 + 256) ), 10 );
		}
		else if ( nShift < 34 + 64 + 256 + 1024 + 4096 )
		{
			data.WriteBits( 1, 2 );
			data.WriteBits( (nShift - (34 + 64 + 256 + 1024) ), 12 );
		}
	}
/*
	// statistics
	if ( nBlockLength <= 2 )
		{ nBlocks[1]++; nCSize += 3; }
	else if ( nBlockLength <= 3 )
		{ nBlocks[2]++; nCSize += 5; }
	else if ( nBlockLength <= 4 )
		{ nBlocks[3]++; nCSize += 3; }
	else if ( nBlockLength <= 5 )
		{ nBlocks[4]++; nCSize += 5; }
	else if ( nBlockLength <= 6 )
		{ nBlocks[5]++; nCSize += 5; }
	else if ( nBlockLength <= 10 )
		{ nBlocks[6]++; nCSize += 4 + 2; }
	else if ( nBlockLength <= 18 )
		{ nBlocks[7]++; nCSize += 5 + 4; }
	else if ( nBlockLength <= 34 )
		{ nBlocks[8]++; nCSize += 6 + 4; }
	else if ( nBlockLength <= 34 + 64 )
		{ nBlocks[9]++; nCSize += 5 + 6; }
	else if ( nBlockLength <= 34 + 64 + 128 )
		{ nBlocks[10]++; nCSize += 6 + 7; }
	if ( nShift == nBlockLength - 1 )
		{ nLengths[9]++; nCSize += 5; }
	else if ( nShift < 2 )
		{ nLengths[0]++; nCSize += 5 + 1; }
	else if ( nShift < 6 )
		{ nLengths[1]++; nCSize += 4 + 2; }
	else if ( nShift < 10 )
		{ nLengths[2]++; nCSize += 5 + 2; }
	else if ( nShift < 18 )
		{ nLengths[3]++; nCSize += 4 + 3; }
	else if ( nShift < 34 )
		{ nLengths[4]++; nCSize += 5 + 4; }
	else if ( nShift < 34+64 )
		{ nLengths[5]++; nCSize += 3 + 6; }
	else if ( nShift < 34+64+256 )
		{ nLengths[6]++; nCSize += 3 + 8; }
	else if ( nShift < 34+64+256+1024 )
		{ nLengths[7]++; nCSize += 2 + 10; }
	else if ( nShift < 34+64+256+1024+4096 )
		{ nLengths[8]++; nCSize += 2 + 12; }*/
}

void CNetCompressor::EmitChar()
{
	//cout << "Char = " << cPrevLetter << endl;
	nBlockStart = -1;
	data.WriteBit(0);
	data.Write( &cPrevLetter, 1 );
	//nBlocks[0]++; nCSize += 1 + 8;
}

void CNetCompressor::FinishPack()
{
	if ( nBlockStart >= 0 )
		EmitBlock( nBlockStart - 1 );
	else
		EmitChar();
}

void CNetCompressor::StartPack( unsigned char c )
{
	// fill hash table
	nNext[ nCurrent ] = N_COMPRESS_HISTORY;//nCurrent;
	// advance nCurrent
	nCurrent = ( nCurrent + 1 ) & ( N_COMPRESS_HISTORY - 1 );
	// write down new code
	cBuffer[ nCurrent ] = c;
	cBuffer[ nCurrent + N_COMPRESS_HISTORY ] = c;
	cPrevLetter = c;
}

void CNetCompressor::StorePack( CDataStream &src, CDataStream &appendTo )
{
	CBitLocker srcData;

	src.Seek(0);
	int nSize = src.GetSize();
	srcData.LockRead( src, nSize );
	const void *pData = srcData.GetCurrentPtr();

	const unsigned char *pszData = (const unsigned char*) pData;
	const unsigned char *pszDataFinish = pszData + nSize;
	data.LockWrite( appendTo, nSize * 9 / 8 + 10 );
	data.Write( &nSize, 4 );
	if ( nSize <= 0 )
	{
		data.Free();
		return;
	}
	for ( ; pszData < pszDataFinish; pszData++ )
	{
		data.WriteBit(0);
		data.Write( pszData, 1 );
	}
	data.Free();
}

void CNetCompressor::Pack( CDataStream &src, CDataStream &appendTo )
{
	const void *pData;
	int nSize;
	unsigned int nHash;
	int nDif, nPrevious;
	unsigned char c;
	unsigned int nC; // for compiler to generate better code
	unsigned char *pCompareSrc;
	const unsigned char *pszData;
	const unsigned char *pszDataFinish;
	CBitLocker srcData;

	src.Seek(0);
	nSize = src.GetSize();
	srcData.LockRead( src, nSize );
	pData = srcData.GetCurrentPtr();

	pszData = (const unsigned char*) pData;
	pszDataFinish = pszData + nSize;
	data.LockWrite( appendTo, nSize * 9 / 8 + 10 );
	data.Write( &nSize, 4 );
	if ( nSize <= 0 )
	{
		data.Free();
		return;
	}
	StartPack( *pszData++ );
	for ( ; pszData < pszDataFinish; pszData++, cPrevLetter = nC )
	{
		nC = *pszData;
		// calc hash value of prevletter & current one
		nHash = cPrevLetter;
		nHash = ( nHash << 2 ) ^ nC;
		// fill hash table
		nDif = ( nCurrent - nHashTable[ nHash ] ) & (N_COMPRESS_HISTORY-1);
		nHashTable[ nHash ] = nCurrent;
		nDif += N_COMPRESS_HISTORY * ( nDif == 0 );
		nNext[ nCurrent ] = nDif;//nHashTable[ nHash ];
		// advance nCurrent
		nPrevious = nCurrent;
		nCurrent = ( nCurrent + 1 ) & ( N_COMPRESS_HISTORY - 1 );
		// write down new code
		c = nC;
		cBuffer[ nCurrent ] = c;
		cBuffer[ nCurrent + N_COMPRESS_HISTORY ] = c;
		// если в рассмотрении находится какая-либо серия
		if ( nBlockStart < 0 )
		{
			short nCmpWith;
			int nShift, nNextCandidate;
			// попытаемся положить начало новой серии
			nCmpWith = *((short*)( pszData - 1 ));
			nBlockStart = nPrevious;
			// lets try candidates
			nShift = nDif;
			while ( nShift < N_COMPRESS_HISTORY - N_COMPRESS_MAX_BLOCK - 1 )
			{
				nNextCandidate = ( nBlockStart - nShift ) & (N_COMPRESS_HISTORY-1);
				if ( *((short*)(cBuffer + nNextCandidate)) == nCmpWith )
				{
					nBlockStart = nNextCandidate;
					nBlockLength = 2;
					goto Done2;
				}
				nShift += nNext[ nNextCandidate ];
			}
			EmitChar();
	Done2:
			;
		}
		else
		{
			if ( nBlockLength < N_COMPRESS_MAX_BLOCK )
			{
				if ( cBuffer[ nBlockLength + nBlockStart ] == c )
					nBlockLength++;
				else
				{
					int nRelStart, nShift, nShift1;
					nRelStart = nCurrent - nBlockLength;
					nShift = ( nRelStart - nBlockStart ) & (N_COMPRESS_HISTORY-1);
					nShift += nNext[ nBlockStart ];

					nShift1 = nDif;
					pCompareSrc = cBuffer + ( nRelStart & (N_COMPRESS_HISTORY - 1));
					while ( nShift < N_COMPRESS_HISTORY - N_COMPRESS_MAX_BLOCK - 1 )
					{
						while ( nShift > nShift1 )
							nShift1 += nNext[ ( nPrevious - nShift1 ) & (N_COMPRESS_HISTORY-1) ];
						if ( nShift < nShift1 )
							nShift += nNext[ ( nRelStart - nShift ) & (N_COMPRESS_HISTORY-1) ];
						else
						{
							int nNextCandidate = ( nRelStart - nShift ) & (N_COMPRESS_HISTORY-1);
							if ( memcmp( cBuffer + nNextCandidate, pCompareSrc, nBlockLength + 1 ) == 0 )
							{
								nBlockStart = nNextCandidate;
								nBlockLength++;
								goto Done;
							}
							nShift += nNext[ nNextCandidate ];
							nShift1 += nNext[ ( nPrevious - nShift1 ) & (N_COMPRESS_HISTORY-1) ];
						}
					}
					EmitBlock( nBlockStart );
			Done:
					;
				}
			}
			else
				EmitBlock( nBlockStart );
		}
	}
	FinishPack();
	data.Free();
}

template<class T>
T __Min( const T &a, const T &b ) { return a < b ? a : b; }
//
void CNetCompressor::Unpack( CDataStream &src, CDataStream &appendTo )
{
	CBitLocker load;
	load.LockRead( src, src.GetSize() - src.GetPosition() );
	int nDataLeft;
	load.Read( &nDataLeft, 4 );
	data.LockWrite( appendTo, nDataLeft );
	for(; nDataLeft > 0;)
	{
		if ( load.ReadBit() )
		{
			int nBlockLength, nShift;
			if ( load.ReadBit() )
			{
				if ( load.ReadBit() )
				{
					if ( load.ReadBit() )
					{
						if ( load.ReadBit() )
						{
							if ( load.ReadBit() )
								nBlockLength = 99 + load.ReadBits(7); // <=34+64+128
							else
								nBlockLength = 19 + load.ReadBits(4); // <=34
						}
						else
							nBlockLength = 6;
					}
					else
					{
						if ( load.ReadBit() )
							nBlockLength = 35 + load.ReadBits(6); // <=34+64
						else
							nBlockLength = 5;
					}
				}
				else
				{
					if ( load.ReadBit() )
						nBlockLength = 7 + load.ReadBits(2); // <=10
					else
					{
						if ( load.ReadBit() )
							nBlockLength = 11 + load.ReadBits(3); // <=18
						else
							nBlockLength = 3;
					}
				}
			}
			else
			{
				if ( load.ReadBit() )
					nBlockLength = 4;
				else
					nBlockLength = 2;
			}
			// shift load
			if ( load.ReadBit() )
			{
				if ( load.ReadBit() )
					nShift = 34 + 64 + 256 + load.ReadBits(10);
				else
					nShift = 34 + 64 + 256 + 1024 + load.ReadBits(12);
			}
			else
			{
				if ( load.ReadBit() )
				{
					if ( load.ReadBit() )
						nShift = 34 + 64 + load.ReadBits(8);
					else
						nShift = 34 + load.ReadBits(6);
				}
				else
				{
					if ( load.ReadBit() )
					{
						if ( load.ReadBit() )
						{
							if ( load.ReadBit() )
								nShift = 0 + load.ReadBits(1); // <2
							else
								nShift = 18 + load.ReadBits(4); // <34
						}
						else
							nShift = 6 + load.ReadBits(2); // <10
					}
					else
					{
						if ( load.ReadBit() )
							nShift = 2 + load.ReadBits(2); // <6
						else
							nShift = 10 + load.ReadBits(3); // <18
					}
				}
			}

			ASSERT( nBlockLength <= nDataLeft );
			nBlockLength = __Min( nBlockLength, nDataLeft );
			for ( int i = 0; i < nBlockLength; i++ )
			{
				int nPos = ( nCurrent - nShift - 1 ) & ( N_COMPRESS_HISTORY - 1 );
				char c = cBuffer[nPos];
				data.Write( &c, 1 );
				cBuffer[ nCurrent ] = c;
				nCurrent = ( nCurrent + 1 ) & ( N_COMPRESS_HISTORY - 1 );
			}
			nDataLeft -= nBlockLength;
		}
		else
		{
			char c;
			load.Read( &c, 1 );
			data.Write( &c, 1 );
			nDataLeft--;
			cBuffer[ nCurrent ] = c;
			nCurrent = ( nCurrent + 1 ) & ( N_COMPRESS_HISTORY - 1 );
		}
	}
	data.Free();
}

