#include "stdafx.h"
#include "GfxCatchGPF.h"

// flipcode COTD: x86 decoder
// http://www.flipcode.com/cgi-bin/fcmsg.cgi?thread_show=24302
static unsigned char *GetNextInstruction(unsigned char *func)
{
	// Skip prefixes F0h, F2h, F3h, 66h, 67h, D8h-DFh
	int operandSize = 4;
	int FPU = 0;
	while(*func == 0xF0 ||
		*func == 0xF2 ||
		*func == 0xF3 ||
		(*func & 0xFE) == 0x66 ||
		(*func & 0xF8) == 0xD8)
	{
		if(*func == 0x66)
		{
			operandSize = 2;
		}
		else if((*func & 0xF8) == 0xD8)
		{
			FPU = *func++;
			break;
		}

		func++;
	}

	// Skip two-byte opcode byte
	bool twoByte = false;
	if(*func == 0x0F)
	{
		twoByte = true;
		func++;
	}

	// Skip opcode byte
	unsigned char opcode = *func++;

	// Skip mod R/M byte, undo when not required
	unsigned char modRM = 0xFF;
	if(FPU)
	{
		if((opcode & 0xC0) != 0xC0)
		{
			modRM = opcode;
		}
	}
	else if(!twoByte)
	{
		if((opcode & 0xC4) == 0x00 ||
			(opcode & 0xF4) == 0x60 && ((opcode & 0x0A) == 0x02 || (opcode & 0x09) == 0x9) ||
			(opcode & 0xF0) == 0x80 ||
			(opcode & 0xF8) == 0xC0 && (opcode & 0x0E) != 0x02 ||
			(opcode & 0xFC) == 0xD0 ||
			(opcode & 0xF6) == 0xF6)
		{
			modRM = *func++;
		}
	}
	else
	{
		if((opcode & 0xF0) == 0x00 && (opcode & 0x0F) >= 0x04 && (opcode & 0x0D) != 0x0D ||
			(opcode & 0xF0) == 0x30 ||
			opcode == 0x77 ||
			(opcode & 0xF0) == 0x80 ||
			(opcode & 0xF0) == 0xA0 && (opcode & 0x07) <= 0x02 ||
			(opcode & 0xF8) == 0xC8)
		{
			// No mod R/M byte
		}
		else
		{
			modRM = *func++;
		}
	}

	// Skip SIB and displacement
	if((modRM & 0x07) == 0x04) func += 1;	// SIB
	if((modRM & 0xC5) == 0x05) func += 4;	// Dword displacement, no base
	if((modRM & 0xC0) == 0x40) func += 1;	// Byte displacement
	if((modRM & 0xC0) == 0x80) func += 4;	// Dword displacement

	// Skip immediate
	if(FPU)
	{
		// Can't have immediate operand
	}
	else if(!twoByte)
	{
		if((opcode & 0xC7) == 0x04 ||
			(opcode & 0xFE) == 0x6A ||	// PUSH/POP/IMUL
			(opcode & 0xF0) == 0x70 ||	// Jcc
			opcode == 0x80 ||
			opcode == 0x83 ||
			(opcode & 0xFD) == 0xA0 ||	// MOV
			opcode == 0xA8 ||		// TEST
			(opcode & 0xF8) == 0xB0 ||	// MOV
			(opcode & 0xFE) == 0xC0 ||	// RCL
			opcode == 0xC6 ||		// MOV
			opcode == 0xCD ||		// INT
			(opcode & 0xFE) == 0xD4 ||	// AAD/AAM
			(opcode & 0xF8) == 0xE0 ||	// LOOP/JCXZ
			opcode == 0xEB ||
			opcode == 0xF6 && (modRM & 0x30) == 0x00)	// TEST
		{
			func += 1;
		}
		else if((opcode & 0xF7) == 0xC2)
		{
			func += 2;   // RET
		}
		else if((opcode & 0xFC) == 0x80 ||
			(opcode & 0xC7) == 0x05 ||
			(opcode & 0xF8) == 0xB8 ||
			(opcode & 0xFE) == 0xE8 ||		// CALL/Jcc
			(opcode & 0xFE) == 0x68 ||
			(opcode & 0xFC) == 0xA0 ||
			(opcode & 0xEE) == 0xA8 ||
			opcode == 0xC7 ||
			opcode == 0xF7 && (modRM & 0x30) == 0x00)
		{
			func += operandSize;
		}
	}
	else
	{
		if(opcode == 0xBA ||		// BT
			opcode == 0x0F ||		// 3DNow!
			(opcode & 0xFC) == 0x70 ||	// PSLLW
			(opcode & 0xF7) == 0xA4 ||	// SHLD
			opcode == 0xC2 ||
			opcode == 0xC4 ||
			opcode == 0xC5 ||
			opcode == 0xC6)
		{
			func += 1;
		}
		else if((opcode & 0xF0) == 0x80)
		{
			func += operandSize;   // Jcc -i
		}
	}
	return func;
}

static bool bWasInitialized = false;
static LPTOP_LEVEL_EXCEPTION_FILTER pPrevExceptionFilter = 0;
struct SRegion
{
	const void *pData;
	int nSize;
	SRegion() : pData(0), nSize(0) {}
	SRegion( const void *_pData, int _nSize ) : pData(_pData), nSize(_nSize) {}
};
inline bool operator==( const SRegion &a, const SRegion &b ) { return a.pData == b.pData && a.nSize == b.nSize; }
struct SRegionHash
{
	int operator()( const SRegion &r ) const { return (int)r.pData; }
};
typedef hash_map<SRegion,int,SRegionHash> TIgnoreHash;
static TIgnoreHash ignoreRegions;

static LONG WINAPI SkipWrongAddress( _EXCEPTION_POINTERS* ExceptionInfo )
{
	if ( ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION )
	{
		DWORD dwAddress = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
		bool bIgnore = false;
		for ( TIgnoreHash::const_iterator i = ignoreRegions.begin(); i != ignoreRegions.end(); ++i )
		{
			const SRegion &r = i->first;
			DWORD	dwStart = (DWORD)r.pData, dwFinish = dwStart + r.nSize;
			if ( dwAddress >= dwStart && dwAddress < dwFinish )
				bIgnore = true;
		}
		if ( bIgnore )
		{
			DWORD &rEIP = ExceptionInfo->ContextRecord->Eip;
			rEIP = (DWORD)GetNextInstruction( (unsigned char*)rEIP );
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	//if ( ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP )
	if ( pPrevExceptionFilter )
		return pPrevExceptionFilter( ExceptionInfo );
	return UnhandledExceptionFilter( ExceptionInfo );
}

void InitCatchGPF()
{
	if ( bWasInitialized )
		return;
	bWasInitialized = true;
	pPrevExceptionFilter = SetUnhandledExceptionFilter( SkipWrongAddress );
}

void AddIgnoreAccessViolationRegion( const void *pStart, int nSize )
{
	SRegion r( pStart, nSize );
	++ignoreRegions[r];
}

void RemoveIgnoreAccessViolationRegion( const void *pStart, int nSize )
{
	SRegion r( pStart, nSize );
	TIgnoreHash::iterator i = ignoreRegions.find( r );
	if ( i != ignoreRegions.end() )
	{
		if ( --i->second == 0 )
			ignoreRegions.erase( i );
	}
	else
		ASSERT(0);
}

