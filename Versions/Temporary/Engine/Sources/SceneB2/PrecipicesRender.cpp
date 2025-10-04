#include "stdafx.h"

#include "Misc/Win32Random.h"
#include "System/FastMath.h"
#include "3Dmotor/DBScene.h"
#include "GenTerrain.h"
#include "VersionInfo.h"

#define DEF_PRECIPICE_SAMPLES_PER_PATCH 4
//
#define DEF_PRECIPICE_SAMP_HEIGHT 1.0f
#define DEF_PRECIPICE_INV_SAMP_HEIGHT (1.0f / DEF_PRECIPICE_SAMP_HEIGHT)
#define DEF_PRECIPICE_DEPTH 0.15f
#define DEF_PRECIPICE_DEPTH_RAND 0.6f
#define DEF_PRECIPICE_RANDX 0.25f
#define DEF_PRECIPICE_RANDY 0.65f
//
#define DEF_PRECIPICE_SMOOTH_RAD ( DEF_TILE_SIZE * 5.0f )
#define DEF_PRECIPICE_SMOOTH_RAD2 ( DEF_PRECIPICE_SMOOTH_RAD * DEF_PRECIPICE_SMOOTH_RAD )
#define DEF_PRECIPICE_SMOOTH_RAD2_INV ( 1.0f / DEF_PRECIPICE_SMOOTH_RAD2 )
#define DEF_STAYED_ON_TERRAIN_ERROR DEF_RIVER_DEPTH/*DEF_TILE_SIZE*/
//
#define DEF_MIN_PRECIPICE_HEIGHT 0.025f

struct STriangleEx
{
	int i1, i2, i3;
};

CVec3 CTerraGen::GetSmoothPrecipiceNorm( const CVec2 &vPos )
{
	float fWeight = 0.0f;
	CVec3 vNorm(0, 0, 0);
	for ( vector<STerrainInfo::SPrecipiceNode>::const_iterator it = terrainInfo.precNodes.begin(); it != terrainInfo.precNodes.end(); ++it )
	{
		if ( it->nCount > 0 )
		{
			const float fDist = fabs2( vPos - it->vPos );
			if ( fDist < DEF_PRECIPICE_SMOOTH_RAD2 )
			{
				const float fCoeff = ( DEF_PRECIPICE_SMOOTH_RAD2 - fDist ) * DEF_PRECIPICE_SMOOTH_RAD2_INV;
				vNorm += it->vNorm * fCoeff / it->nCount;
				fWeight += fCoeff;
			}
		}
	}
	NI_ASSERT( fWeight > (1.0f - DEF_EPS), "Precipice smooth error" );
	vNorm /= fWeight;
	const float fNormDist = fabs2( vNorm );
	//if ( ( fabs( fNormDist - 1.0f ) > DEF_EPS ) && ( fNormDist > DEF_EPS ) )
	if ( fNormDist > (1.0f + DEF_EPS) )
	{
		const float d = 1.0f / NMath::Sqrt( fNormDist );
		vNorm.x *= d;
		vNorm.y *= d;
		vNorm.z *= d;
		//Normalize( &vNorm );
	}
	return vNorm;
	//return ( vNorm / fWeight );
}

inline void AddUniquePrecipiceVertex( STerrainInfo::SPrecipiceNode *pNode, const float fHeight )
{
	int nInd = 0;
	for ( ; nInd < pNode->verts.size(); ++nInd )
	{
		if ( fabs( pNode->verts[nInd].z - fHeight ) < DEF_EPS )
		{
			if	( ( nInd == 0 ) || ( nInd == ( pNode->verts.size() - 1 ) ) )
				return;
			if ( ( fabs( pNode->verts[nInd].x - pNode->vPos.x ) < DEF_EPS ) &&
					 ( fabs( pNode->verts[nInd].y - pNode->vPos.y ) < DEF_EPS ) )
				return;
		}
	}
	// add new vertex
	vector<CVec3>::iterator itFind = pNode->verts.begin();
	for ( ; itFind != pNode->verts.end(); ++itFind )
	{
		if ( itFind->z < fHeight )
			break;
	}
	pNode->verts.insert( itFind, CVec3( pNode->vPos.x, pNode->vPos.y, fHeight ) );
}

