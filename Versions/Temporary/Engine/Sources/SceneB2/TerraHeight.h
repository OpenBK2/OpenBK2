#pragma once

#include "RidgeHeight.h"
#include "TerrainInfo.h"

inline void FindNormalsInTile( const STerrainInfo::STile &tile, const CVec3 &vert, CVec3 &vResNorm, int &nNormsCount )
{
	CVec3 vNorm;

	for ( std::vector<STriangle>::const_iterator it = tile.triangles.begin(); it != tile.triangles.end(); ++it )
	{
		const CVec3 v1( tile.vertices[it->i1].x,
		                tile.vertices[it->i1].y,
		                (std::max)( tile.vertices[it->i1].z + tile.addHeights[it->i1], 0.0f ) );
		const CVec3 v2( tile.vertices[it->i2].x,
		                tile.vertices[it->i2].y,
		                (std::max)( tile.vertices[it->i2].z + tile.addHeights[it->i2], 0.0f ) );
		const CVec3 v3( tile.vertices[it->i3].x,
		                tile.vertices[it->i3].y,
		                (std::max)( tile.vertices[it->i3].z + tile.addHeights[it->i3], 0.0f ) );

		if ( ( fabs2( v1 - vert ) < DEF_EPS ) || ( fabs2( v2 - vert ) < DEF_EPS ) || ( fabs2( v3 - vert ) < DEF_EPS ) )
		{
			GetTrueNormal( &vNorm, v1, v2, v3 );
			if ( vNorm.z < 0.0f )
			{
				vNorm = -vNorm;
			}
			vResNorm += vNorm;
			++nNormsCount;
		}
	}
}
