#include "StdAfx.h"

#include <limits.h>
#include "TerraTools.h"
#include "GenTerrain.h"
#include "TerraHeight.h"
#include "Scene.h"

#define DEF_HEIGHTS_SMOOTH_RADIUS 3
#define DEF_SMOOTH_DIST ( DEF_TILE_SIZE * 7.0f )
#define DEF_SMOOTH_SLEEKNESS ( DEF_TILE_SIZE * FP_SQRT_2 / DEF_SMOOTH_DIST )
#define DEF_SMOOTH_SLEEKNESS_COEFF ( 1.0f / ( 1.0f - DEF_SMOOTH_SLEEKNESS ) )
#define DEF_SEA_OUTSIDE_MAP	0.1f

inline void AI2VisTiles( CVec2i *pRes, const CVec3 &vPos )
{
	pRes->x = vPos.x * AI_TO_VIS * DEF_INV_TILE_SIZE;
	pRes->y = vPos.y * AI_TO_VIS * DEF_INV_TILE_SIZE;
}


inline void AI2VisTiles( CVec2i *pRes, const CVec2 &vPos )
{
	pRes->x = vPos.x * AI_TO_VIS * DEF_INV_TILE_SIZE;
	pRes->y = vPos.y * AI_TO_VIS * DEF_INV_TILE_SIZE;
}


struct SWaterSmoothProfile
{
	float operator()( const float x ) const
	{
		//return x < DEF_SMOOTH_SLEEKNESS ? 1.0f :
			//( 0.5f - NMath::Sin( ( x - DEF_SMOOTH_SLEEKNESS ) * DEF_SMOOTH_SLEEKNESS_COEFF * FP_PI - FP_PI2 ) * 0.5f );
		if ( x < DEF_SMOOTH_SLEEKNESS )
			return 0.0f;
		const float tx = ( x - DEF_SMOOTH_SLEEKNESS ) * DEF_SMOOTH_SLEEKNESS_COEFF;
		return tx * tx;
	}
};


inline int GetWaterFillIndex( vector<NWaterStuff::SWaterParams> *pWaterParams, const NDb::SWater &water )
{
	int nInd = -1;
	for ( int i = 0; i < pWaterParams->size(); ++i )
	{
		if ( (*pWaterParams)[i] == water )
		{
			nInd = i;
			break;
		}
	}

	if ( nInd == -1 )
	{
		vector<NWaterStuff::SWaterParams>::iterator it = pWaterParams->insert( pWaterParams->end(), NWaterStuff::SWaterParams() );
		*it = water;
		nInd = pWaterParams->size() - 1;
	}

	return nInd + 1;
}


//static void FillSeaArea( CArray2D<float> *pHeights, const vector<NDb::SVSOPoint> &samples, const CVec3 &_vMidPoint, CArray2D<BYTE> *pMask )
//{
//	vector<CVec2i> points( samples.size() );
//	for ( int i = 0; i < samples.size(); ++i )
//		AI2VisTiles( &(points[i]), samples[i].vPos );
//
//	CVec2i vMidPoint;
//	AI2VisTiles( &vMidPoint, _vMidPoint );
//
//	CArray2D<BYTE> fillMap( pMask->GetSizeX(), pMask->GetSizeY() );
//	fillMap.FillZero();
//
//	for ( int i = 1; i < points.size(); ++i )
//		DrawLine( &fillMap, points[i - 1].x, points[i - 1].y, points[i].x, points[i].y, 1 );
//
//	SimpleFill( &fillMap, vMidPoint.x, vMidPoint.y, 1, 1 );
//
//	for ( int g = 0; g < fillMap.GetSizeY(); ++g )
//	{
//		for ( int i = 0; i < fillMap.GetSizeX(); ++i )
//		{
//			if ( fillMap[g][i] )
//				(*pHeights)[g][i] = -1.0f;
//		}
//	}
//
//	for ( int i = 1; i < points.size(); ++i )
//		DrawLine( &fillMap, points[i - 1].x, points[i - 1].y, points[i].x, points[i].y, 0 );
//
//	for ( int g = 0; g < fillMap.GetSizeY(); ++g )
//	{
//		for ( int i = 0; i < fillMap.GetSizeX(); ++i )
//		{
//			const bool b1 = i > 0 ? fillMap[g][i - 1] : 1;
//			const bool b2 = g > 0 ? fillMap[g - 1][i] : 1;
//			const bool b3 = i < ( fillMap.GetSizeX() - 1 ) ? fillMap[g][i + 1] : 1;
//			const bool b4 = g < ( fillMap.GetSizeY() - 1 ) ? fillMap[g + 1][i] : 1;
//
//			if ( fillMap[g][i] && b1 && b2 && b3 && b4 )
//				(*pMask)[g][i] = 1;
//		}
//	}
//}


