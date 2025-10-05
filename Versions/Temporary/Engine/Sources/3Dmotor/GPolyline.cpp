#include "stdafx.h"
#include "GPolyline.h"
#include "GfxBuffers.h"
#include "GfxRender.h"

namespace NGScene
{

// CMemGeometry

CMemGeometry::CMemGeometry( const std::vector<CVec3> &_points ) : points(_points)
{
}

void CMemGeometry::Recalc()
{
	ASSERT( !IsValid( pValue ) );
	if ( NGfx::IsTnLDevice() )
	{
		NGfx::CBufferLock<NGfx::SGeomVecT2C1> geom( &pValue, points.size() );
		for ( int i = 0; i < points.size(); ++i )
		{
			NGfx::SGeomVecT2C1 v;
			v.pos = points[i];
			geom[i] = v;
		}
	}
	else
	{
		NGfx::CBufferLock<NGfx::SGeomVecFull> geom( &pValue, points.size() );
		for ( int i = 0; i < points.size(); ++i )
		{
			NGfx::SGeomVecFull v;
			v.pos = points[i];
			geom[i] = v;
		}
	}
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x02911170, CMemGeometry )

