#include "stdafx.h"

#include "MemoryLib_export.h"

struct SAlloc
{
	DWORD dwAddress;
	int nSize;
};
struct SPtrHash
{
	int operator()( void *p ) const { return (int)p; }
};

MEMORYLIB_EXPORT void DumpMemoryStats()
{
}
