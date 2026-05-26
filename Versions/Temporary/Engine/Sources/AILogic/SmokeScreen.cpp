#include "stdafx.h"

#include "Misc/bresenham.h"
#include "SmokeScreen.h"
#include "StaticObjectsIters.h"
#include "Common_RTS_AI/AIMap.h"
#include "GlobalWarFog.h"

extern NTimer::STime curTime;
extern CStaticObjects theStatObjs;
extern CGlobalWarFog theWarFog; 

REGISTER_SAVELOAD_CLASS( 0x1508D4B5, CSmokeScreen );

CSmokeScreen::CSmokeScreen( const CVec3 &_vCenter, const float _fRadius, const int _nTransparency, const int nTime )
: vCenter( _vCenter ), fRadius( _fRadius ), nTransparency( _nTransparency ), timeOfDissapear( curTime + nTime ),
	CExistingObject( 0, 1.0f ), tileCenter( AICellsTiles::GetTile( CVec2(_vCenter.x,_vCenter.y) ) ), bTransparencySet( false )
{
}

void CSmokeScreen::Init()
{
	nextSegmTime = curTime + 4 * SConsts::AI_SEGMENT_DURATION + NRandom::Random( 0, 3 * SConsts::AI_SEGMENT_DURATION ); RecordRandomCall();
	theStatObjs.RegisterSegment( this );
}

void CSmokeScreen::Segment()
{
	nextSegmTime = curTime + 4 * SConsts::AI_SEGMENT_DURATION + NRandom::Random( 0, 3 * SConsts::AI_SEGMENT_DURATION ); RecordRandomCall();
	if ( curTime >= timeOfDissapear )
	{
		RemoveTransparencies();
		theStatObjs.DeleteInternalObjectInfo( this );
	}
}

void CSmokeScreen::TraceToPoint( const int x, const int y, const bool bAdd )
{
	const SVector center = tileCenter;
	const SVector finishPoint( tileCenter.x + x, tileCenter.y + y );
	
	CBres bres;
	bres.InitPoint( center, finishPoint );

	do
	{
		bres.MakePointStep();
		if ( !GetAIMap()->IsTileInside( bres.GetDirection() ) )
			break;
		//theWarFog.SetTransparency( bres.GetDirection(), !bAdd );
	} while ( bres.GetDirection() != finishPoint );
}

void CSmokeScreen::OctupleTrace( const int x, const int y, const bool bAdd )
{
	TraceToPoint(  x,  y, bAdd );
	TraceToPoint( -x,  y, bAdd );
	TraceToPoint(  x, -y, bAdd );
	TraceToPoint( -x, -y, bAdd );
	
	TraceToPoint(  y,  x, bAdd );
	TraceToPoint(  y, -x, bAdd );
	TraceToPoint( -y,  x, bAdd );
	TraceToPoint( -y, -x, bAdd ); 
}

void CSmokeScreen::Trace( const bool bAdd )
{
	/*
	if ( GetAIMap()->IsTileInside( tileCenter ) )
	{
		theWarFog.SetTransparency( tileCenter, !bAdd );
	}
	*/

	const int r = fRadius / SConsts::TILE_SIZE + 1;
	int x = 0, y = r;
	int d = 3 - 2*y;

	TraceToPoint(  0,  r, bAdd );
	TraceToPoint(  0, -r, bAdd );
	TraceToPoint(  r,  0, bAdd );
	TraceToPoint( -r,  0, bAdd );

	do 
	{
		if ( d < 0 )
			d += 4*x + 6;
		else
		{
			d += 4*(x - y) + 10;
			--y;
			OctupleTrace( x, y, bAdd );
		}
		++x;
		
		OctupleTrace( x, y, bAdd );
	}	while ( x <= y );
}

void CSmokeScreen::SetTransparencies()
{
	Trace( true );
	bTransparencySet = true;
}

void CSmokeScreen::RestoreTransparenciesImmidiately()
{
	if ( bTransparencySet )
		SetTransparencies();
}

void CSmokeScreen::RemoveTransparencies()
{
	Trace( false );
	bTransparencySet = false;

	/*for ( CStObjCircleIter<false> iter( CVec2(vCenter.x,vCenter.y), fRadius + SConsts::TILE_SIZE * 30 ); !iter.IsFinished(); iter.Iterate() )
	{
		(*iter)->RestoreTransparencies();
	}*/
}

void CSmokeScreen::GetCoveredTiles( std::list<SVector> *pTiles ) const
{
	pTiles->clear();
	const int nTilesR = fRadius / SConsts::TILE_SIZE + 1;
	for ( int y = tileCenter.y - nTilesR; y <= tileCenter.y + nTilesR; ++y )
	{
		for ( int x = tileCenter.x - nTilesR; x <= tileCenter.x + nTilesR; ++x )
		{
			const SVector tile( x, y );
			if ( GetAIMap()->IsTileInside( tile ) )
				pTiles->push_back( tile );
		}
	}
}


