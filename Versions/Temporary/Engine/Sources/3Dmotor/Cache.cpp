#include "StdAfx.h"
#include "Cache.h"

namespace NCache
{
int nFibbonachiSeries[N_MAX_FIB];
struct SCacheInit
{
	SCacheInit() 
	{
		nFibbonachiSeries[0] = nFibbonachiSeries[1] = 1;
		for ( int i = 2; i < N_MAX_FIB; ++i )
			nFibbonachiSeries[i] = nFibbonachiSeries[i-1] + nFibbonachiSeries[i-2];
		for ( int i = 1; i < N_MAX_FIB; ++i )
			nFibbonachiSeries[i-1] = nFibbonachiSeries[i];
	}
} init;
}