static void FillWaterArea( CArray2D<float> *pHeights, const float fFillHeight, CArray2D<BYTE> *pMask, const BYTE cFillMask,
													 const vector<NDb::SVSOPoint> &samples )
{
	if ( samples.size() < 2 )
		return;

	vector<CVec3dEx> boundPoints( samples.size() );
	CVec2i vMin( INT_MAX, INT_MAX ), vMax( INT_MIN, INT_MIN );
	for ( int i = 0; i < samples.size(); ++i )
	{
		const CVec3 &vPos = samples[i].vPos/* + samples[i].vNorm * samples[i].fWidth*/;
		boundPoints[i].Set( AI2Vis(vPos.x), AI2Vis(vPos.y), 0.0f, 1 );
		const int x = Float2Int( boundPoints[i].x * DEF_INV_TILE_SIZE );
		const int y = Float2Int( boundPoints[i].y * DEF_INV_TILE_SIZE );
		vMin.x = min( vMin.x, x );
		vMin.y = min( vMin.y, y );
		vMax.x = max( vMax.x, x );
		vMax.y = max( vMax.y, y );
	}

	// expand for i2fp optimization
	vMin.x = Clamp( vMin.x - 1, 0, pHeights->GetSizeX() - 1 );
	vMin.y = Clamp( vMin.y - 1, 0, pHeights->GetSizeY() - 1 );
	vMax.x = Clamp( vMax.x + 2, 0, pHeights->GetSizeX() - 1 );
	vMax.y = Clamp( vMax.y + 2, 0, pHeights->GetSizeY() - 1 );

	CArray2D<BYTE> fillMap( vMax.x - vMin.x + 2, vMax.y - vMin.y + 2 );
	fillMap.FillZero();

	for ( int g = vMin.y; g <= vMax.y; ++g )
	{
		for ( int i = vMin.x; i <= vMax.x; ++i )
		{
			if ( IsInside(boundPoints, CVec3dEx((float)i * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE, 0, 1)) )
				fillMap[g - vMin.y][i - vMin.x] = 1;
		}
	}

	for ( int g = 0; g < fillMap.GetSizeY(); ++g )
	{
		for ( int i = 0; i < fillMap.GetSizeX(); ++i )
		{
			if ( fillMap[g][i] )
			{
				const int ox = i + vMin.x;
				const int oy = g + vMin.y;
				(*pMask)[oy][ox] = cFillMask;
				(*pHeights)[oy][ox] = fFillHeight;
			}
		}
	}
}


static void SmoothHeightsFromSamples( CArray2D<float> *pHeights, const vector<NDb::SVSOPoint> &samples )
{
	if ( samples.size() < 2 )
		return;

	vector<CVec3fEx> ridge( samples.size() * 2 );
	CVec2 vPos;
	CVec2 vMin( FP_MAX_VALUE, FP_MAX_VALUE ), vMax( -FP_MAX_VALUE, -FP_MAX_VALUE );
	int nInd = 0;
	for ( vector<NDb::SVSOPoint>::const_iterator it = samples.begin(); it != samples.end(); ++it, ++nInd )
	{
		vPos.Set( it->vPos.x, it->vPos.y );
		AI2Vis( &vPos );
		const CVec2 vNextPos( vPos.x - it->vNorm.x * DEF_SMOOTH_DIST, vPos.y - it->vNorm.y * DEF_SMOOTH_DIST );
		ridge[nInd].Set( vPos.x, vPos.y, 1.0f, 0 );
		ridge[ridge.size() - 1 - nInd].Set( vNextPos.x, vNextPos.y, 1.0f, 0 );
		vMin.Minimize( vPos ); vMin.Minimize( vNextPos );
		vMax.Maximize( vPos ); vMax.Maximize( vNextPos );
	}

	vMin.x = Clamp( int(vMin.x * DEF_INV_TILE_SIZE), 0, pHeights->GetSizeX() - 1 );
	vMin.y = Clamp( int(vMin.y * DEF_INV_TILE_SIZE), 0, pHeights->GetSizeY() - 1 );
	vMax.x = Clamp( int(vMax.x * DEF_INV_TILE_SIZE) + 1, 0, pHeights->GetSizeX() - 1 );
	vMax.y = Clamp( int(vMax.y * DEF_INV_TILE_SIZE) + 1, 0, pHeights->GetSizeY() - 1 );

	float fIncHeight;

	SWaterSmoothProfile waterSmoothProfile;

	for ( int g = vMin.y; g <= vMax.y; ++g )
	{
		for ( int i = vMin.x; i <= vMax.x; ++i )
		{
			if ( GetIncRidgeHeight(CVec2((float)i * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE), ridge, &fIncHeight, waterSmoothProfile) )
			{
				float &fCurHeight = (*pHeights)[g][i];
				if ( fCurHeight < 0 )
					fCurHeight = fIncHeight;
				else
					fCurHeight = min( fCurHeight, fIncHeight );
			}
		}
	}
}


