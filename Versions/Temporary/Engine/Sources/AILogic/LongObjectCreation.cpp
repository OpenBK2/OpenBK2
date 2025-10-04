#include "stdafx.h"

#include "longobjectcreation.h"
#include "UnitsIterators2.h"
#include "AIUnit.h"
#include "UnitStates.h"
#include "System/FastMath.h"

BASIC_REGISTER_CLASS(CLongObjectCreation);

WORD CLongObjectCreation::GetLineAngle( const CVec2 &vBegin, const CVec2 &vEnd )
{
	CVec2 vTmp = vEnd - vBegin;  
	Normalize( &vTmp );
	float fAngle = NMath::ACos( ( vTmp.x )/ fabs( vTmp.x, vTmp.y ) );
	if ( vTmp.y < 0 )
		fAngle = FP_2PI - fAngle;
	return WORD ( (fAngle/( 2.0f * PI ) ) * 65535 );
}

void CLongObjectCreation::SplitLineToSegrments( vector<CVec2> *_vPoints, const CVec2 &vBegin, const CVec2 &vEnd, float TRENCHWIDTH )
{
	CVec2 currentPoint = vBegin;
	vector<CVec2> &vPoints = *_vPoints; 

	if ( vBegin == vEnd )
		return;
	CVec2 vDir( vEnd- vBegin );
	Normalize( &vDir );

	float allLenght = fabs( vEnd.x - vBegin.x, vEnd.y - vBegin.y );
	CVec2 vAddSegment;
	vAddSegment.x = vDir.x * TRENCHWIDTH;
	vAddSegment.y = vDir.y * TRENCHWIDTH;
	vAddSegment.x += currentPoint.x;
	vAddSegment.y += currentPoint.y;
	if ( fabs( vAddSegment.x - currentPoint.x, vAddSegment.y - currentPoint.y ) < allLenght )
	{
		vPoints.push_back( CVec2( currentPoint.x, currentPoint.y ) );
	}

	while ( fabs( vBegin.x - vAddSegment.x, vBegin.y - vAddSegment.y ) < allLenght )
	{
		vPoints.push_back( CVec2( vAddSegment.x, vAddSegment.y ) );
		currentPoint = CVec2( vAddSegment.x, vAddSegment.y );
		vAddSegment.x = vDir.x * TRENCHWIDTH ;
		vAddSegment.y = vDir.y * TRENCHWIDTH ;
		vAddSegment.x += currentPoint.x;
		vAddSegment.y += currentPoint.y;
	}	
}

void CLongObjectCreation::GetUnitsPreventing( const SRect &r1, list< CPtr<CAIUnit> > *units ) const
{
	const float fRadius = r1.lengthAhead + r1.lengthBack + r1.width + SConsts::TILE_SIZE * 5;
	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, r1.center, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		if ( !(*iter)->GetStats()->IsInfantry() && !(*iter)->GetStats()->IsAviation() )
		{
			const SRect rect = (*iter)->GetUnitRect();
			if ( r1.IsIntersected( rect ) ) 
				units->push_back( *iter );
		}
	}
}

bool CLongObjectCreation::IsAnyUnitPrevent( const SRect &r1 ) const
{
	const float fRadius = r1.lengthAhead + r1.lengthBack + r1.width + SConsts::TILE_SIZE * 5;
	for ( CUnitsIter<0,2> iter( 0, ANY_PARTY, r1.center, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		if ( !(*iter)->GetStats()->IsInfantry() && !(*iter)->GetStats()->IsAviation()  )
		{
			const SRect rect = (*iter)->GetUnitRect();
			if ( r1.IsIntersected( rect ) )
				return true;
		}
	}
	return false;
}

bool CLongObjectCreation::CanBuildOnRect( SRect r1, const list<SVector> &tilesUnder ) const
{
	// hack! to avoid problem with bounds (due to open set of coordinates)
	r1.Compress( 0.99f );	
	if ( !GetAIMap()->IsRectInside( r1 ) )
		return false;

	list<CPtr<CAIUnit> > preventing;

	GetUnitsPreventing( r1, &preventing );
	UnlockPreventingUnits( preventing );

	// теперь проверить, можно ли строить
	for ( list<SVector>::const_iterator it = tilesUnder.begin(); it != tilesUnder.end(); ++it )
	{
		if ( 0 != GetTerrain()->GetTileLockInfo( *it ) )
			return false;
	}
	return true;
}

void CLongObjectCreation::UnlockPreventingUnits( list<CPtr<CAIUnit> > &preventing ) const
{
	for ( list<CPtr<CAIUnit> >::iterator it = preventing.begin(); it != preventing.end(); ++it )
	{
		CAIUnit * pUnit = *it;
		if ( pUnit->GetPlayer() == nPlayer && EUSN_REST == pUnit->GetState()->GetName() && pUnit->CanMove() )
			pUnit->UnlockTiles();
	}
}

