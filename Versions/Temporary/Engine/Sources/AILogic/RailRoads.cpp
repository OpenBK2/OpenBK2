#include "stdafx.h"
#include "RailRoads.h"

SRailRoadSystem theRailRoadSystem;

void SRailRoadSystem::AddRailRoad( const NDb::SVSOInstance *pVSO )
{
	SRRInstance newRR;

	newRR.pDescriptor = pVSO->pDescriptor;
	newRR.points = pVSO->points;
	newRR.fPointLength = fabs( pVSO->points[1].vPos - pVSO->points[0].vPos );

	segments.push_back( newRR );
}

void SRailRoadSystem::SRRInstance::Decompose( const float fPos, int *pPoint, float *pFraction ) const
{
	*pFraction = fPos / fPointLength;
	*pPoint = int( *pFraction );
	*pFraction -= *pPoint;
}

const CVec2 SRailRoadSystem::SRRInstance::GetPoint( const float fPos ) const
{
	float fFraction;
	int nPoint;
	CVec3 vPos;
	
	Decompose( (std::max)( fPos, 0.0f ), &nPoint, &fFraction );
	if ( nPoint + 1 >= points.size() )
		vPos = points.back().vPos;
	else
		vPos.Lerp( fFraction, points[nPoint].vPos, points[nPoint + 1].vPos );

	return CVec2( vPos.x, vPos.y );
}

