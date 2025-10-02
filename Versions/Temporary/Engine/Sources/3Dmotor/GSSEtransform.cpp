#include "StdAfx.h"
#include "GPixelFormat.h"
bool bIsSSEPresent = ( GetCPUID() & CPUID_SSE_FEATURE_PRESENT ) != 0;

short nNormalizeTable[16384];
NGfx::SMMXWord mmxWeights[512];
unsigned char nCubicRoot[32768];
static struct SNormalizeInit
{
	SNormalizeInit() 
	{
		nNormalizeTable[0] = 0x7fff;
		for ( int k = 1; k < ARRAY_SIZE(nNormalizeTable); ++k )
			nNormalizeTable[k] = Min( 0x7fff, Float2Int( (64 * (127 * 16)) / sqrt( k + 0.99f ) ) );
		for ( int k = 0; k < ARRAY_SIZE(mmxWeights); ++k )
		{
			NGfx::SMMXWord &a = mmxWeights[k];
			a.nZ = a.nY = a.nX = a.nW = k << 6;
		}
		for ( int k = 0; k < 16384; ++k )
		{
			float f = k * 1.0f / 16384;
			nCubicRoot[k] = Float2Int( exp( log(f) / 3 ) * 255 );
			f = f / 16384;
			nCubicRoot[k + 16384] = Float2Int( exp( log(f) / 3 ) * 255 );
		}
	}
} normalizeInit;

