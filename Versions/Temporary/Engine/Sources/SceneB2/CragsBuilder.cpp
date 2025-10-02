#include "StdAfx.h"

#include "../Misc/Win32Random.h"
#include "GenTerrain.h"
#include "Scene.h"
#include "TerraHeight.h"
#include "VSOConsts.h"
#include "VersionInfo.h"

#define DEF_CRAG_HIGH_BORDER_RAND 1.0f
#define DEF_CRAG_HEIGHT 0.1f
#define DEF_CRAG_HEIGHT_ERROR 0.025f

static void MakeLeftSided( vector<STerrainInfo::SVSOPoint> *pDstArr, const vector<NDb::SVSOPoint> &srcArr )
{
	pDstArr->reserve( srcArr.size() );
	pDstArr->resize( 0 );
	vector<NDb::SVSOPoint>::const_iterator it = srcArr.end();
	do
	{
		--it;
		pDstArr->push_back( *it );
		pDstArr->back().vNorm = -it->vNorm;
	} while ( it != srcArr.begin() );
}

void CTerraGen::AddCrag( const NDb::SVSOInstance *pInstance )
{
	int nSeed = -1;
	if ( AddCrag(pInstance, GetVSOSeed(pInstance)) )
	{
		CragManipulator( FindCragInfo(pInstance->nVSOID), false );
		if ( pAIObserver )
			PutCragToAI( pInstance );
	}
}

