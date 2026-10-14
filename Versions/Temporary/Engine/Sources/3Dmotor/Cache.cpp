#include "stdafx.h"
#include "Cache.h"

#include <climits>
#include <cstdint>

namespace NCache
{
int nFibbonachiSeries[N_MAX_FIB];
struct SCacheInit
{
	SCacheInit()
	{
		nFibbonachiSeries[0] = nFibbonachiSeries[1] = 1;
		for ( int i = 2; i < N_MAX_FIB; ++i )
		{
			// Saturating rather than wrapping. The series passes INT_MAX at the
			// forty seventh term while the table runs to sixty four, so the tail
			// overflowed a signed int, which is undefined and in practice came out
			// negative. Negative terms are worse than useless here: GetMajorFib
			// walks up the table until a term reaches the size being asked for, and
			// a negative term never does, so the walk would run off the end of the
			// table instead of stopping. Clamping keeps the tail monotonic and
			// larger than any size that can be asked for, and every term that fit
			// before is unchanged.
			const int64_t nSum = static_cast<int64_t>( nFibbonachiSeries[i - 1] ) + nFibbonachiSeries[i - 2];
			nFibbonachiSeries[i] = nSum > INT_MAX ? INT_MAX : static_cast<int>( nSum );
		}
		for ( int i = 1; i < N_MAX_FIB; ++i )
			nFibbonachiSeries[i - 1] = nFibbonachiSeries[i];
	}
} init;
}
