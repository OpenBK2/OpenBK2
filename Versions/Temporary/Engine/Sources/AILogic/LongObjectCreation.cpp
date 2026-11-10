#include "stdafx.h"

#include "AIUnit.h"
#include "BoundedSampling.h"
#include "LongObjectCreation.h"
#include "System/FastMath.h"
#include "UnitsIterators2.h"
#include "UnitStates.h"

#include "AILogic_export.h"

#include <cstdint>

BASIC_REGISTER_CLASS( AILOGIC, CLongObjectCreation );

uint16_t CLongObjectCreation::GetLineAngle( const CVec2 &vBegin, const CVec2 &vEnd )
{
	CVec2 vTmp = vEnd - vBegin;
	const float fLength = fabs( vTmp );
	if ( !( fLength > 0.0f ) || !std::isfinite( fLength ) )
	{
		return 0;
	}
	Normalize( &vTmp );
	float fAngle = NMath::ACos( ( vTmp.x ) / fabs( vTmp.x, vTmp.y ) );
	if ( vTmp.y < 0 )
	{
		fAngle = FP_2PI - fAngle;
	}
	return uint16_t( ( fAngle / ( 2.0f * PI ) ) * 65535 );
}

void CLongObjectCreation::SplitLineToSegrments( std::vector<CVec2> *_vPoints,
                                                const CVec2 &vBegin,
                                                const CVec2 &vEnd,
                                                float TRENCHWIDTH )
{
	CVec2 currentPoint = vBegin;
	std::vector<CVec2> &vPoints = *_vPoints;

	const float allLenght = fabs( vEnd.x - vBegin.x, vEnd.y - vBegin.y );
	if ( vBegin == vEnd || !NBoundedSampling::IsValidSpacing( allLenght, TRENCHWIDTH ) || !std::isfinite( vBegin.x ) ||
	     !std::isfinite( vBegin.y ) || !std::isfinite( vEnd.x ) || !std::isfinite( vEnd.y ) )
	{
		return;
	}
	CVec2 vDir( vEnd - vBegin );
	Normalize( &vDir );

	const size_t nOriginalSize = vPoints.size();
	CVec2 vAddSegment;
	vAddSegment.x = vDir.x * TRENCHWIDTH;
	vAddSegment.y = vDir.y * TRENCHWIDTH;
	vAddSegment.x += currentPoint.x;
	vAddSegment.y += currentPoint.y;
	if ( fabs( vAddSegment.x - currentPoint.x, vAddSegment.y - currentPoint.y ) < allLenght )
	{
		vPoints.push_back( CVec2( currentPoint.x, currentPoint.y ) );
	}

	float fPreviousDistance = 0.0f;
	float fDistance = fabs( vBegin.x - vAddSegment.x, vBegin.y - vAddSegment.y );
	int nSamples = 0;
	while ( fDistance < allLenght )
	{
		if ( !( fDistance > fPreviousDistance ) || ++nSamples > NBoundedSampling::MAX_SAMPLES )
		{
			// A sub-ULP width can leave the point unchanged even when the width itself is positive.
			vPoints.resize( nOriginalSize );
			return;
		}
		vPoints.push_back( CVec2( vAddSegment.x, vAddSegment.y ) );
		currentPoint = CVec2( vAddSegment.x, vAddSegment.y );
		vAddSegment.x = vDir.x * TRENCHWIDTH;
		vAddSegment.y = vDir.y * TRENCHWIDTH;
		vAddSegment.x += currentPoint.x;
		vAddSegment.y += currentPoint.y;
		fPreviousDistance = fDistance;
		fDistance = fabs( vBegin.x - vAddSegment.x, vBegin.y - vAddSegment.y );
	}
}

void CLongObjectCreation::GetUnitsPreventing( const SRect &r1, std::list<CPtr<CAIUnit>> *units ) const
{
	const float fRadius = r1.lengthAhead + r1.lengthBack + r1.width + SConsts::TILE_SIZE * 5;
	for ( CUnitsIter<0, 2> iter( 0, ANY_PARTY, r1.center, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		if ( !( *iter )->GetStats()->IsInfantry() && !( *iter )->GetStats()->IsAviation() )
		{
			const SRect rect = ( *iter )->GetUnitRect();
			if ( r1.IsIntersected( rect ) )
			{
				units->push_back( *iter );
			}
		}
	}
}

bool CLongObjectCreation::IsAnyUnitPrevent( const SRect &r1 ) const
{
	const float fRadius = r1.lengthAhead + r1.lengthBack + r1.width + SConsts::TILE_SIZE * 5;
	for ( CUnitsIter<0, 2> iter( 0, ANY_PARTY, r1.center, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		if ( !( *iter )->GetStats()->IsInfantry() && !( *iter )->GetStats()->IsAviation() )
		{
			const SRect rect = ( *iter )->GetUnitRect();
			if ( r1.IsIntersected( rect ) )
			{
				return true;
			}
		}
	}
	return false;
}

bool CLongObjectCreation::CanBuildOnRect( SRect r1, const std::list<SVector> &tilesUnder ) const
{
	// hack! to avoid problem with bounds (due to open set of coordinates)
	r1.Compress( 0.99f );
	if ( !GetAIMap()->IsRectInside( r1 ) )
	{
		return false;
	}

	std::list<CPtr<CAIUnit>> preventing;

	GetUnitsPreventing( r1, &preventing );
	UnlockPreventingUnits( preventing );

	// теперь проверить, можно ли строить
	for ( std::list<SVector>::const_iterator it = tilesUnder.begin(); it != tilesUnder.end(); ++it )
	{
		if ( 0 != GetTerrain()->GetTileLockInfo( *it ) )
		{
			return false;
		}
	}
	return true;
}

void CLongObjectCreation::UnlockPreventingUnits( std::list<CPtr<CAIUnit>> &preventing ) const
{
	// Placement previews are local to one client and must not alter pathfinding locks.
	if ( !IsAIModificationAllowed() )
	{
		return;
	}

	for ( std::list<CPtr<CAIUnit>>::iterator it = preventing.begin(); it != preventing.end(); ++it )
	{
		CAIUnit *pUnit = *it;
		if ( pUnit->GetPlayer() == nPlayer && EUSN_REST == pUnit->GetState()->GetName() && pUnit->CanMove() )
		{
			pUnit->UnlockTiles();
		}
	}
}
