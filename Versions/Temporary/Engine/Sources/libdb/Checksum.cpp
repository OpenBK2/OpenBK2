#include "stdafx.h"

#include "Checksum.h"

#include <cstdint>

#include <zlib.h>

namespace NDb
{
uint32_t GetDefaultCheckSum()
{
	return adler32(0L, Z_NULL, 0);
}

uint32_t CalcCheckSum( const uint32_t dwLastCheckSum, const uint8_t *pBuf, const int nLen )
{
	return adler32( dwLastCheckSum, pBuf, nLen );
}
}


