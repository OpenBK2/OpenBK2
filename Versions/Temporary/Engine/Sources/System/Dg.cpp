#include "StdAfx.h"
#include "DG.h"
#include "..\Misc\Pool.h"

#ifdef _MAPEDIT 
	const int N_HOLD_TIME = 16384; // must be power of 2
#else
	const int N_HOLD_TIME = 256; // must be power of 2
#endif
int nDGCurrentFrame = 100, nDGIncFrameRnd = 1234567;

static CPool<CObj<CObjectBase>, 64 > holdQueue[N_HOLD_TIME];
void SetToHoldQueue( CObjectBase *p, int *pnFrame )
{
	//if ( *pnFrame != -1 )
	//	holdQueue[ (*pnFrame) & (N_HOLD_TIME-1) ].remove( p );
	*pnFrame = nDGCurrentFrame;
	*holdQueue[ (*pnFrame) & (N_HOLD_TIME-1) ].Alloc() = p;
	nDGIncFrameRnd = nDGIncFrameRnd * 13 + 2457823;
	*pnFrame += ( nDGIncFrameRnd & 63 ) + 64;
}

void ClearHoldQueue()
{
	for ( int i = 0; i < N_HOLD_TIME; ++i )
		holdQueue[i].Clear();
}


void MarkNewDGFrame() 
{
	nDGCurrentFrame++;
	holdQueue[ nDGCurrentFrame&(N_HOLD_TIME-1) ].Clear();
}

REGISTER_SAVELOAD_CLASS( 0x1009DCC3, CVersioningBase )

