#pragma once

template <class THeightCalc>
inline float GetHeight( const CVec2 &p, const CVec3fEx &p0, const CVec3fEx &p1, const CVec3fEx &p2, const THeightCalc &heightCalc )
{
	CVec2 vBary;
	GetBaryCoords( p, p0, p1, p2, &vBary );
	if ( ( vBary.x > -DEF_EPS ) && ( vBary.y > -DEF_EPS ) && ( ( vBary.x + vBary.y ) < ( 1.0f + DEF_EPS ) ) )
		return ( ( p1.z - p0.z ) * vBary.x + ( p2.z - p0.z ) * vBary.y + p0.z ) * heightCalc( vBary.x );
	else
		//return 0.0f;
		return -1.0f;
}


template <class THeightCalc>
inline float GetHeightInv( const CVec2 &p, const CVec3fEx &p0, const CVec3fEx &p1, const CVec3fEx &p2, const THeightCalc &heightCalc )
{
	CVec2 vBary;
	GetBaryCoords( p, p0, p1, p2, &vBary );
	if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) )
		return ( (p1.z - p0.z) * vBary.x + (p2.z - p0.z) * vBary.y + p0.z ) * heightCalc( 1.0f - vBary.x );
	else
		//return 0.0f;
		return -1.0f;
}


template <class THeightCalc>
inline bool GetIncRidgeHeight( const CVec2 &v, const std::vector<CVec3fEx> &ridge, float *pIncHeight, const THeightCalc &heightCalc )
{
	*pIncHeight = 0.0f;
	int nCount = 0;
	const int nHalfSize = ( ridge.size() >> 1 ) - 1;
	for ( int k = 0; k < nHalfSize; ++k )
	{
		const float fH1 = GetHeight( v, ridge[k+1], ridge[ridge.size()-k-2], ridge[k], heightCalc );
		if ( fH1 >= 0.0f )
		{
			*pIncHeight += fH1;
			++nCount;
		}
		const float fH2 = GetHeightInv( v, ridge[ridge.size()-k-1], ridge[k], ridge[ridge.size()-k-2], heightCalc );
		if ( fH2 >= 0.0f )
		{
			*pIncHeight += fH2;
			++nCount;
		}
	}
	if ( nCount > 0 )
	{
		*pIncHeight /= nCount;
		return true;
	}
	return false;
}


template <class THeightCalc>
inline bool GetIncRidgeHeightMaxLaw( const CVec2 &v, const std::vector<CVec3fEx> &ridge, float *pIncHeight, const THeightCalc &heightCalc )
{
	*pIncHeight = 0.0f;
	bool bFlag = false;
	const int nHalfSize = ( ridge.size() >> 1 ) - 1;
	for ( int k = 0; k < nHalfSize; ++k )
	{
		const float fH1 = GetHeight( v, ridge[k+1], ridge[ridge.size()-k-2], ridge[k], heightCalc );
		if ( fH1 >= 0.0f )
		{
			*pIncHeight = (std::max)( fH1, *pIncHeight );
			bFlag = true;
		}
		const float fH2 = GetHeightInv( v, ridge[ridge.size()-k-1], ridge[k], ridge[ridge.size()-k-2], heightCalc );
		if ( fH2 >= 0.0f )
		{
			*pIncHeight = (std::max)( fH2, *pIncHeight );
			bFlag = true;
		}
	}
	return bFlag;
}


template <class THeightCalc>
inline bool GetIncRidgeHeightMinLaw( const CVec2 &v, const std::vector<CVec3fEx> &ridge, float *pIncHeight, const THeightCalc &heightCalc )
{
	*pIncHeight = FP_MAX_VALUE;
	bool bFlag = false;
	const int nHalfSize = ( ridge.size() >> 1 ) - 1;
	for ( int k = 0; k < nHalfSize; ++k )
	{
		const float fH1 = GetHeight( v, ridge[k+1], ridge[ridge.size()-k-2], ridge[k], heightCalc );
		if ( fH1 >= 0.0f )
		{
			*pIncHeight = (std::min)( fH1, *pIncHeight );
			bFlag = true;
		}
		const float fH2 = GetHeightInv( v, ridge[ridge.size()-k-1], ridge[k], ridge[ridge.size()-k-2], heightCalc );
		if ( fH2 >= 0.0f )
		{
			*pIncHeight = (std::min)( fH2, *pIncHeight );
			bFlag = true;
		}
	}
	if ( !bFlag )
		*pIncHeight = 0.0f;
	return bFlag;
}


inline void FindNormalsInTile( const STerrainInfo::STile &tile, const CVec3 &vert, CVec3 &vResNorm, int &nNormsCount )
{
	CVec3 vNorm;

	for ( std::vector<STriangle>::const_iterator it = tile.triangles.begin(); it != tile.triangles.end(); ++it )
	{
		const CVec3 v1( tile.vertices[it->i1].x, tile.vertices[it->i1].y, (std::max)(tile.vertices[it->i1].z + tile.addHeights[it->i1], 0.0f) );
		const CVec3 v2( tile.vertices[it->i2].x, tile.vertices[it->i2].y, (std::max)(tile.vertices[it->i2].z + tile.addHeights[it->i2], 0.0f) );
		const CVec3 v3( tile.vertices[it->i3].x, tile.vertices[it->i3].y, (std::max)(tile.vertices[it->i3].z + tile.addHeights[it->i3], 0.0f) );

		if ( (fabs2(v1 - vert) < DEF_EPS) || (fabs2(v2 - vert) < DEF_EPS) || (fabs2(v3 - vert) < DEF_EPS) )
		{
			GetTrueNormal( &vNorm, v1, v2, v3 );
			if ( vNorm.z < 0.0f )
				vNorm = -vNorm;
			vResNorm += vNorm;
			++nNormsCount;
		}
	}
}



