#pragma once

#include "Misc/PlaneGeometry.h"

#include <cstdint>

EXTERNVAR const float MINIMAL_POINT_DISTANCE;	//2.0f


template<>
inline void UpdateBoundingBox( CVec3 *pvMin, CVec3 *pvMax, const CVec3 &rvPosition )
{
	if ( pvMin->x > rvPosition.x )
	{
		pvMin->x = rvPosition.x;
	}
	else if ( pvMax->x < rvPosition.x )
	{
		pvMax->x = rvPosition.x;
	}
	//
	if ( pvMin->y > rvPosition.y )
	{
		pvMin->y = rvPosition.y;
	}
	else if ( pvMax->y < rvPosition.y )
	{
		pvMax->y = rvPosition.y;
	}
	//
	if ( pvMin->z > rvPosition.z )
	{
		pvMin->z = rvPosition.z;
	}
	else if ( pvMax->z < rvPosition.z )
	{
		pvMax->z = rvPosition.z;
	}
}

//AI Direction

//uint16_t GetAIDirectionByVector( float x, float y, uint16_t wDefaultDirection );

//void GetVectorByAIDirection( CVec2 *pVec2, uint16_t wDirection, float fRadius );

//3D Geometry
float GetDistanceTo3DLine( const CVec3 &rvPoint, const CVec3 &rvOrigin, const CVec3 &rvDirection );