#define AREA_HASH_ELEMS_NUM 12
#define AREA_BOTTOM_LEFT 0
#define AREA_BOTTOM_RIGHT 1
#define AREA_TOP_RIGHT 2
#define AREA_TOP_LEFT 3
const int defAreaHash[AREA_HASH_ELEMS_NUM][5] =
{
	{ 17, AREA_BOTTOM_RIGHT, -1, -1, -1 },
	{ 10, AREA_BOTTOM_LEFT, AREA_TOP_LEFT, AREA_TOP_RIGHT, -1 },
	{ 26, AREA_TOP_RIGHT, -1, -1, -1 },
	{ 19, AREA_BOTTOM_RIGHT, AREA_BOTTOM_LEFT, AREA_TOP_LEFT, -1 },
	{ 35, AREA_TOP_LEFT, -1, -1, -1 },
	{ 28, AREA_TOP_RIGHT, AREA_BOTTOM_RIGHT, AREA_BOTTOM_LEFT, -1 },
	{ 12, AREA_BOTTOM_LEFT, -1, -1, -1 },
	{ 33, AREA_TOP_LEFT, AREA_TOP_RIGHT, AREA_BOTTOM_RIGHT, -1 },
	{ 25, AREA_TOP_RIGHT, AREA_BOTTOM_RIGHT, -1, -1 },
	{ 11, AREA_BOTTOM_LEFT, AREA_TOP_LEFT, -1, -1 },
	{ 20, AREA_BOTTOM_RIGHT, AREA_BOTTOM_LEFT, -1, -1 },
	{ 34, AREA_TOP_LEFT, AREA_TOP_RIGHT, -1, -1 }
};

#define AREA_BOTTOM_SIDE 1
#define AREA_RIGHT_SIDE 2
#define AREA_TOP_SIDE 3
#define AREA_LEFT_SIDE 4
#define AREA_NONE_SIDE 7

typedef hash_map<int, vector<int> > CAreaCornersHash;
static CAreaCornersHash areaCornersHash;

static void InitCornersHash()
{
	int k;
	for ( int i = 0; i < AREA_HASH_ELEMS_NUM; ++i )
	{
		const int nInd = defAreaHash[i][0];
		areaCornersHash[nInd].reserve( 3 );
		areaCornersHash[nInd].resize( 0 );
		k = 1;
		for ( ; defAreaHash[i][k] != -1; ++k )
			areaCornersHash[nInd].push_back( defAreaHash[i][k] );
	}
}


static int GetPointIndOnRect( const NDb::SVSOPoint &p, const CVec2 &p1, const CVec2 &p2 )
{
	if ( fabs(p1.y - p.vPos.y) < DEF_EPS )
		return AREA_BOTTOM_SIDE;
	if ( fabs( p2.x - p.vPos.x) < DEF_EPS )
		return AREA_RIGHT_SIDE;
	if ( fabs(p2.y - p.vPos.y) < DEF_EPS )
		return AREA_TOP_SIDE;
	if ( fabs(p1.x - p.vPos.x) < DEF_EPS )
		return AREA_LEFT_SIDE;
	return AREA_NONE_SIDE;
}


inline void GetCornerFromInd( NDb::SVSOPoint *pPoint, const int nInd, const CVec2 &p1, const CVec2 &p2 )
{
	switch ( nInd )
	{
	case AREA_BOTTOM_LEFT:
		pPoint->vPos.Set( p1.x, p1.y, 0 );
		pPoint->vNorm.Set( FP_SQRT_2 / 2, FP_SQRT_2 / 2, 0 );
		break;
	case AREA_BOTTOM_RIGHT:
		pPoint->vPos.Set( p2.x, p1.y, 0 );
		pPoint->vNorm.Set( -FP_SQRT_2 / 2, FP_SQRT_2 / 2, 0 );
		break;
	case AREA_TOP_RIGHT:
		pPoint->vPos.Set( p2.x, p2.y, 0 );
		pPoint->vNorm.Set( -FP_SQRT_2 / 2, -FP_SQRT_2 / 2, 0 );
		break;
	case AREA_TOP_LEFT:
		pPoint->vPos.Set( p1.x, p2.y, 0 );
		pPoint->vNorm.Set( FP_SQRT_2 / 2, -FP_SQRT_2 / 2, 0 );
		break;
	default:
		NI_ASSERT( false, "Error in corner hash" );
		pPoint->vPos.Set( p1.x, p1.y, 0 );
		pPoint->vNorm.Set( FP_SQRT_2 / 2, FP_SQRT_2 / 2, 0 );
	}
}


