#pragma once
#include <zlib.h>

template<typename... Args>
unsigned long CalculateChecksum(unsigned long base, const Args&... args) {
    // Fold expression over all arguments
    unsigned long result = base;
    ((result = adler32(result, (unsigned char*)&args, sizeof(args))), ...);
    return result;
}