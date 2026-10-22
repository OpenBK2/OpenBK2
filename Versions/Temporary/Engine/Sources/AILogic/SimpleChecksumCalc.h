#pragma once
#include <zlib.h>

template <typename... Args>
uint32_t CalculateChecksum( uint32_t base, const Args &...args )
{
	// Fold expression over all arguments
	uint32_t result = base;
	( ( result = adler32( result, (unsigned char *)&args, sizeof( args ) ) ), ... );
	return result;
}