void CTerraGen::CreateVerticesInPrecipiceNode( STerrainInfo::SPrecipiceNode *pNode, const int nNodeInd )
{
	if ( !pNode )
	{
		NI_ASSERT( pNode, "Wrong node" );
		return;
	}

	if ( pNode->nCount == 0 )
		return;

	NI_ASSERT( pNode->nCount > 0, "Unused precipice node was found" );
	if ( pNode->fMinHeight > pNode->fMaxHeight )
		return;

	const CVec3 vNorm = GetSmoothPrecipiceNorm( pNode->vPos );

	const float fHeight = pNode->fMaxHeight - pNode->fMinHeight;
	const int nVertsNum = int( fHeight * DEF_PRECIPICE_INV_SAMP_HEIGHT ) + 2;
	const float fInvVertsNum = 1.0f / ( nVertsNum - 1 );

	const float fVertsHalfNum = (float)( nVertsNum - 1 ) * 0.5f;
	const float fInvVertsHalfNum = 1.0f / fVertsHalfNum;

	float fDepthRand = 0.0f;
	float fRandX = 0.0f;
	float fRandY = 0.0f;
	const int nVertsNumCoeff = 1.0f / nVertsNum;

	CVec3 vPos;

	pNode->verts.reserve( nVertsNum );
	pNode->verts.resize( 0 );

	const float fPrecipiceDepth = ( pNode->nCount > 0 ) ? ( pNode->fDepth / pNode->nCount ) : DEF_PRECIPICE_DEPTH;
	const float fPrecipiceDepthRand = ( pNode->nCount > 0 ) ? ( pNode->fDepthRand / pNode->nCount ) : DEF_PRECIPICE_DEPTH_RAND;
	const float fPrecipiceRandX = ( pNode->nCount > 0 ) ? ( pNode->fRandX / pNode->nCount ) : DEF_PRECIPICE_RANDX;
	const float fPrecipiceRandY = ( pNode->nCount > 0 ) ? ( pNode->fRandY / pNode->nCount ) : DEF_PRECIPICE_RANDY;

	for ( int g = nVertsNum - 1; g >= 0; --g )
	{
		vPos.Set( pNode->vPos.x, pNode->vPos.y, pNode->fMinHeight + ( pNode->fMaxHeight - pNode->fMinHeight) * fInvVertsNum * g );
		if ( g < ( nVertsNum - 1 ) )
		{
			const float fDepth = ( 1.0f - fabs2( (float)( g - fVertsHalfNum ) * fInvVertsHalfNum ) ) * fPrecipiceDepth * nVertsNum;
			const float fDampingCoeff = ( nVertsNum - 1 - g ) * fInvVertsNum;
			fDepthRand += NWin32Random::Random( 0.0f, fDampingCoeff * fPrecipiceDepthRand ) * ( 1.0f - (float)g * nVertsNumCoeff );
			fRandX += NWin32Random::Random( -fPrecipiceRandX, fPrecipiceRandX );
			fRandY = ( fRandY + NWin32Random::Random( -fPrecipiceRandY, fPrecipiceRandY ) * ( 1.0f - fabs2( (float)( g - fVertsHalfNum ) * fInvVertsHalfNum ) ) ) * 0.5f;
			vPos.x += -vNorm.x * ( fDepth - fDepthRand - fRandX ) - vNorm.y * fRandY;
			vPos.y += -vNorm.y * ( fDepth - fDepthRand - fRandX ) + vNorm.x * fRandY;
		}
		pNode->verts.push_back( vPos );
	}
	pNode->verts.back().x += vNorm.x * DEF_CRAG_HOLE_WIDTH * 4.0f;
	pNode->verts.back().y += vNorm.y * DEF_CRAG_HOLE_WIDTH * 4.0f;

	// create intermidiate vertices
	const float fCoeff = (float)( nVertsNum - 1 ) / ( pNode->fMaxHeight - pNode->fMinHeight );
	for ( list<int>::const_iterator itUsePrecID = pNode->precs.begin(); itUsePrecID != pNode->precs.end(); ++itUsePrecID )
	{
		const STerrainInfo::SPrecipice *pPrec = FindPrecipice( *itUsePrecID );
		if ( !pPrec )
			continue;
		for ( int i = 0; i < pPrec->nodes.size(); ++i )
		{
			if ( pPrec->nodes[i] == nNodeInd )
			{
				AddUniquePrecipiceVertex( pNode, pPrec->maxHeights[i] );
				AddUniquePrecipiceVertex( pNode, pPrec->minHeights[i] );
			}
		}
	}
}