bool GetIntersectWithArea( NDb::SVSOPoint *pPoint, const NDb::SVSOPoint &vso1, const NDb::SVSOPoint &vso2, const CVec2 &p1, const CVec2 &p2 )
{
	const CVec3 &po1 = vso1.vPos;
	const CVec3 &po2 = vso2.vPos;
	float t = 0.0f, d;
	bool bFlag = false;

	// check bottom side
	d = po2.y - po1.y;
	if ( fabs(d) > EPS_VALUE )
	{
		t = ( p1.y - po1.y ) / d;
		if ( (t > -EPS_VALUE) && (t < (1.0f + EPS_VALUE)) )
		{
			pPoint->vPos.x = po1.x + ( po2.x - po1.x ) * t;
			if ( (pPoint->vPos.x > (p1.x - EPS_VALUE)) && (pPoint->vPos.x < (p2.x + EPS_VALUE)) )
			{
				pPoint->vPos.y = p1.y;
				bFlag = true;
			}
		}
	}

	if ( !bFlag )
	{
		// check top side
		d = po2.y - po1.y;
		if ( fabs(d) > EPS_VALUE )
		{
			t = ( p2.y - po1.y ) / d;
			if ( (t > -EPS_VALUE) && (t < (1.0f + EPS_VALUE)) )
			{
				pPoint->vPos.x = po1.x + ( po2.x - po1.x ) * t;
				if ( (pPoint->vPos.x > (p1.x - EPS_VALUE)) && (pPoint->vPos.x < (p2.x + EPS_VALUE)) )
				{
					pPoint->vPos.y = p2.y;
					bFlag = true;
				}
			}
		}
	}

	if ( !bFlag )
	{
		// check left side
		d = po2.x - po1.x;
		if ( fabs(d) > EPS_VALUE )
		{
			t = ( p1.x - po1.x ) / d;
			if ( (t > -EPS_VALUE) && (t < (1.0f + EPS_VALUE)) )
			{
				pPoint->vPos.y = po1.y + ( po2.y - po1.y ) * t;
				if ( (pPoint->vPos.y > (p1.y - EPS_VALUE)) && (pPoint->vPos.y < (p2.y + EPS_VALUE)) )
				{
					pPoint->vPos.x = p1.x;
					bFlag = true;
				}
			}
		}
	}

	if ( !bFlag )
	{
		// check right side
		d = po2.x - po1.x;
		if ( fabs(d) > EPS_VALUE )
		{
			t = ( p2.x - po1.x ) / d;
			if ( (t > -EPS_VALUE) && (t < (1.0f + EPS_VALUE)) )
			{
				pPoint->vPos.y = po1.y + ( po2.y - po1.y ) * t;
				if ( ( pPoint->vPos.y > (p1.y - EPS_VALUE)) && (pPoint->vPos.y < (p2.y + EPS_VALUE)) )
				{
					pPoint->vPos.x = p2.x;
					bFlag = true;
				}
			}
		}
	}

	if ( bFlag )
	{
		pPoint->vNorm = vso1.vNorm + ( vso2.vNorm - vso1.vNorm ) * t;
		Normalize( &(pPoint->vNorm) );
	}

	return bFlag;
}


