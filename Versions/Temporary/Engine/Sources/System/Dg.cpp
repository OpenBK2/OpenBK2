#include "stdafx.h"
#include "Dg.h"
#include "Misc/Pool.h"

#include "System_export.h"

#ifdef _MAPEDIT
const int N_HOLD_TIME = 16384; // must be power of 2
#else
const int N_HOLD_TIME = 256; // must be power of 2
#endif
int nDGCurrentFrame = 100;
// Unsigned, because the line below is a small multiply and add generator whose
// whole point is to wrap, and wrapping a signed int is undefined. Only the low
// six bits are ever read out of it, and those are the bits the signed version
// already produced, so the sequence of delays is the same one.
uint32_t nDGIncFrameRnd = 1234567;

static CPool<CObj<CObjectBase>, 64> holdQueue[N_HOLD_TIME];
void SetToHoldQueue( CObjectBase *p, int *pnFrame )
{
	//if ( *pnFrame != -1 )
	//	holdQueue[ (*pnFrame) & (N_HOLD_TIME-1) ].remove( p );
	*pnFrame = nDGCurrentFrame;
	*holdQueue[( *pnFrame ) & ( N_HOLD_TIME - 1 )].Alloc() = p;
	nDGIncFrameRnd = nDGIncFrameRnd * 13u + 2457823u;
	*pnFrame += static_cast<int>( nDGIncFrameRnd & 63u ) + 64;
}

void ClearHoldQueue()
{
	for ( int i = 0; i < N_HOLD_TIME; ++i )
	{
		holdQueue[i].Clear();
	}
}

void MarkNewDGFrame()
{
	nDGCurrentFrame++;
	holdQueue[nDGCurrentFrame & ( N_HOLD_TIME - 1 )].Clear();
}

REGISTER_SAVELOAD_CLASS( SYSTEM, 0x1009DCC3, CVersioningBase )