void CTerraGen::CheckPrecipiceIntersectionWithRivers( STerrainInfo::SPrecipice *pPrec )
{
	for ( int i = 0; i < pPrec->nodes.size(); ++i )
	{
		if ( IsPointInsideRivers( CVec3( terrainInfo.precNodes[pPrec->nodes[i]].vPos.x, terrainInfo.precNodes[pPrec->nodes[i]].vPos.y, 0 ), pPrec->nExcludeRiverID ) )
			pPrec->visibles[i] = 0;
	}
}

void CTerraGen::CreatePrecipiceMesh( STerrainInfo::SPrecipice *pCurPrec, const bool bNeedRiversClamping )
{
	SPrecipiceGFXInfo gfxInfo;
	int nPrevVertsNum, nCurVertsNum, nPrevVertsOffset;
	STriangleEx trg;
	NGScene::SVertex vert;
	CalcCompactVector( &(vert.texU), V3_AXIS_X );
	CalcCompactVector( &(vert.texV), V3_AXIS_Y );

	int nFind1, nFind2;
	float fTexX;

	if ( bNeedRiversClamping )
		CheckPrecipiceIntersectionWithRivers( pCurPrec );

	gfxInfo.nID = pCurPrec->nID;
	gfxInfo.pMaterial = pCurPrec->pMaterial;

	vector<NGScene::SVertex> vertices;
	vertices.reserve( 256 );
	vertices.resize( 0 );

	vector<STriangleEx> triangles;
	triangles.reserve( 256 );
	triangles.resize( 0 );

	nPrevVertsNum = 0;
	nPrevVertsOffset = 0;

	const float fTexScaleX = pCurPrec->fTexGeomScale / pCurPrec->pMaterial->pTexture->nWidth;
	const float fTexScaleY = pCurPrec->fTexGeomScale / pCurPrec->pMaterial->pTexture->nHeight;

	fTexX = 0.0f;

	static vector<STerrainInfo::SVSOPoint> peakPoints(256);
	peakPoints.resize( 0 );
	STerrainInfo::SPeak peak;
	//peak.nID = ( pCurPrec->nExcludeRiverID >= 0 ) ? ( pCurPrec->nID | 0xf0000 ) : pCurPrec->nID;
	peak.nID = pCurPrec->nID;

	vector<int> nodeVertsNum( pCurPrec->nodes.size() );
	fill( nodeVertsNum.begin(), nodeVertsNum.end(), 0 );

	vector<int> nodeTrgsNum( pCurPrec->nodes.size() - 1 );
	fill( nodeTrgsNum.begin(), nodeTrgsNum.end(), 0 );

	int nPrevFind1 = -1, nPrevFind2 = -1;

	for ( int i = 0; i < pCurPrec->nodes.size(); ++i )
	{
		const int nVertsOffsetBeforeAdded = vertices.size();
		nCurVertsNum = 0;

		if ( pCurPrec->visibles[i] )
		{
			const STerrainInfo::SPrecipiceNode &node = terrainInfo.precNodes[pCurPrec->nodes[i]];

			//( ( i > 0 ) && ( i < ( pCurPrec->nodes.size() - 1 ) ) &&
			//if ( ( fabs( pCurPrec->maxHeights[i] - pCurPrec->minHeights[i] ) <= DEF_MIN_PRECIPICE_HEIGHT ) &&
			if ( (pCurPrec->maxHeights[i] <= (pCurPrec->minHeights[i] + DEF_MIN_PRECIPICE_HEIGHT)) &&
			//{
/*if (*/ //( ( i == 0 ) || ( fabs( pCurPrec->maxHeights[i - 1] - pCurPrec->minHeights[i - 1] ) <= DEF_MIN_PRECIPICE_HEIGHT ) ) &&
				 //( ( i == ( pCurPrec->nodes.size() - 1 ) ) || ( fabs( pCurPrec->maxHeights[i + 1] - pCurPrec->minHeights[i + 1] ) <= DEF_MIN_PRECIPICE_HEIGHT ) ) )
				 ((i == 0) || (pCurPrec->maxHeights[i - 1] <= (pCurPrec->minHeights[i - 1] + DEF_MIN_PRECIPICE_HEIGHT))) &&
				 ((i == (pCurPrec->nodes.size() - 1)) || (pCurPrec->maxHeights[i + 1] <= (pCurPrec->minHeights[i + 1] + DEF_MIN_PRECIPICE_HEIGHT))) )
				{
					if ( peakPoints.size() >= 2 )
						peak.points.push_back( peakPoints );
					peakPoints.resize( 0 );
					nPrevVertsNum = nCurVertsNum;
					nPrevVertsOffset = nVertsOffsetBeforeAdded;
					nodeVertsNum[i] = nCurVertsNum;
					nPrevFind1 = nPrevFind2 = -1;
					continue;
				}
			//}

			//if ( i > 0 )
			//{
			//	const STerrainInfo::SPrecipiceNode &prevNode = terrainInfo.precNodes[pCurPrec->nodes[i - 1]];
			//	const float fPrevDist = sqrt( fabs2( prevNode.vPos.x - node.vPos.x ) + fabs2( prevNode.vPos.y - node.vPos.y ) );
			//	fTexX += fPrevDist * fTexScaleX;
			//}

			const float fMinH = pCurPrec->minHeights[i];
			const float fMaxH = pCurPrec->maxHeights[i];

			NI_ASSERT( (fMinH >= node.fMinHeight) && (fMaxH <= node.fMaxHeight), "Error was occured during precipice collection" );
			//NI_ASSERT( fabs( node.fMaxHeight - node.fMinHeight ) > DEF_EPS, "Precipice node is singular" );
			NI_ASSERT( node.verts.size() >= 2, "Precipice node's vertices data is not ready" );

			// fill vertices
			CalcCompactVector( &(vert.normal), node.vNorm );
			vert.tex.Set( 0, 0 );

			nFind1 = 0;
			while ( node.verts[nFind1].z > (fMaxH + DEF_EPS) )
			{
				++nFind1;
				if ( nFind1 >= node.verts.size() )
				{
					nFind1 = node.verts.size() - 1;
					break;
				}
			}

			NI_ASSERT( (node.verts[nFind1].z - fMaxH) < DEF_EPS, "Node verts structure was not filled correctly" );

			nFind2 = nFind1;

			while ( node.verts[nFind2].z > (fMinH + DEF_EPS) )
			{
				++nFind2;
				if ( nFind2 >= node.verts.size() )
				{
					nFind2 = node.verts.size() - 1;
					break;
				}
			}

			NI_ASSERT( (node.verts[nFind2].z - fMinH) < DEF_EPS, "Node verts structure was not filled correctly" );

			/*if ( ( i > 0 ) && ( nPrevFind2 >= 0 ) )
			{
				const STerrainInfo::SPrecipiceNode &prevNode = terrainInfo.precNodes[pCurPrec->nodes[i - 1]];
				const float fPrevDist1 = fabs( prevNode.verts[nPrevFind1] - node.verts[nFind1] );
				const float fPrevDist2 = fabs( prevNode.verts[nPrevFind2] - node.verts[nFind2] );
				fTexX += max( fPrevDist1, fPrevDist2 ) * fTexScaleX;
			}*/

			nPrevFind1 = nFind1;
			nPrevFind2 = nFind2;

			const int nCurColVertsOffs = vertices.size();

			for ( int k = nFind1; k <= nFind2; ++k )
			{
				vert.pos = node.verts[k];
				vertices.push_back( vert );
				++nCurVertsNum;
			}

			///if ( (pCurPrec->bStayedOnTerrain) && (node.nCount == 1) && (nFind2 > nFind1) )
			if ( pCurPrec->bStayedOnTerrain )
			{
				CVec3 &vPos = vertices.back().pos;
#ifdef VERSION_DEV_M1
				NDb::SVSOPoint point;
				point.vPos = vPos;
				point.vNorm = node.vNorm;
				const float fBottomHeight = GetLowerHeight( point );
#else	// old code
				const float fBottomHeight = GetTerraHeight( vPos.x, vPos.y ) + GetMaxCragHeight( CVec2(vPos.x, vPos.y), pCurPrec->nID );
#endif

				///if ( (fBottomHeight < node.verts[nFind2 - 1].z) && (fabs(fBottomHeight - vPos.z) < DEF_STAYED_ON_TERRAIN_ERROR) )
				if ( fabs(fBottomHeight - vPos.z) < DEF_STAYED_ON_TERRAIN_ERROR )

					vPos.z = fBottomHeight;
			}

			if ( (nPrevVertsNum > 0) && (nCurVertsNum > 0) )
			{
				float fDist = 0.0f;
				const int nMaxVertsNum = max( nPrevVertsNum, nCurVertsNum );
				const float fCoeff1 = nPrevVertsNum > nCurVertsNum ? 1.0f : (float)nPrevVertsNum / nCurVertsNum;
				const float fCoeff2 = nCurVertsNum > nPrevVertsNum ? 1.0f : (float)nCurVertsNum / nPrevVertsNum;
				for ( int g = 0; g < nMaxVertsNum; ++g )
				{
					const int nInd1 = fCoeff1 * g;
					const int nInd2 = fCoeff2 * g;
					//fDist = max( fDist, fabs( vertices[nPrevVertsOffset + nInd1].pos - vertices[nVertsOffsetBeforeAdded + nInd2].pos ) );
					fDist = max( fDist, fabs(GetVec2(vertices[nPrevVertsOffset + nInd1].pos) - GetVec2(vertices[nVertsOffsetBeforeAdded + nInd2].pos)) );
				}
				fTexX += fDist * fTexScaleX;
			}

			float fCurPrecTexY = vertices.back().pos.z * fTexScaleY;
			for ( int k = nFind2; k >= nFind1; --k )
			{
				const int nCurVertInd = nCurColVertsOffs + k - nFind1;
				if ( k < nFind2 )
					fCurPrecTexY += fabs( vertices[nCurVertInd + 1].pos - vertices[nCurVertInd].pos ) * fTexScaleY;
				vertices[nCurVertInd].tex.x = fTexX;
				vertices[nCurVertInd].tex.y = fCurPrecTexY;
			}

			// fill triangles
			if ( (nPrevVertsNum > 0) && (nCurVertsNum > 0) && (pCurPrec->nodes[i] != pCurPrec->nodes[i - 1]) )
			{
				const int nMaxVertsNum = max( nPrevVertsNum, nCurVertsNum );
				const float fCoeff1 = nPrevVertsNum > nCurVertsNum ? 1.0f : (float)nPrevVertsNum / nCurVertsNum;
				const float fCoeff2 = nCurVertsNum > nPrevVertsNum ? 1.0f : (float)nCurVertsNum / nPrevVertsNum;
				for ( int g = 0; g < ( nMaxVertsNum - 1 ); ++g )
				{
					const int nLeftInd1 = fCoeff1 * g;
					const int nLeftInd2 = fCoeff1 * ( g + 1 );
					const int nRightInd1 = fCoeff2 * g;
					const int nRightInd2 = fCoeff2 * ( g + 1 );
					if ( nLeftInd1 != nLeftInd2 )
					{
						trg.i1 = nLeftInd1 + nPrevVertsOffset;
						trg.i2 = nLeftInd2 + nPrevVertsOffset;
						trg.i3 = nRightInd2 + nVertsOffsetBeforeAdded;
						triangles.push_back( trg );
						++nodeTrgsNum[i - 1];
					}
					if ( nRightInd1 != nRightInd2 )
					{
						trg.i1 = nRightInd2 + nVertsOffsetBeforeAdded;
						trg.i2 = nRightInd1 + nVertsOffsetBeforeAdded;
						trg.i3 = nLeftInd1 + nPrevVertsOffset;
						triangles.push_back( trg );
						++nodeTrgsNum[i - 1];
					}
				}
			}
			
			if ( (nPrevVertsNum == 0) || (nCurVertsNum == 0) || (pCurPrec->nodes[i] == pCurPrec->nodes[i - 1]) || (fMaxH <= (fMinH + DEF_MIN_PRECIPICE_HEIGHT)) )
			{
				if ( peakPoints.size() >= 2 )
					peak.points.push_back( peakPoints );
				peakPoints.resize( 0 );
			}

			/*if ( ( i > 0 ) && ( pCurPrec->nodes[i] == pCurPrec->nodes[i - 1] ) )// && ( pCurPrec->nExcludeRiverID >= 0 ) )
			{
				if ( peakPoints.size() >= 2 )
					peak.points.push_back( peakPoints );
				peakPoints.resize( 0 );
			}*/
			if ( nCurVertsNum > 0 )
			{
				vector<STerrainInfo::SVSOPoint>::iterator itPeak = peakPoints.insert( peakPoints.end(), STerrainInfo::SVSOPoint() );
				//itPeak->vPos.Set( node.vPos.x, node.vPos.y, node.verts[nFind1].z );
				itPeak->vPos = node.verts[nFind1];
				itPeak->vNorm = node.nCount > 0 ? ( node.vNorm / node.nCount ) : node.vNorm;
			}
			else
				peakPoints.resize( 0 );
			/*	//peakPoints.push_back( NDb::SVSOPoint() );
				//peakPoints.back().vPos = node.verts[nFind1];
				//peakPoints.back().vNorm = node.nCount > 0 ? ( node.vNorm / node.nCount ) : node.vNorm;
				vector<NDb::SVSOPoint>::iterator itPeak = peakPoints.insert( peakPoints.end(), NDb::SVSOPoint() );
				itPeak->vPos.Set( node.vPos.x, node.vPos.y, node.verts[nFind1].z );
				itPeak->vNorm = node.nCount > 0 ? ( node.vNorm / node.nCount ) : node.vNorm;
			}
			else
			//if ( pCurPrec->nExcludeRiverID >= 0 )
			{
				peakPoints.push_back( NDb::SVSOPoint() );
				peakPoints.back().vPos = node.verts[nFind1];
				peakPoints.back().vNorm = node.nCount > 0 ? ( node.vNorm / node.nCount ) : node.vNorm;
			}*/
		}
		else
		{
			if ( peakPoints.size() >= 2 )
				peak.points.push_back( peakPoints );
			peakPoints.resize( 0 );
			nPrevVertsNum = nCurVertsNum;
			nPrevVertsOffset = nVertsOffsetBeforeAdded;
			nodeVertsNum[i] = nCurVertsNum;
			nPrevFind1 = nPrevFind2 = -1;
			continue;
		}

		nPrevVertsNum = nCurVertsNum;
		nPrevVertsOffset = nVertsOffsetBeforeAdded;
		nodeVertsNum[i] = nCurVertsNum;
	}

	if ( peakPoints.size() >= 2 )
		peak.points.push_back( peakPoints );
	peakPoints.resize( 0 );

	if ( !peak.points.empty() )
	{
		peak.vBBMin.Set( FP_MAX_VALUE, FP_MAX_VALUE );
		peak.vBBMax.Set( -FP_MAX_VALUE, -FP_MAX_VALUE );
		for ( vector< vector<STerrainInfo::SVSOPoint> >::const_iterator itArr = peak.points.begin(); itArr != peak.points.end(); ++itArr )
		{
			for ( vector<STerrainInfo::SVSOPoint>::const_iterator it = itArr->begin(); it != itArr->end(); ++it )
			{
				const CVec2 vPos( it->vPos.x, it->vPos.y );
				peak.vBBMin.Minimize( vPos );
				peak.vBBMax.Maximize( vPos );
			}
		}
		if ( !(pCurPrec->bHasNotPeak) )
		{
			list<STerrainInfo::SPeak>::const_iterator itAddPeak = terrainInfo.peaks.insert( terrainInfo.peaks.end(), peak );
			AddPeak( *itAddPeak );
		}
		peak.points.resize( 0 );
	}

	// create smooth normals
	vector<CVec3> precipiceNorms( vertices.size() );
	vector<int> precipiceNormsCounts( vertices.size() );
	fill( precipiceNorms.begin(), precipiceNorms.end(), CVec3(0, 0, 0) );
	fill( precipiceNormsCounts.begin(), precipiceNormsCounts.end(), 0 );
	CVec3 vNorm;
	for ( vector<STriangleEx>::const_iterator itTrg = triangles.begin(); itTrg != triangles.end(); ++itTrg )
	{
		GetTrueNormal( &vNorm, vertices[itTrg->i1].pos, vertices[itTrg->i2].pos, vertices[itTrg->i3].pos );
		vNorm = -vNorm; // because using of anticlockwise orientation
		precipiceNorms[itTrg->i1] += vNorm;
		++precipiceNormsCounts[itTrg->i1];
		precipiceNorms[itTrg->i2] += vNorm;
		++precipiceNormsCounts[itTrg->i2];
		precipiceNorms[itTrg->i3] += vNorm;
		++precipiceNormsCounts[itTrg->i3];
	}
	for ( int k = 0; k < vertices.size(); ++k )
	{
		if ( precipiceNormsCounts[k] > 0 )
		{
			vNorm = precipiceNorms[k] / precipiceNormsCounts[k];
			if ( fabs2(vNorm) > DEF_EPS )
			{
				if ( fabs(fabs2(vNorm) - 1.0f) > DEF_EPS )
					Normalize( &vNorm );
				CalcCompactVector( &(vertices[k].normal), vNorm );
				UpdateAlphaByPosition( &(vertices[k]) );
			}
		}
	}

	// break data for patches
	const int nPatchesNum = ( pCurPrec->nodes.size() % DEF_PRECIPICE_SAMPLES_PER_PATCH ) ?
													( pCurPrec->nodes.size() / DEF_PRECIPICE_SAMPLES_PER_PATCH + 1 ) :
													( pCurPrec->nodes.size() / DEF_PRECIPICE_SAMPLES_PER_PATCH );
	gfxInfo.patches.resize( nPatchesNum );
	for ( int i = 0; i < nPatchesNum; ++i )
	{
		gfxInfo.patches[i].vertices.reserve( 512 );
		gfxInfo.patches[i].vertices.resize( 0 );
		gfxInfo.patches[i].triangles.reserve( 512 );
		gfxInfo.patches[i].triangles.resize( 0 );
	}

	int nCurPatch = 0, nPatchInd = 0;
	int nVertsOffs = 0, nTrgsOffs = 0, nLastOffs;
	for ( int i = 0; i < nPatchesNum; ++i )
	{
		const int nFirstNode = i * DEF_PRECIPICE_SAMPLES_PER_PATCH;
		const int nLastNode = min( ( i + 1 ) * DEF_PRECIPICE_SAMPLES_PER_PATCH, pCurPrec->nodes.size() - 1 );
		if ( i > 0 )
			nVertsOffs -= nodeVertsNum[nFirstNode];
		nLastOffs = nVertsOffs;
		for ( int g = nFirstNode; g <= nLastNode; ++g )
		{
			for ( int k = 0; k < nodeVertsNum[g]; ++k )
				gfxInfo.patches[i].vertices.push_back( vertices[nLastOffs++] );
		}
		for ( int g = nFirstNode; g < nLastNode; ++g )
		{
			for ( int k = 0; k < nodeTrgsNum[g]; ++k )
			{
				const STriangleEx &trg = triangles[nTrgsOffs++];
				NI_ASSERT( (trg.i1 >= nVertsOffs) && (trg.i2 >= nVertsOffs) && (trg.i3 >= nVertsOffs), "Wrong triangle" );
				gfxInfo.patches[i].triangles.push_back( STriangle( trg.i1 - nVertsOffs, trg.i2 - nVertsOffs, trg.i3 - nVertsOffs ) );
			}
		}
		nVertsOffs = nLastOffs;

		RemoveInvisibleTriangles( &gfxInfo.patches[i] );
	}

	terrainGfxInfo.precipices.push_back( gfxInfo );
	if ( pGfxObserver )
		pGfxObserver->AddPrecipice( &gfxInfo );
}