static void GetSeaAreas( vector< vector<NDb::SVSOPoint> > *pRes, const vector<NDb::SVSOPoint> &samples, const CVec2 &p1, const CVec2 &p2 )
{
	if ( samples.size() < 2 )
		return;

	if ( areaCornersHash.empty() )
		InitCornersHash();

	vector<vector<NDb::SVSOPoint> > &res = *pRes;
	vector<NDb::SVSOPoint> addPoints( 128 );

	NDb::SVSOPoint p;
	vector<NDb::SVSOPoint>::const_iterator it1 = samples.begin();
	vector<NDb::SVSOPoint>::const_iterator it2 = samples.begin(); ++it2;
	for ( ; it1 != samples.end() && it2 != samples.end(); ++it1, ++it2 )
	{
		if ( GetIntersectWithArea( &p, *it1, *it2, p1, p2 ) )
		{
			addPoints.resize( 0 );
			addPoints.push_back( p );
			++it1; ++it2;

			for ( ; it2 != samples.end(); ++it1, ++it2 )
			{
				addPoints.push_back( *it1 );
				if ( GetIntersectWithArea(&p, *it1, *it2, p1, p2) )
					break;
			}
			if ( it2 != samples.end() )
				addPoints.push_back( p );

			if ( addPoints.size() >= 2 )
			{
				const int nInd1 = GetPointIndOnRect( addPoints[0], p1, p2 );
				const int nInd2 = GetPointIndOnRect( addPoints.back(), p1, p2 );

				CAreaCornersHash::const_iterator itFind = areaCornersHash.find( (nInd2 << 3) | nInd1 );
				if ( itFind != areaCornersHash.end() )
				{
					const vector<int> &cornersArr = itFind->second;
					for ( vector<int>::const_iterator it = cornersArr.begin(); it != cornersArr.end(); ++it )
					{
						GetCornerFromInd( &p, *it, p1, p2 );
						addPoints.push_back( p );
					}
				}

				res.push_back( addPoints );
			}
		}
	}
}


static void ClampArrayLower( CArray2D<float> *pArray, float fValidLowerValue )
{
	// clamp to over or equal to zero
	for ( int g = 0; g < pArray->GetSizeY(); ++g )
	{
		for ( int i = 0; i < pArray->GetSizeX(); ++i )
		{
			float &fVal = (*pArray)[g][i];
			if ( fVal < fValidLowerValue )
				fVal = fValidLowerValue;
		}
	}
}


static void SetMinOfTwoArrays( CArray2D<float> *pDst, const CArray2D<float> &src )
{
	for ( int g = 0; g < pDst->GetSizeY(); ++g )
	{
		for ( int i = 0; i < pDst->GetSizeX(); ++i )
		{
			float &fVal = (*pDst)[g][i];
			fVal = min( fVal, src[g][i] );
		}
	}
}