bool CTerraGen::AddCrag( const NDb::SVSOInstance *pInstance, const int nRandSeed )
{
	NI_VERIFY( pInstance, "CTerraGen::AddCrag - Invalid crag instance", return false )

	// if such crag was already built then skip building
	for ( list<STerrainInfo::SCrag>::const_iterator it = terrainInfo.crags.begin(); it != terrainInfo.crags.end(); ++it )
	{
		if ( it->nID == pInstance->nVSOID )
			return false;
	}

	if ( pInstance->points.empty() )
		return false;

	STerrainInfo::SCrag newCragInfo;
	NWin32Random::Seed( nRandSeed );
	newCragInfo.nRandSeed = NWin32Random::GetSeed();
	newCragInfo.nID = pInstance->nVSOID;
	newCragInfo.pDesc = static_cast<const NDb::SCragDesc *>( pInstance->pDescriptor.GetPtr() );

	const bool bLeftSided = newCragInfo.pDesc->bLeftSided;

	// convert right-sided crags to left sided
	if ( !bLeftSided )
		MakeLeftSided( &newCragInfo.sampPoints, pInstance->points );
	else
	{
		newCragInfo.sampPoints.resize( 0 );

		for ( vector<NDb::SVSOPoint>::const_iterator itSampPoint = pInstance->points.begin(); itSampPoint != pInstance->points.end(); ++itSampPoint )
		{
			newCragInfo.sampPoints.push_back( STerrainInfo::SVSOPoint(*itSampPoint) );
		}
	}

	// add some random to crag's border
	for ( vector<STerrainInfo::SVSOPoint>::iterator it = newCragInfo.sampPoints.begin(); it != newCragInfo.sampPoints.end(); ++it )
	{
		const float fDist = NWin32Random::Random( -newCragInfo.pDesc->fBorderRand * 0.5f, newCragInfo.pDesc->fBorderRand * 0.5f );
		AI2Vis( &(it->vPos.x) );
		AI2Vis( &(it->vPos.y) );
		AI2Vis( &(it->vPos.z) );
		AI2Vis( &(it->fWidth) );
		it->vPos.x += it->vNorm.x * fDist;
		it->vPos.y += it->vNorm.y * fDist;
	}

	// create ridge
	newCragInfo.ridge.reserve( newCragInfo.sampPoints.size() * 2 );
	newCragInfo.vBBMin.Set( FP_MAX_VALUE, FP_MAX_VALUE );
	newCragInfo.vBBMax.Set( -FP_MAX_VALUE, -FP_MAX_VALUE );

	vector<STerrainInfo::SVSOPoint>::const_iterator it = newCragInfo.sampPoints.begin();
	while ( it != newCragInfo.sampPoints.end() )
	{
		const CVec2 vPos( it->vPos.x - it->vNorm.x * DEF_CRAG_HOLE_WIDTH,
											it->vPos.y - it->vNorm.y * DEF_CRAG_HOLE_WIDTH );
		newCragInfo.ridge.push_back( CVec3fEx(vPos.x, vPos.y, it->vPos.z, 0) );
		newCragInfo.vBBMin.Minimize( vPos );
		newCragInfo.vBBMax.Maximize( vPos );
		++it;
	}
	it = newCragInfo.sampPoints.end();
	do
	{
		--it;
		const CVec2 vPos( it->vPos.x - it->vNorm.x * (DEF_CRAG_HOLE_WIDTH + it->fWidth),
											it->vPos.y - it->vNorm.y * (DEF_CRAG_HOLE_WIDTH + it->fWidth) );
		newCragInfo.ridge.push_back( CVec3fEx(vPos.x, vPos.y, it->vPos.z, 0) );
		newCragInfo.vBBMin.Minimize( vPos );
		newCragInfo.vBBMax.Maximize( vPos );
	} while( it != newCragInfo.sampPoints.begin() );

	newCragInfo.vBBMin.x -= DEF_CRAG_HOLE_WIDTH;
	newCragInfo.vBBMin.y -= DEF_CRAG_HOLE_WIDTH;
	newCragInfo.vBBMax.x += DEF_CRAG_HOLE_WIDTH;
	newCragInfo.vBBMax.y += DEF_CRAG_HOLE_WIDTH;

	newCragInfo.vRidgeMin.Set( Clamp(int(newCragInfo.vBBMin.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1),
														 Clamp(int(newCragInfo.vBBMin.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1) );
	newCragInfo.vRidgeMax.Set( Clamp(int(newCragInfo.vBBMax.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1),
														 Clamp(int(newCragInfo.vBBMax.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1) );

	terrainInfo.crags.push_back( newCragInfo );

	return true;
}

void CTerraGen::CragsPushPointLR( vector<CVec3fEx> *pLeft, vector<CVec3fEx> *pRight, const STerrainInfo::SVSOPoint &point, CVec3 *pVMin, CVec3 *pVMax, const int nID )
{
	const CVec3 v1 = point.vPos - point.vNorm * DEF_CRAG_HOLE_WIDTH;
	const CVec3 v2 = point.vPos + point.vNorm * DEF_CRAG_HOLE_WIDTH;

	pLeft->push_back( CVec3fEx(v1.x, v1.y, GetTerraHeight(v1.x, v1.y), 0) );
	pRight->push_back( CVec3fEx(v2.x, v2.y, GetTerraHeight(v2.x, v2.y), 1) );

	pVMin->Minimize( v1 );
	pVMin->Minimize( v2 );
	pVMax->Maximize( v1 );
	pVMax->Maximize( v2 );
}

#define DEF_EXACT_EPS 0.001f

void CTerraGen::CragsPushExactPoint( vector<STerrainInfo::SVSOPoint> *pArray, const STerrainInfo::SVSOPoint &p1, const STerrainInfo::SVSOPoint &p2,
																const float t1, const float t2, const int nExcludeID, const bool bStart )
{
	const float t = ( t1 + t2 ) * 0.5f;
	const CVec3 vPos = p1.vPos + (p2.vPos - p1.vPos) * t;

#ifdef VERSION_DEV_M1
	const CVec3 vPosUp = vPos - p1.vPos + (p2.vPos - p1.vPos) * t * 3.0f;
	const float fMaxHeight1 = GetMaxCragHeight( CVec2(vPos.x, vPos.y), nExcludeID );
	const float fMaxHeight2 = 0;// GetTerraHeight( vPos.x, vPos.y );
	const float fMaxHeight = max( fMaxHeight1, fMaxHeight2 );
#else // old code
	const float fMaxHeight = GetMaxCragHeight( CVec2(vPos.x, vPos.y), nExcludeID );
#endif

	if ( ((t2 - t1) < DEF_EXACT_EPS) || (fabs(vPos.z - fMaxHeight) < DEF_EXACT_EPS) )
	{
		NDb::SVSOPoint p;
		p.vPos = vPos;
		p.vNorm = p1.vNorm + ( p2.vNorm - p1.vNorm ) * t;
		p.fRadius = vPos.z /*- GetTerraHeight( vPos.x, vPos.y )*/ - fMaxHeight;
		pArray->push_back( p );
	}
	else
	{
		if ( !bStart )
		{
			if ( vPos.z > fMaxHeight )
				CragsPushExactPoint( pArray, p1, p2, t, t2, nExcludeID, bStart );
			else
				CragsPushExactPoint( pArray, p1, p2, t1, t, nExcludeID, bStart );
		}
		else
		{
			if ( vPos.z > fMaxHeight )
				CragsPushExactPoint( pArray, p1, p2, t1, t, nExcludeID, bStart );
			else
				CragsPushExactPoint( pArray, p1, p2, t, t2, nExcludeID, bStart );
		}
	}
}

void CTerraGen::PutCragOnTerrain( STerrainInfo::SCrag *pCrag, const CVec2i &vMinTile, const CVec2i &vMaxTile )
{

#ifdef VERSION_DEV_M1
	if ( pCrag->sampPoints.size() < 2 )
		return;

	STerrainInfo::SFoot foot;
	foot.nID = pCrag->nID;
	foot.pFootMaterial = pCrag->pDesc->pFootMaterial;
	foot.fTexGeomScale = pCrag->pDesc->fRidgeTexGeomScale;

	CVec3 vMin, vMax;
	vMin.Set( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE );
	vMax.Set( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );
	//hole.resize( 0 );

	pCrag->precVerts.resize( 0 );
	pCrag->precHeights.resize( 0 );
	pCrag->precNorms.resize( 0 );

	//	collect hole points
	vector<STerrainInfo::SVSOPoint>::const_iterator endVSOPoint = pCrag->sampPoints.end();
	--endVSOPoint;
	int nHoleIndex = 0;
	for ( vector<STerrainInfo::SVSOPoint>::const_iterator itVSOPoint = pCrag->sampPoints.begin(); itVSOPoint != pCrag->sampPoints.end(); ++itVSOPoint, ++nHoleIndex )
	{
		const STerrainInfo::SVSOPoint &point = (*itVSOPoint);

		CVec3 vLowerPos = point.vPos;
		vLowerPos.z = GetLowerHeight( point );

		CVec3 vUpperPos = point.vPos - point.vNorm * DEF_CRAG_HOLE_WIDTH;
		vUpperPos.z = GetUpperHeight( point );

		if ( (itVSOPoint == pCrag->sampPoints.begin()) || (itVSOPoint == endVSOPoint) )
		{
			vLowerPos.z = GetTerraHeight( vUpperPos.x, vUpperPos.y );
			vUpperPos.z = vLowerPos.z + DEF_CRAG_HEIGHT_ERROR;
		}

		vMin.Minimize( vLowerPos );
		vMin.Minimize( vUpperPos );
		vMax.Maximize( vLowerPos );
		vMax.Maximize( vUpperPos );

		pCrag->precVerts.push_back( vUpperPos );
		pCrag->precVerts.back().z = vLowerPos.z;
		pCrag->precHeights.push_back( vUpperPos.z );
		pCrag->precNorms.push_back( point.vNorm );
	}

	const int nTileX1 = Clamp( int(vMin.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY1 = Clamp( int(vMin.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );
	const int nTileX2 = Clamp( int(vMax.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY2 = Clamp( int(vMax.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );

	for ( int i = 0; i < pCrag->precVerts.size() - 1; ++i )
	{
		vector<CVec3fEx> hole( 4 );

		CVec3 v1( pCrag->precVerts[i] - pCrag->precNorms[i] * DEF_CRAG_HOLE_WIDTH );
		CVec3 v2( pCrag->precVerts[i+1] - pCrag->precNorms[i+1] * DEF_CRAG_HOLE_WIDTH );
		hole[0] = CVec3fEx( pCrag->precVerts[i], 0 );
		hole[1] = CVec3fEx( v1.x, v1.y, pCrag->precHeights[i], 0 );
		hole[2] = CVec3fEx( v2.x, v2.y, pCrag->precHeights[i+1], 0 );
		hole[3] = CVec3fEx( pCrag->precVerts[i+1], 0 );

		MakeHole( hole, nTileX1, nTileY1, nTileX2, nTileY2 );
	}

	// add foot
	foot.points.push_back( pCrag->sampPoints );

	vector<CVec3>::const_iterator itPrecVert = pCrag->precVerts.begin();
	for ( vector<STerrainInfo::SVSOPoint>::iterator itPoint = foot.points.back().begin(); itPoint != foot.points.back().end(); ++itPoint, ++itPrecVert )
	{
		itPoint->vPos = (*itPrecVert) + itPoint->vNorm * DEF_CRAG_HOLE_WIDTH * 4.0f;
		itPoint->fRadius += NWin32Random::Random( 0.0f, pCrag->pDesc->fDepth ) * 4.0f;
	}

	// add precipice for just added crag
	AddToPrecipiceUpdateQueue( pCrag->nID );
	// add foot for just added crag
	terrainInfo.foots.push_back( foot );
	needAddFoots.push_back( foot.nID );

#else	// B2

	if ( pCrag->sampPoints.size() < 2 )
		return;

	vector<CVec3fEx> samplesL, samplesR;
	samplesL.reserve( pCrag->sampPoints.size() * 2 );
	samplesR.reserve( pCrag->sampPoints.size() );
	vector<STerrainInfo::SVSOPoint>::const_iterator itPointPrev = pCrag->sampPoints.begin();
	vector<STerrainInfo::SVSOPoint>::const_iterator itPoint = itPointPrev;
	++itPoint;
	CVec3 vMin, vMax;

	static vector<STerrainInfo::SVSOPoint> precPoints( 128 );

	STerrainInfo::SFoot foot;
	foot.nID = pCrag->nID;
	foot.pFootMaterial = pCrag->pDesc->pFootMaterial;
	foot.fTexGeomScale = pCrag->pDesc->fRidgeTexGeomScale;

	float fMaxCragHeight;

	while ( 1 )
	{
		samplesL.resize( 0 );
		samplesR.resize( 0 );
		precPoints.resize( 0 );
		vMin.Set( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE );
		vMax.Set( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );

		while ( (itPoint != pCrag->sampPoints.end()) && (itPoint->vPos.z < GetMaxCragHeight(CVec2(itPoint->vPos.x, itPoint->vPos.y), pCrag->nID)) )
		{
			++itPoint;
			++itPointPrev;
		}
		if ( itPoint == pCrag->sampPoints.end() )
			break;
		CragsPushExactPoint( &precPoints, *itPointPrev, *itPoint, 0.0f, 1.0f, pCrag->nID, true );
		CragsPushPointLR( &samplesL, &samplesR, precPoints.back(), &vMin, &vMax, pCrag->nID );
		while ( (itPoint != pCrag->sampPoints.end()) && (itPoint->vPos.z >= (fMaxCragHeight = GetMaxCragHeight(CVec2(itPoint->vPos.x, itPoint->vPos.y), pCrag->nID))) )
		{
			precPoints.push_back( *itPoint );
			precPoints.back().fRadius = itPoint->vPos.z /*- GetTerraHeight( itPoint->vPos.x, itPoint->vPos.y )*/ - fMaxCragHeight;
			CragsPushPointLR( &samplesL, &samplesR, *itPoint, &vMin, &vMax, pCrag->nID );

			++itPointPrev;
			++itPoint;
		}

		if ( itPoint != pCrag->sampPoints.end() )
		{
			CragsPushExactPoint( &precPoints, *itPointPrev, *itPoint, 0.0f, 1.0f, pCrag->nID, false );
			CragsPushPointLR( &samplesL, &samplesR, precPoints.back(), &vMin, &vMax, pCrag->nID );
		}

		if ( samplesL.size() >= 2 )
		{
			vector<CVec3fEx>::const_iterator it = samplesR.end();
			do {
				--it;
				samplesL.push_back( *it );
			} while( it != samplesR.begin() );

			if ( vMinTile.x == -1 )
			{
				const int nTileX1 = Clamp( int(vMin.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
				const int nTileY1 = Clamp( int(vMin.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );
				const int nTileX2 = Clamp( int(vMax.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
				const int nTileY2 = Clamp( int(vMax.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );
				MakeHole( samplesL, nTileX1, nTileY1, nTileX2, nTileY2 );
			}
			else
				MakeHole( samplesL, vMinTile.x, vMinTile.y, vMaxTile.x, vMaxTile.y );

			// add foot
			foot.points.push_back( precPoints );
		}
	}

	// create precipice for this crag
	pCrag->precVerts.resize( 0 );
	pCrag->precHeights.resize( 0 );
	pCrag->precNorms.resize( 0 );

	for ( vector<STerrainInfo::SVSOPoint>::const_iterator it = pCrag->sampPoints.begin(); it != pCrag->sampPoints.end(); ++it )
	{
		const CVec3 vPos = it->vPos - it->vNorm * DEF_CRAG_HOLE_WIDTH;
		const float fTerraHeight = GetTerraHeight( vPos.x, vPos.y );
		const float fMaxCragH = GetMaxCragHeight( CVec2(vPos.x, vPos.y), pCrag->nID );
		pCrag->precVerts.push_back( CVec3(vPos.x, vPos.y, fTerraHeight + (fMaxCragH < vPos.z ? fMaxCragH : vPos.z)) );
		pCrag->precHeights.push_back( fTerraHeight + vPos.z );
		if ( pCrag->precHeights.back() < (pCrag->precVerts.back().z + DEF_CRAG_HEIGHT_ERROR) )
			pCrag->precVerts.back().z -= DEF_CRAG_HEIGHT_ERROR;
		pCrag->precNorms.push_back( it->vNorm );
	}

	AddToPrecipiceUpdateQueue( pCrag->nID );

	terrainInfo.foots.push_back( foot );
	needAddFoots.push_back( foot.nID );
#endif

}

void CTerraGen::PutAllCragsOnTerrain()
{
	for ( list<STerrainInfo::SCrag>::iterator it = terrainInfo.crags.begin(); it != terrainInfo.crags.end(); ++it )
		PutCragOnTerrain( &(*it), CVec2i(-1, -1), CVec2i(-1, -1) );

	//	statistcis
	const bool bShowDebugInfo_IntersectionsCount = false;
	if ( bShowDebugInfo_IntersectionsCount )
	{
		int nIntersectionsCount = 0;
		int nCragsCount = terrainInfo.crags.size();

		for ( list<STerrainInfo::SCrag>::const_iterator itCrag1 = terrainInfo.crags.begin(); itCrag1 != terrainInfo.crags.end(); ++itCrag1 )
		{
			const STerrainInfo::SCrag &crag1 = (*itCrag1);
			vector<CVec3dEx> poly1( crag1.precVerts.size() );
			poly1.resize( 0 );

			for ( vector<CVec3>::const_iterator itPoint1 = crag1.precVerts.begin(); itPoint1 != crag1.precVerts.end(); ++itPoint1 )
			{
				poly1.push_back( CVec3dEx(*itPoint1, 0) );
			}

			list<STerrainInfo::SCrag>::const_iterator itCrag1ex = itCrag1;
			++itCrag1ex;
			for ( list<STerrainInfo::SCrag>::const_iterator itCrag2 = itCrag1ex; itCrag2 != terrainInfo.crags.end(); ++itCrag2 )
			{
				const STerrainInfo::SCrag &crag2 = (*itCrag2);

				vector<CVec3>::const_iterator itPoint = crag2.precVerts.begin();
				vector<CVec3>::const_iterator itPointPrev = itPoint;
				++itPoint;

				while ( itPoint != crag2.precVerts.end() )
				{
					if ( IsIntersect(poly1, CVec3dEx(*itPointPrev, 0), CVec3dEx(*itPoint, 0)) )
					{
						++nIntersectionsCount;
					}

					itPointPrev = itPoint;
					++itPoint;
				}
			}
		}

		DebugTrace( "MAP DEBUG INFO: %d crags found; %d intersections", nCragsCount, nIntersectionsCount );
	}
}

static bool IsGridPoint( const CVec3fEx &vPoint )
{
	return ( fabs(sqr(fmodf(vPoint.x, DEF_TILE_SIZE)) + sqr(fmodf(vPoint.y, DEF_TILE_SIZE))) < DEF_EPS2 );
}

static CVec2i GetGridPoint( const CVec3fEx &vPoint )
{
	return CVec2i( vPoint.x / DEF_TILE_SIZE, vPoint.y / DEF_TILE_SIZE );
}

void CTerraGen::UpdateHeightsAfterCrags( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2 )
{

#ifdef VERSION_DEV_M1
	return;
#endif

	for ( int g = nTileY1; g < nTileY2; ++g )
	{
		for ( int i = nTileX1; i < nTileX2; ++i )
		{
			STerrainInfo::STile &curTile = terrainInfo.tiles[g][i];
			//
			int nCount = 0;
			for ( vector<CVec3fEx>::iterator it = curTile.vertices.begin(); it != curTile.vertices.end(); ++it, ++nCount )
			{
				it->z = GetTerraHeight( it->x, it->y );
				float fHeight = 0.0f;
				const CVec2 v( it->x, it->y );
				GetMaxCragHeightEx( v, &fHeight );
				//terrainInfo.heights[g][i] += fHeight;
				curTile.addHeights[nCount] = fHeight;// + GetMaxRiverHeight( v );
			}
		}
	}
	terrainAIInfo.heights.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	for ( int g = nTileY1; g <= nTileY2; ++g )
	{
		for ( int i = nTileX1; i <= nTileX2; ++i )
		{
			const CVec2 vTile( (float)i * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE );
			float fMaxCragH = 0;
			GetMaxCragHeightEx( vTile, &(terrainInfo.addHeights[g][i]) );
			//terrainInfo.addHeights[g][i] += GetMaxRiverHeight( vTile );
			//terrainInfo.riverHeights[g][i] = GetMaxRiverHeight( vTile );
			terrainAIInfo.heights[g][i] = Vis2AI( GetFullTerraHeight(i, g) );
		}
	}
}

// ridge height calculations

struct SRidgeProfile
{
	float operator()( const float x ) const
	{ 
		return 1.0f - x * x;
	}
};

float CTerraGen::GetMaxCragHeight( const CVec2 &v, const int nExcludeID ) const
{
	float fMaxHeight = 0.0f;
	for ( list<STerrainInfo::SCrag>::const_iterator it = terrainInfo.crags.begin(); it != terrainInfo.crags.end(); ++it )
	{
		if ( it->nID != nExcludeID )
		{
			if ( IsInsideBB(v, it->vBBMin, it->vBBMax) )
			{
				SRidgeProfile ridgeProfile;
				float fHeight;
				if ( GetIncRidgeHeight(v, it->ridge, &fHeight, ridgeProfile) )
					fMaxHeight = max( fMaxHeight, fHeight );
			}
		}
	}
	//return max( fMaxHeight, GetTerraHeight(v.x, v.y) );
	return fMaxHeight;
}

float CTerraGen::GetMaxCragHeight2( const CVec2 &v, const int nExcludeID1, const int nExcludeID2 ) const
{
	float fMaxHeight = 0.0f, fHeight;
	SRidgeProfile ridgeProfile;
	for ( list<STerrainInfo::SCrag>::const_iterator it = terrainInfo.crags.begin(); it != terrainInfo.crags.end(); ++it )
	{
		if ( (it->nID != nExcludeID1) && (it->nID != nExcludeID2) )
		{
			if ( IsInsideBB(v, it->vBBMin, it->vBBMax) )
			{
				if ( GetIncRidgeHeight(v, it->ridge, &fHeight, ridgeProfile) )
					fMaxHeight = max( fMaxHeight, fHeight );
			}
		}
	}
	//return max( fMaxHeight, GetTerraHeight(v.x, v.y) );
	return fMaxHeight;
}

bool CTerraGen::GetMaxCragHeightEx( const CVec2 &v, float *pHeight ) const
{
	(*pHeight) = 0.0f;

	if ( terrainInfo.crags.empty() )
		return false;
	float fH;
	bool bResult = false;
	SRidgeProfile ridgeProfile;
	for ( list<STerrainInfo::SCrag>::const_iterator it = terrainInfo.crags.begin(); it != terrainInfo.crags.end(); ++it )
	{
		if ( IsInsideBB(v, it->vBBMin, it->vBBMax) )
		{
			if ( GetIncRidgeHeight(v, it->ridge, &fH, ridgeProfile) )
			{
				(*pHeight) = max( (*pHeight), fH );
				bResult = true;
			}
		}
	}
	//(*pHeight) = max( (*pHeight), GetTerraHeight(v.x, v.y) );

	return bResult;
}

void CTerraGen::CragManipulator( const STerrainInfo::SCrag *pCrag, const bool bRemove )
{
	vector<int> updatedCrags( 64 );
	vector<int> updatedRivers( 64 );

	const CVec2i vBBMin = pCrag->vRidgeMin;
	const CVec2i vBBMax = pCrag->vRidgeMax;
	CollectAllCragsAndRiversInArea( &updatedCrags, &updatedRivers, vBBMin, vBBMax, pCrag->nID, -1 );

	if ( bRemove )
	{
		RemovePrecipiceFromCollector( pCrag->nID, false );
		RemoveFoot( pCrag->nID );
		RemoveCragInfo( pCrag->nID );
	}
	else
	{
		AddUnique( &updatedCrags, pCrag->nID );
	}

	UpdateCragsAndRiversInArea( updatedCrags, updatedRivers, vBBMin, vBBMax );

	UpdateArea( vBBMin.x, vBBMin.y, vBBMax.x + 1, vBBMax.y + 1, TERRAIN_UPDATE_ALL );
	UpdateAllOnTerrainObjectsInArea( vBBMin, vBBMax );

	// grid updater (intellectual)
	if ( IScene *pScene = Scene() )
		pScene->UpdateGrid( vBBMin.x, vBBMin.y, vBBMax.x, vBBMax.y );
}

void CTerraGen::RemoveCrag( const int nVSOID )
{
	// if such crag is not exists, than skip it removing
	list<STerrainInfo::SCrag>::const_iterator itCrag = terrainInfo.crags.begin();
	for ( list<STerrainInfo::SCrag>::const_iterator itCrag = terrainInfo.crags.begin(); itCrag != terrainInfo.crags.end(); ++itCrag )
	{
		if ( itCrag->nID == nVSOID )
		{
			CragManipulator( &(*itCrag), true );
			return;
		}
	}
	NI_ASSERT( false, StrFmt("Removed crag is not exists: %d", nVSOID) );
}

void CTerraGen::UpdateCrag( const int nVSOID )
{
	RemoveCrag( nVSOID );
	AddCrag( FindCrag(nVSOID) );
}

void CTerraGen::RemoveCragInfo( const int nVSOID )
{
	for ( list<STerrainInfo::SCrag>::iterator it = terrainInfo.crags.begin(); it != terrainInfo.crags.end(); ++it )
	{
		if ( it->nID == nVSOID )
		{
			terrainInfo.crags.erase( it );
			return;
		}
	}
	NI_ASSERT( false, StrFmt("Couldn't find info for crag with ID=%d", nVSOID) );
}

void CTerraGen::AddAllCrags()
{
	NI_ASSERT( pDesc, "Terrain is not loaded" );
	//RemoveAllCrags();
	for ( vector<NDb::SVSOInstance>::const_iterator it = pDesc->crags.begin(); it != pDesc->crags.end(); ++it )
		AddCrag( &(*it), GetVSOSeed(it) );
}

inline const NDb::SVSOInstance* CTerraGen::FindCrag( int nID ) const
{
	for ( vector<NDb::SVSOInstance>::const_iterator it = pDesc->crags.begin(); it != pDesc->crags.end(); ++it )
	{
		if ( it->nVSOID == nID )
			return static_cast<const NDb::SVSOInstance *>( &(*it) );
	}
	//NI_ASSERT( false, StrFmt("Couldn't find crag's instance: ID=%d", nID) );
	return 0;
}

inline STerrainInfo::SCrag* CTerraGen::FindCragInfo( int nID )
{
	for ( list<STerrainInfo::SCrag>::iterator it = terrainInfo.crags.begin(); it != terrainInfo.crags.end(); ++it )
	{
		if ( it->nID == nID )
			return ( &(*it) );
	}
	//NI_ASSERT( false, StrFmt("Couldn't find crag's info: ID=%d", nID) );
	return 0;
}

bool CTerraGen::GetCragPrecVerts( vector<CVec3> *pVerts, int nVSOId )
{
	pVerts->clear();
	//
	for ( list<STerrainInfo::SCrag>::const_iterator it = terrainInfo.crags.begin(); it != terrainInfo.crags.end(); ++it )
	{
		if ( it->nID == nVSOId )
		{
			*pVerts = it->precVerts;
			return true;
		}
	}
	return false;
}