void CTerraGen::RemovePrecipiceFromCollector( const int nID, const bool bFast )
{
	static vector<BYTE> nodesHash( 2048 );
	nodesHash.resize( terrainInfo.precNodes.size() );
	fill( nodesHash.begin(), nodesHash.end(), 0 );

	for ( list<STerrainInfo::SPrecipice>::iterator itPrec = terrainInfo.precipices.begin(); itPrec != terrainInfo.precipices.end(); ++itPrec )
	{
		if ( itPrec->nID == nID )
		{
			for ( int i = 0; i < itPrec->nodes.size(); ++i )
			{
				const int nNodeInd = itPrec->nodes[i];
				if ( !(nodesHash[nNodeInd]) )
				{
					STerrainInfo::SPrecipiceNode &node = terrainInfo.precNodes[nNodeInd];
					node.vNorm -= itPrec->norms[i];
					node.fDepth -= itPrec->fDepth;
					node.fDepthRand -= itPrec->fDepthRand;
					node.fRandX -= itPrec->fRandX;
					node.fRandY -= itPrec->fRandY;
					--node.nCount;
					if ( node.nCount <= 0 )
					{
						node.nCount = 0;
						node.fMinHeight = FP_MAX_VALUE;
						node.fMaxHeight = -FP_MAX_VALUE;
					}
					node.precs.remove( nID );
					nodesHash[nNodeInd] = 1;
				}
			}
			terrainInfo.precipices.erase( itPrec );
			break;
		}
	}

	// remove gfx info
	for ( list<SPrecipiceGFXInfo>::iterator it = terrainGfxInfo.precipices.begin(); it != terrainGfxInfo.precipices.end(); ++it )
	{
		if ( it->nID == nID )
		{
			terrainGfxInfo.precipices.erase( it );
			break;
		}
	}

	RemovePeakInfo( nID );
	RemovePeakGfxInfo( nID );

	if ( pGfxObserver )
	{
		pGfxObserver->RemovePrecipice( nID );
		pGfxObserver->RemovePeak( nID );
	}

	if ( !bFast )
	{
		// remove from intersectors
		for ( list<STerrainInfo::SPrecipice>::iterator itPrec = terrainInfo.precipices.begin(); itPrec != terrainInfo.precipices.end(); ++itPrec )
		{
			vector<int>::iterator itFind = find( itPrec->intersectors.begin(), itPrec->intersectors.end(), nID );
			if ( itFind != itPrec->intersectors.end() )
				itPrec->intersectors.erase( itFind );
		}
	}
}