void CTerraGen::ReCreateAllWaterZones()
{
	waterParams.resize( 0 );

	terrainInfo.seaMask.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	terrainInfo.seaMask.FillZero();
	terrainInfo.waterHeightCoeffs.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	terrainInfo.waterHeightCoeffs.FillEvery( 1.0f );

	CArray2D<BYTE> coastMask;
	coastMask.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	coastMask.FillZero();
	CArray2D<float> coastHeights;
	coastHeights.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	coastHeights.FillZero();

	// place sea
	if ( pDesc->coast.points.size() >= 2 && pDesc->coast.pDescriptor != 0 )
	{
		const NDb::SCoastDesc *pCoastDesc = static_cast<const NDb::SCoastDesc *>( pDesc->coast.pDescriptor.GetPtr() );
		waterParams.push_back( NWaterStuff::SWaterParams() );
		waterParams.back() = *(pCoastDesc->pWater);
		//vector<NWaterStuff::SWaterParams>::iterator it = waterParams.insert( waterParams.end(), NWaterStuff::SWaterParams() );
		//*it = *( pCoastDesc->pWater );
		//FillSeaArea( pDesc->coast.points, pDesc->vCoastMidPoint, terrainInfo.seaMask, terrainInfo.waterHeightCoeffs );
		vector<vector<NDb::SVSOPoint> > seaAreas;
		GetSeaAreas( &seaAreas, pDesc->coast.points,
								 CVec2(Vis2AI(-DEF_SEA_OUTSIDE_MAP), Vis2AI(-DEF_SEA_OUTSIDE_MAP)),
								 CVec2(Vis2AI((float)(terrainInfo.heights.GetSizeX() - 1) * DEF_TILE_SIZE + DEF_SEA_OUTSIDE_MAP),
											 Vis2AI((float)(terrainInfo.heights.GetSizeY() - 1) * DEF_TILE_SIZE + DEF_SEA_OUTSIDE_MAP)) );
		for ( int i = 0; i < seaAreas.size(); ++i )
		{
			FillWaterArea( &terrainInfo.waterHeightCoeffs, -1.0f, &terrainInfo.seaMask, 1, seaAreas[i] );
		}
		SmoothHeightsFromSamples( &terrainInfo.waterHeightCoeffs, pDesc->coast.points );
		ClampArrayLower( &terrainInfo.waterHeightCoeffs, 0.0f );
	}

	// add lakes & islands
	vector<int> procLakes;
	vector<int> procIslands;
	procLakes.reserve( pDesc->lakes.size() );
	procIslands.reserve( pDesc->lakes.size() );
	for ( int i = 0; i < pDesc->lakes.size(); ++i )
	{
		const NDb::SLakeDesc *pLakeDesc = static_cast<const NDb::SLakeDesc *>( pDesc->lakes[i].pDescriptor.GetPtr() );
		// if LakeVSO is not lake then it is an island		
		if ( pLakeDesc->bIsLake )
			procLakes.push_back( i );
		else
			procIslands.push_back( i );
	}

	// affect lakes
	CArray2D<float> newHeights( terrainInfo.waterHeightCoeffs.GetSizeX(), terrainInfo.waterHeightCoeffs.GetSizeY() );
	for ( vector<int>::const_iterator it = procLakes.begin(); it != procLakes.end(); ++it )
	{
		const NDb::SVSOInstance &lake = pDesc->lakes[*it];
		const NDb::SLakeDesc *pLakeDesc = static_cast<const NDb::SLakeDesc *>( lake.pDescriptor.GetPtr() );
		const int nFillInd = GetWaterFillIndex( &waterParams, *(pLakeDesc->pWaterParams) );
		newHeights.FillEvery( 1.0f );

		FillWaterArea( &newHeights, -1.0f, &terrainInfo.seaMask, nFillInd, lake.points );
		SmoothHeightsFromSamples( &newHeights, lake.points );
		ClampArrayLower( &newHeights, 0.0f );
		SetMinOfTwoArrays( &terrainInfo.waterHeightCoeffs, newHeights );
	}

	// affect islands
	for ( vector<int>::const_iterator it = procIslands.begin(); it != procIslands.end(); ++it )
	{
		const NDb::SVSOInstance &island = pDesc->lakes[*it];
		const NDb::SLakeDesc *pLakeDesc = static_cast<const NDb::SLakeDesc *>( island.pDescriptor.GetPtr() );
		const int nFillInd = GetWaterFillIndex( &waterParams, *(pLakeDesc->pWaterParams) );

		FillWaterArea( &terrainInfo.waterHeightCoeffs, 1.0f, &terrainInfo.seaMask, 0, island.points );
	}

	// update heights in affected tiles
	for ( int g = 0; g < terrainInfo.tiles.GetSizeY(); ++g )
	{
		for ( int i = 0; i < terrainInfo.tiles.GetSizeX(); ++i )
		{
			if ( (terrainInfo.waterHeightCoeffs[g][i] < 1.0f) ||
					 (terrainInfo.waterHeightCoeffs[g][i + 1] < 1.0f) ||
					 (terrainInfo.waterHeightCoeffs[g + 1][i] < 1.0f) ||
					 (terrainInfo.waterHeightCoeffs[g + 1][i + 1] < 1.0f) )
			{
				STerrainInfo::STile &tile = terrainInfo.tiles[g][i];
				for ( vector<CVec3fEx>::iterator it = tile.vertices.begin(); it != tile.vertices.end(); ++it )
					it->z = GetTerraHeight( it->x, it->y, i, g );
			}
		}
	}

	// erase not used waterAddHeights
	for ( int g = 0; g < terrainInfo.waterHeightCoeffs.GetSizeY(); ++g )
	{
		for ( int i = 0; i < terrainInfo.waterHeightCoeffs.GetSizeX(); ++i )
		{
			if ( terrainInfo.waterHeightCoeffs[g][i] > (1.0f - EPS_VALUE) )
				terrainInfo.waterAddHeights[g][i] = 0.0f;
		}
	}

	// affect islands on seas and lakes
	for ( int g = 0; g < terrainInfo.seaMask.GetSizeY(); ++g )
	{
		for ( int i = 0; i < terrainInfo.seaMask.GetSizeX(); ++i )
		{
			if ( coastMask[g][i] )
			{
				if ( GetFullTerraHeight((float)i * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE) > FP_EPSILON )
				{
					if ( ((i - 1) >= 0) && ((i + 1) < terrainInfo.heights.GetSizeX()) &&
							 ((g - 1) >= 0) && ((g + 1) < terrainInfo.heights.GetSizeY()) )
					{
						if ( (GetFullTerraHeight((float)(i-1) * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE) > FP_EPSILON) &&
								 (GetFullTerraHeight((float)(i+1) * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE) > FP_EPSILON) &&
								 (GetFullTerraHeight((float)i * DEF_TILE_SIZE, (float)(g-1) * DEF_TILE_SIZE) > FP_EPSILON) &&
								 (GetFullTerraHeight((float)i * DEF_TILE_SIZE, (float)(g+1) * DEF_TILE_SIZE) > FP_EPSILON) )
						{
							terrainInfo.seaMask[g][i] = 0;
						}
					}
				}
			}
		}
	}
}


