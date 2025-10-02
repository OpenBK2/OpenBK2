#include "stdafx.h"

#include "HitsStore.h"
#include "Misc/2Darray.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NTimer::STime curTime;
CHitsStore theHitsStore;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CHitsStore::Init( const int nMapSizeX, const int nMapSizeY )
{
	hits[0].SetSizes( nMapSizeX / SConsts::HIT_CELL_COEFF, nMapSizeY / SConsts::HIT_CELL_COEFF + 1 );
	hits[1].SetSizes( nMapSizeX / SConsts::HIT_CELL_COEFF, nMapSizeY / SConsts::HIT_CELL_COEFF + 1 );

	hits[0].FillZero();
	hits[1].FillZero();

	curIndex = 0;
	timeOfIndexBegin = curTime;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CHitsStore::Segment()
{
	if ( curTime - timeOfIndexBegin >= SConsts::TIME_OF_HIT_NOTIFY )
	{
		timeOfIndexBegin = curTime;
		curIndex = 1 - curIndex;
		hits[curIndex].FillZero();
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CHitsStore::AddHit( const CVec2 &center, const EHitTypes eHitType )
{
	const int nX = Clamp( int( center.x ) / SConsts::HIT_CELL_SIZE, 0, hits[curIndex].GetSizeX() - 1 );
	const int nY = Clamp( int( center.y ) / SConsts::HIT_CELL_SIZE, 0, hits[curIndex].GetSizeY() - 1 );

	hits[curIndex][nY][nX] = eHitType;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CHitsStore::WasHit( const CVec2 &center, const float fR, const EHitTypes eHitType ) const
{
	const int nDownX = Max( 0, int( center.x - fR ) / SConsts::HIT_CELL_SIZE );
	const int nDownY = Max( 0, int( center.y - fR ) / SConsts::HIT_CELL_SIZE );
	const int nUpX = Min( hits[0].GetSizeX() - 1, int( center.x + fR ) / SConsts::HIT_CELL_SIZE );
	const int nUpY = Min( hits[0].GetSizeY() - 1, int( center.y + fR ) / SConsts::HIT_CELL_SIZE );

	for ( int y = nDownY; y <= nUpY; ++y )
	{
		for ( int x = nDownX; x <= nUpX; ++x )
		{
			if ( ( hits[0][y][x] & eHitType ) || ( hits[1][y][x] & eHitType ) )
				return true;
		}
	}

	return false;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CHitsStore::Clear()
{
	hits[0].FillZero();
	hits[1].FillZero();
	
	curIndex = 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

