#pragma once

#include "BitStreams.h"

const int N_COMPRESS_MAX_BLOCK = 34 + 64 + 128 - 1;//;256; // must be less then half of history
const int N_COMPRESS_HISTORY = 1024;

class CNetCompressor
{
	unsigned char cBuffer[ N_COMPRESS_HISTORY * 2 ];//+ N_COMPRESS_MAX_BLOCK ];
	unsigned int nNext[ N_COMPRESS_HISTORY ];  // list of same pairs
	unsigned int nHashTable[ 1024 ];//8192 ];
	CBitLocker data;
	int nCurrent;       // позиция на место которой должна быть записана текущая буква
	int nBlockStart;    // позиция начала блока повторения
	int nBlockLength;   // длина текущего блока повторения
	//unsigned char cPrevLetter;
	unsigned int cPrevLetter;
	//
	static int nLengthBits[34], nLengthBitsNum[34];
	static int nShiftBits[34], nShiftBitsNum[34];
	static void Initialize();

	void EmitBlock( int nBlock );
	void EmitChar();
	void StartPack( unsigned char c );
	void FinishPack();
public:
/*	int nBlocks[11];
	int nLengths[11];
	int nCSize;*/

	CNetCompressor();
	void Clear() { CNetCompressor::~CNetCompressor(); ::new(this) CNetCompressor;}
	void Pack( CDataStream &src, CDataStream &appendTo );
	void StorePack( CDataStream &src, CDataStream &appendTo );
	void Unpack( CDataStream &src, CDataStream &appendTo );

	friend struct SNetCompressorInit;
};