void CTerraGen::UpdateWater()
{
	CArray2D<BYTE> prevMask = terrainInfo.seaMask;
	CArray2D<float> prevWaterHeights = terrainInfo.waterHeightCoeffs;
	CVec2i vBBMin( INT_MAX, INT_MAX ), vBBMax( INT_MIN, INT_MIN );
	//ReCreateAllWaterZones();
	InitWater();

	if ( (prevMask.GetSizeX() == terrainInfo.seaMask.GetSizeX()) &&
			 (prevMask.GetSizeY() == terrainInfo.seaMask.GetSizeY()) &&
			 (prevWaterHeights.GetSizeX() == terrainInfo.waterHeightCoeffs.GetSizeX()) &&
			 (prevWaterHeights.GetSizeY() == terrainInfo.waterHeightCoeffs.GetSizeY()) )
	{
		for ( int g = 0; g < prevMask.GetSizeY(); ++g )
		{
			for ( int i = 0; i < prevMask.GetSizeX(); ++i )
			{
				if ( (prevMask[g][i] != terrainInfo.seaMask[g][i]) ||
						 (prevWaterHeights[g][i] != terrainInfo.waterHeightCoeffs[g][i]) )
				{
					vBBMin.x = min( vBBMin.x, i );
					vBBMin.y = min( vBBMin.y, g );
					vBBMax.x = max( vBBMax.x, i );
					vBBMax.y = max( vBBMax.y, g );
				}
			}
		}
		vBBMin.x = max( vBBMin.x, 0 );
		vBBMin.y = max( vBBMin.y, 0 );
		vBBMax.x = min( vBBMax.x, terrainInfo.tiles.GetSizeX() - 1 );
		vBBMax.y = min( vBBMax.y, terrainInfo.tiles.GetSizeY() - 1 );
	}
	else
	{
		NI_ASSERT( false, "Warning: change water will change all area" );
		vBBMin.Set( 0, 0 );
		vBBMax.Set( terrainInfo.tiles.GetSizeX() - 1, terrainInfo.tiles.GetSizeY() - 1 );
	}

	if ( (vBBMin.x <= vBBMax.x) && (vBBMin.y <= vBBMax.y) )
	{
		UpdateVectorAreaInfo( vBBMin.x, vBBMin.y, vBBMax.x, vBBMax.y, TERRAIN_UPDATE_ALL );
	}

	// grid updater (intellectual)
	if ( IScene *pScene = Scene() )
		pScene->UpdateGrid( vBBMin.x, vBBMin.y, vBBMax.x, vBBMax.y );
}


void CTerraGen::InitWater()
{
	TIME_STAT_START( CTerraGen__InitWater )

	ReCreateAllWaterZones();

	const float fAngle = ToRadian( pDesc->weather.nWindDirection );

	vector<NWaterStuff::SSurfBorder> waterBorders;

	if ( pDesc->coast.points.size() >= 2 && pDesc->coast.pDescriptor != 0 )
	{
		const NDb::SCoastDesc *pCoastDesc = static_cast_ptr<const NDb::SCoastDesc *>( pDesc->coast.pDescriptor );
		waterBorders.push_back( NWaterStuff::SSurfBorder() );
		waterBorders.back().points = pDesc->coast.points;
		waterBorders.back().pSurfMaterial = pCoastDesc->pWater->pWaterSet->pSurf;
	}

	for ( vector<NDb::SVSOInstance>::const_iterator it = pDesc->lakes.begin(); it != pDesc->lakes.end(); ++it )
	{
		if ( it->points.size() >= 2 )
		{
			const NDb::SLakeDesc *pLakeDesc = static_cast<const NDb::SLakeDesc *>( it->pDescriptor.GetPtr() );
			waterBorders.push_back( NWaterStuff::SSurfBorder() );
			waterBorders.back().points = it->points;
			waterBorders.back().pSurfMaterial = pLakeDesc->pWaterParams->pWaterSet->pSurf;
		}
	}

	waterController.Init( fAngle, terrainInfo.seaMask, waterParams, waterBorders, pGScene, pDesc->pOceanWater );

	// delete surfes, which are placed inside any river
	vector<NWaterStuff::SSurfBorder> surfBorders;
	vector<NDb::SVSOPoint> lastSurf;
	lastSurf.reserve( 512 );
	CVec3 vPos;
	for ( int i = 0; i < waterBorders.size(); ++i )
	{
		if ( waterBorders[i].pSurfMaterial )
		{
			lastSurf.resize( 0 );
			for ( vector<NDb::SVSOPoint>::const_iterator it = waterBorders[i].points.begin(); it != waterBorders[i].points.end(); ++it )
			{
				vPos = it->vPos;
				AI2Vis( &vPos );
				if ( !IsPointInsideRivers(vPos, -1) )
					lastSurf.push_back( *it );
				else
				{
					if ( lastSurf.size() >= 2 )
					{
						surfBorders.push_back( NWaterStuff::SSurfBorder() );
						surfBorders.back().points = lastSurf;
						surfBorders.back().pSurfMaterial = waterBorders[i].pSurfMaterial;
					}
					lastSurf.resize( 0 );
				}
			}
			if ( lastSurf.size() >= 2 )
			{
				surfBorders.push_back( NWaterStuff::SSurfBorder() );
				surfBorders.back().points = lastSurf;
				surfBorders.back().pSurfMaterial = waterBorders[i].pSurfMaterial;
			}
		}
	}
	surfController.Init( fAngle, CVec2i(terrainInfo.tiles.GetSizeX(), terrainInfo.tiles.GetSizeY()), surfBorders, pGScene );

	TIME_STAT_FINISH( CTerraGen__InitWater )
}


