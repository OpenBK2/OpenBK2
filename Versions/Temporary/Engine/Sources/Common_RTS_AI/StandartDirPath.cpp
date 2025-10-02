#include "StdAfx.h"

#include "StandartDirPath.h"

#include "../Misc/Bresenham.h"

REGISTER_SAVELOAD_CLASS( 0x31114C82, CStandartDirPath );

CStandartDirPath::CStandartDirPath( const CVec2 &_startPoint, const CVec2 &_dir, const CVec2 &_finishPoint, const int _nTileSize )
: dir( _dir ), startPoint( _startPoint ), finishPoint( _finishPoint ), curPoint( _startPoint ), nTileSize( _nTileSize )
{
}

CStandartDirPath::CStandartDirPath( const CVec2 &_startPoint, const CVec2 &_finishPoint, const int _nTileSize )
: startPoint( _startPoint ), finishPoint( _finishPoint ), curPoint( _startPoint ), nTileSize( _nTileSize )
{
	dir = Norm( finishPoint - startPoint );
}

bool CStandartDirPath::IsFinished() const
{
	return mDistance( curPoint, finishPoint ) < 2;
}

void CStandartDirPath::RecoverPath( const CVec2 &point, const bool bIsPointAtWater, const SVector &lastKnownGoodTile )
{
	curPoint = point;
	dir = Norm( finishPoint - curPoint );
}

void CStandartDirPath::RecalcPath( const CVec2 &point, const bool bIsPointAtWater, const SVector &lastKnownGoodTile )
{
	startPoint = point;
	dir = finishPoint - point;
	Normalize( &dir );
}

const CVec2 CStandartDirPath::PeekPoint( const int nShift ) const
{
	if ( !IsFinished() )
	{
		CVec2 res( curPoint );
		int inc = 0;

		while ( ( finishPoint - res ) * dir > 0  && inc < nShift )
		{
			res += dir * nTileSize;
			++inc;
		}

		if ( ( finishPoint - res ) * dir <= 0 )
			return finishPoint;
		else
			return res;
	}
	else 
		return finishPoint;
}

void CStandartDirPath::Shift( const int nShift )
{
	if ( !IsFinished() )
	{
		int inc = 0;

		while ( ( finishPoint - curPoint ) * dir > 0 && inc < nShift )
		{
			curPoint += dir * nTileSize;
			++inc;
		}

		if ( ( finishPoint - curPoint ) * dir <= 0 )
			curPoint = finishPoint;
	}
}

void CStandartDirPath::MarkPath( const int nID, const NDebugInfo::EColor color ) const
{
	vector<SVector> tiles;
	SVector vStartTile( startPoint.x / nTileSize, startPoint.y / nTileSize );
	SVector vFinishTile( finishPoint.x / nTileSize, finishPoint.y / nTileSize );
	CBres bres;
	bres.InitPoint( vStartTile, vFinishTile );
	tiles.push_back( bres.GetDirection() );
	while ( bres.GetDirection() != vFinishTile )
	{
		bres.MakePointStep();
		tiles.push_back( bres.GetDirection() );
	}
	DebugInfoManager()->CreateMarker( nID, tiles, color );
}

