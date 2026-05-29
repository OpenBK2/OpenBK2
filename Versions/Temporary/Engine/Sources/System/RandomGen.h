#pragma once

#include "System_export.h"


struct IRandomSeed : public CObjectBase
{
	// re-initialize random seed
	virtual void Init() = 0;
	virtual void InitByZeroSeed() = 0;
	// store and restore binary data in the stream form (for non-structure-saver usage)
	virtual void Store( CDataStream *pStream ) = 0;
	virtual void Restore( CDataStream *pStream ) = 0;
	// serialize to XML
	virtual int operator&( struct IXmlSaver &saver ) = 0;
};

namespace NRandom
{
	struct SDebugState
	{
		unsigned int randcnt;
		unsigned int randa;
		unsigned int randb;
		unsigned int randc;
		unsigned long randrslChecksum;
		unsigned long randmemChecksum;
		unsigned __int64 randomCalls;
	};

	struct RngCall
	{
		int callNumber = 0;
		const char* file;
		int line;
		const char* function;
	};

	// Keep raw macro pointers here: wrapping __FILE__/__func__ in temporary strings leaves dangling c_str() pointers in the ring buffer.
	SYSTEM_EXPORT bool RecordCall(const char* file, int line, const char* func);
	SYSTEM_EXPORT void DumpRecords(FILE* f);

	// initialize random generator with random seed
	SYSTEM_EXPORT void SetRandomSeed( IRandomSeed *pSeed );
	// create copy of the current random gen seed and return it
	SYSTEM_EXPORT IRandomSeed *CreateRandomSeedCopy();
	// copy compact debug info for ASYNC diagnostics without consuming RNG values
	SYSTEM_EXPORT void GetDebugState( SDebugState *pState );
	// get random value
	SYSTEM_EXPORT UINT Random();
	// random w/o checks
	__forceinline unsigned int Random( const unsigned int uMax ) { return Random() % uMax; }
	__forceinline int Random( const int nMin, const int nMax ) { return nMin + (int)Random( (unsigned int)(nMax - nMin + 1) ); }
	__forceinline float Random( const float fMin, const float fMax ) { return fMin + float( double(Random()) / double(0xffffffffUL) * double(fMax - fMin) ); }
	// random with checks
	__forceinline unsigned int RandomCheck( const unsigned int uMax ) { return uMax == 0 ? 0 : Random( uMax ); }
	__forceinline int RandomCheck( const int nMin, const int nMax ) { return nMax < nMin ? nMin : Random( nMin, nMax ); }
	__forceinline int RandomCheck( const float fMin, const float fMax ) { return fMax < fMin ? fMin : Random( fMin, fMax ); }

	struct SRandomFunc
	{
		float operator()( float fMin, float fMax ) const { return Random( fMin, fMax ); }
	};
	const SRandomFunc& RndFunc();
};

#define RecordRandomCall() NRandom::RecordCall(__FILE__, __LINE__, __func__)