void CTerraGen::InitRainyWater( CDBPtr< NDb::SWater > pWater )
{
	if ( !pWater )
	{
		InitWater();
		return;
	}

	TIME_STAT_START( CTerraGen__InitRainyWater )

	ReCreateAllWaterZones();

	const float fAngle = ToRadian( pDesc->weather.nWindDirection );

	vector<NWaterStuff::SSurfBorder> waterBorders;

	if ( pDesc->coast.points.size() >= 2 && pDesc->coast.pDescriptor != 0 )
	{
		const NDb::SCoastDesc *pCoastDesc = static_cast_ptr<const NDb::SCoastDesc *>( pDesc->coast.pDescriptor );
		waterBorders.push_back( NWaterStuff::SSurfBorder() );
		waterBorders.back().points = pDesc->coast.points;
		waterBorders.back().pSurfMaterial = pCoastDesc->pWater->pWaterSet->pSurf;
	}

	for ( vector<NDb::SVSOInstance>::const_iterator it = pDesc->lakes.begin(); it != pDesc->lakes.end(); ++it )
	{
		if ( it->points.size() >= 2 )
		{
			const NDb::SLakeDesc *pLakeDesc = static_cast<const NDb::SLakeDesc *>( it->pDescriptor.GetPtr() );
			waterBorders.push_back( NWaterStuff::SSurfBorder() );
			waterBorders.back().points = it->points;
			waterBorders.back().pSurfMaterial = pLakeDesc->pWaterParams->pWaterSet->pSurf;
		}
	}

	for ( vector<NWaterStuff::SWaterParams>::iterator it = waterParams.begin(); it < waterParams.end(); ++it )
	{
		if ( it->eWaterType == pWater->eWaterType )
		{
			it->pMaterial = pWater->pWaterSet->water.pMaterial;
			it->bUseWaves = pWater->bUseWaves;
		}
	}

	waterController.Init( fAngle, terrainInfo.seaMask, waterParams, waterBorders, pGScene, pWater );

	// delete surfes, which are placed inside any river
	vector<NWaterStuff::SSurfBorder> surfBorders;
	vector<NDb::SVSOPoint> lastSurf;
	lastSurf.reserve( 512 );
	CVec3 vPos;
	for ( int i = 0; i < waterBorders.size(); ++i )
	{
		if ( waterBorders[i].pSurfMaterial )
		{
			lastSurf.resize( 0 );
			for ( vector<NDb::SVSOPoint>::const_iterator it = waterBorders[i].points.begin(); it != waterBorders[i].points.end(); ++it )
			{
				vPos = it->vPos;
				AI2Vis( &vPos );
				if ( !IsPointInsideRivers(vPos, -1) )
					lastSurf.push_back( *it );
				else
				{
					if ( lastSurf.size() >= 2 )
					{
						surfBorders.push_back( NWaterStuff::SSurfBorder() );
						surfBorders.back().points = lastSurf;
						surfBorders.back().pSurfMaterial = waterBorders[i].pSurfMaterial;
					}
					lastSurf.resize( 0 );
				}
			}
			if ( lastSurf.size() >= 2 )
			{
				surfBorders.push_back( NWaterStuff::SSurfBorder() );
				surfBorders.back().points = lastSurf;
				surfBorders.back().pSurfMaterial = waterBorders[i].pSurfMaterial;
			}
		}
	}
	surfController.Init( fAngle, CVec2i(terrainInfo.tiles.GetSizeX(), terrainInfo.tiles.GetSizeY()), surfBorders, pGScene );

	TIME_STAT_FINISH( CTerraGen__InitRainyWater )
}

