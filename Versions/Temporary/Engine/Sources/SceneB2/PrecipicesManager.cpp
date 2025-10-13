#include "stdafx.h"

#include "GenTerrain.h"

#include <algorithm>
#include <cstdint>

int CTerraGen::PrecAddUniqueNode( const CVec2 &vPos, const CVec3 &vNorm,
															const float fMin, const float fMax,
															const int nID,
															const float fDepth, const float fDepthRand,
															const float fRandX, const float fRandY )
{
	int nCount = 0;
	for ( std::vector<STerrainInfo::SPrecipiceNode>::iterator it = terrainInfo.precNodes.begin(); it != terrainInfo.precNodes.end(); ++it, ++nCount )
	{
		if ( fabs2( vPos - it->vPos ) < DEF_EPS )
		{
			it->vNorm += vNorm;
			it->fDepth += fDepth;
			it->fDepthRand += fDepthRand;
			it->fRandX += fRandX;
			it->fRandY += fRandY;
			it->fMinHeight = min( it->fMinHeight, fMin );
			it->fMaxHeight = max( it->fMaxHeight, fMax );
			++it->nCount;
			AddUnique( &(it->precs), nID );
			it->verts.resize( 0 );
			AddUnique( &updatedPrecNodes, nCount );
			return nCount;
		}
	}

	STerrainInfo::SPrecipiceNode node;
	node.vPos = vPos;
	node.vNorm = vNorm;
	node.fDepth = fDepth;
	node.fDepthRand = fDepthRand;
	node.fRandX = fRandX;
	node.fRandY = fRandY;
	node.fMinHeight = fMin;
	node.fMaxHeight = fMax;
	node.nCount = 1;
	node.precs.push_back( nID );
	node.verts.resize( 0 );
	terrainInfo.precNodes.push_back( node );
	AddUnique( &updatedPrecNodes, static_cast<int>(terrainInfo.precNodes.size() - 1) );
	return ( terrainInfo.precNodes.size() - 1 );
}

int CTerraGen::PrecAddUniqueNode2( const CVec2 &vPos, const CVec3 &vNorm1, const CVec3 &vNorm2, const float fMin, const float fMax,
																	 const int nID1, const int nID2, const float fDepth1, const float fDepth2,
																	 const float fDepthRand1, const float fDepthRand2, const float fRandX1, const float fRandX2,
																	 const float fRandY1, const float fRandY2 )
{
	int nCount = 0;
	for ( std::vector<STerrainInfo::SPrecipiceNode>::iterator it = terrainInfo.precNodes.begin(); it != terrainInfo.precNodes.end(); ++it, ++nCount )
	{
		if ( fabs2( vPos - it->vPos ) < DEF_EPS )
		{
			it->vNorm += vNorm1 + vNorm2;
			it->fDepth += fDepth1 + fDepth2;
			it->fDepthRand += fDepthRand1 + fDepthRand2;
			it->fRandX += fRandX1 + fRandX2;
			it->fRandY += fRandY1 + fRandY2;
			it->fMinHeight = min( it->fMinHeight, fMin );
			it->fMaxHeight = max( it->fMaxHeight, fMax );
			it->nCount += 2;
			AddUnique( &(it->precs), nID1 );
			AddUnique( &(it->precs), nID2 );
			it->verts.resize( 0 );
			AddUnique( &updatedPrecNodes, nCount );
			return nCount;
		}
	}

	STerrainInfo::SPrecipiceNode node;
	node.vPos = vPos;
	node.vNorm = vNorm1 + vNorm2;
	node.fDepth = fDepth1 + fDepth2;
	node.fDepthRand = fDepthRand1 + fDepthRand2;
	node.fRandX = fRandX1 + fRandX2;
	node.fRandY = fRandY1 + fRandY2;
	node.fMinHeight = fMin;
	node.fMaxHeight = fMax;
	node.nCount = 2;
	node.precs.push_back( nID1 );
	node.precs.push_back( nID2 );
	node.verts.resize( 0 );
	terrainInfo.precNodes.push_back( node );
	AddUnique( &updatedPrecNodes, static_cast<int>(terrainInfo.precNodes.size() - 1) );
	return ( terrainInfo.precNodes.size() - 1 );
}

struct SPrecipiceIntersect
{
	float fDist;
	int nNode;
	CVec3 vNorm;
	float fMinHeight;
	float fMaxHeight;
	//
	bool operator < ( const SPrecipiceIntersect &v ) const { return fDist < v.fDist; }
};

inline void SwapPrecipices( STerrainInfo::SPrecipice &prec1, STerrainInfo::SPrecipice &prec2 )
{
	prec1.nodes.swap( prec2.nodes );
	prec1.norms.swap( prec2.norms );
	prec1.minHeights.swap( prec2.minHeights );
	prec1.maxHeights.swap( prec2.maxHeights );
	prec1.visibles.swap( prec2.visibles );
}

inline void AddToPrecipice( STerrainInfo::SPrecipice &prec, const int nNodeInd, const CVec3 &vNorm,
														const float fMinHeight, const float fMaxHeight )
{
	prec.nodes.push_back( nNodeInd );
	prec.norms.push_back( vNorm );
	prec.minHeights.push_back( fMinHeight );
	prec.maxHeights.push_back( fMaxHeight );
	prec.visibles.push_back( 1 );
}

inline void AddToPrecipice( STerrainInfo::SPrecipice &dstPrec, const STerrainInfo::SPrecipice &srcPrec, const int nInd )
{
	dstPrec.nodes.push_back( srcPrec.nodes[nInd] );
	dstPrec.norms.push_back( srcPrec.norms[nInd] );
	dstPrec.minHeights.push_back( srcPrec.minHeights[nInd] );
	dstPrec.maxHeights.push_back( srcPrec.maxHeights[nInd] );
	dstPrec.visibles.push_back( srcPrec.visibles[nInd] );
}

inline void InitPrecipiceFromPrecipice( STerrainInfo::SPrecipice &dstPrec, const STerrainInfo::SPrecipice &srcPrec )
{
	dstPrec.vMin = srcPrec.vMin;
	dstPrec.vMax = srcPrec.vMax;
	dstPrec.nodes.reserve( srcPrec.nodes.size() );
	dstPrec.nodes.resize( 0 );
	dstPrec.norms.reserve( srcPrec.norms.size() );
	dstPrec.norms.resize( 0 );
	dstPrec.minHeights.reserve( srcPrec.minHeights.size() );
	dstPrec.minHeights.resize( 0 );
	dstPrec.maxHeights.reserve( srcPrec.maxHeights.size() );
	dstPrec.maxHeights.resize( 0 );
	dstPrec.visibles.reserve( srcPrec.visibles.size() );
	dstPrec.visibles.resize( 0 );
}

STerrainInfo::SPrecipice* CTerraGen::FindPrecipice( const int nID )
{
	for ( std::list<STerrainInfo::SPrecipice>::iterator it = terrainInfo.precipices.begin(); it != terrainInfo.precipices.end(); ++it )
	{
		if ( it->nID == nID )
			return &(*it);
	}
	//NI_ASSERT( false, StrFmt( "Can't find precipice with ID=%d", nID ) );
	return 0;
}

void CTerraGen::AddPrecipiceToCollector( const int nID, const std::vector<CVec3> &posArr, const std::vector<float> &heightsArr,
																				 const std::vector<CVec3> &normsArr, const NDb::SMaterial *pMaterial, const float fTexGeomScale,
																				 const uint8_t bStayedOnTerrain, const int nExcludeID, const NDb::SMaterial *pFootMaterial,
																				 const float fDepth, const float fDepthRand, const float fRandX, const float fRandY,
																				 const bool bHasPeak )
{
	NI_ASSERT( (posArr.size() == heightsArr.size()) && (posArr.size() == normsArr.size()), "Wrong parameters" );

	if ( posArr.size() < 2 )
		return;

	const int nMemReserve = posArr.size() + ( posArr.size() >> 1 );

	STerrainInfo::SPrecipice addPrec;
	addPrec.nID = nID;
	addPrec.nExcludeRiverID = nExcludeID;
	addPrec.nodes.reserve( nMemReserve );
	addPrec.nodes.resize( 0 );
	addPrec.minHeights.reserve( nMemReserve );
	addPrec.minHeights.resize( 0 );
	addPrec.maxHeights.reserve( nMemReserve );
	addPrec.maxHeights.resize( 0 );
	addPrec.vMin.Set( FP_MAX_VALUE, FP_MAX_VALUE );
	addPrec.vMax.Set( -FP_MAX_VALUE, -FP_MAX_VALUE );
	addPrec.norms.reserve( nMemReserve );
	addPrec.norms = normsArr;
	addPrec.visibles.reserve( nMemReserve );
	addPrec.visibles.resize( 0 );
	addPrec.intersectors.reserve( 16 );
	addPrec.intersectors.resize( 0 );
	addPrec.pMaterial = pMaterial;
	addPrec.pFootMaterial = pFootMaterial;
	addPrec.fTexGeomScale = fTexGeomScale;
	addPrec.bStayedOnTerrain = bStayedOnTerrain;
	addPrec.fDepth = fDepth;
	addPrec.fDepthRand = fDepthRand;
	addPrec.fRandX = fRandX;
	addPrec.fRandY = fRandY;
	addPrec.bHasNotPeak = !bHasPeak;

	for ( int i = 0; i < posArr.size(); ++i )
	{
		addPrec.vMin.Minimize( CVec2(posArr[i].x, posArr[i].y) );
		addPrec.vMax.Maximize( CVec2(posArr[i].x, posArr[i].y) );
		const float fMinH = min( posArr[i].z, heightsArr[i] );
		const float fMaxH = max( posArr[i].z, heightsArr[i] );
		const int nNodeInd = PrecAddUniqueNode( CVec2(posArr[i].x, posArr[i].y), normsArr[i], fMinH, fMaxH, addPrec.nID,
			fDepth, fDepthRand, fRandX, fRandY );
		AddToPrecipice( addPrec, nNodeInd, normsArr[i], fMinH, fMaxH );
	}

	STerrainInfo::SPrecipice oldPrecipice, resPrecipice;
	float fDist;
	std::vector<SPrecipiceIntersect> precInters( 8 );

	for ( std::list<STerrainInfo::SPrecipice>::iterator itPrecipice = terrainInfo.precipices.begin(); itPrecipice != terrainInfo.precipices.end(); ++itPrecipice )
	{
		if ( IsBBIntersect(itPrecipice->vMin, itPrecipice->vMax, addPrec.vMin, addPrec.vMax) )
		{
			InitPrecipiceFromPrecipice( oldPrecipice, *itPrecipice );

			for ( int i = 1; i < itPrecipice->nodes.size(); ++i )
			{
				precInters.resize( 0 );

				if ( itPrecipice->nodes[i - 1] != itPrecipice->nodes[i] )
				{
					const STerrainInfo::SPrecipiceNode &node1 = terrainInfo.precNodes[itPrecipice->nodes[i - 1]];
					const STerrainInfo::SPrecipiceNode &node2 = terrainInfo.precNodes[itPrecipice->nodes[i]];

					InitPrecipiceFromPrecipice( resPrecipice, addPrec );

					for ( int g = 1; g < addPrec.nodes.size(); ++g )
					{
						AddToPrecipice( resPrecipice, addPrec, g - 1 );

						const STerrainInfo::SPrecipiceNode &addNode1 = terrainInfo.precNodes[addPrec.nodes[g - 1]];
						const STerrainInfo::SPrecipiceNode &addNode2 = terrainInfo.precNodes[addPrec.nodes[g]];

						fDist = ( node2.vPos.x - node1.vPos.x ) * ( addNode2.vPos.y - addNode1.vPos.y ) -
										( node2.vPos.y - node1.vPos.y ) * ( addNode2.vPos.x - addNode1.vPos.x );
						if ( fabs( fDist ) > DEF_EPS )
						{
							fDist = 1.0f / fDist;
							const float t = ( (addNode1.vPos.x - node1.vPos.x ) * (addNode2.vPos.y - addNode1.vPos.y) -
																(addNode1.vPos.y - node1.vPos.y ) * (addNode2.vPos.x - addNode1.vPos.x) ) * fDist;
							const float k = ( (node1.vPos.y - addNode1.vPos.y ) * (node2.vPos.x - node1.vPos.x) -
																(node1.vPos.x - addNode1.vPos.x ) * (node2.vPos.y - node1.vPos.y) ) * fDist;
							if ( (t >= 0.0f) && (t <= 1.0f) && (k >= 0.0f) && (k <= 1.0f) )
							{
								const CVec2 vPos( node1.vPos.x + (node2.vPos.x - node1.vPos.x) * t,
																	node1.vPos.y + (node2.vPos.y - node1.vPos.y) * t );
								const CVec3 vNormFromNode1 = node1.vNorm / node1.nCount;
								const CVec3 vNormFromNode2 = node2.vNorm / node2.nCount;
								const CVec3 vNorm1 = vNormFromNode1 + ( vNormFromNode2 - vNormFromNode1 ) * t;
								const CVec3 vNorm2 = vNormFromNode1 + ( vNormFromNode2 - vNormFromNode1 ) * k;
								const float fTerraHeight = GetTerraHeight( vPos.x, vPos.y );

								//const float fNH1 = itPrecipice->nExcludeRiverID < 0 ?
								//	( GetTerraHeight( vPos.x, vPos.y ) +
								//	addPrec.nExcludeRiverID < 0 ? GetMaxCragHeight2( vPos, nID, itPrecipice->nID ) : GetMaxCragHeight( vPos, itPrecipice->nID ) ) :
								//	( itPrecipice->minHeights[i - 1] + ( itPrecipice->minHeights[i] - itPrecipice->minHeights[i - 1] ) * t );
								float fNH1;
								if ( itPrecipice->nExcludeRiverID < 0 )
									fNH1 = fTerraHeight + ( addPrec.nExcludeRiverID < 0 ? GetMaxCragHeight2(vPos, nID, itPrecipice->nID) : GetMaxCragHeight(vPos, itPrecipice->nID) );
								else
									fNH1 = itPrecipice->minHeights[i - 1] + ( itPrecipice->minHeights[i] - itPrecipice->minHeights[i - 1] ) * t;

								const float fXH1 = itPrecipice->maxHeights[i - 1] + ( itPrecipice->maxHeights[i] - itPrecipice->maxHeights[i - 1] ) * t;
								if ( fXH1 < fNH1 )
									continue;
								const float fMinH1 = min( fNH1, fXH1 );
								const float fMaxH1 = max( fNH1, fXH1 );
								//const float fNH2 = addPrec.nExcludeRiverID < 0 ?
								//	( GetTerraHeight( vPos.x, vPos.y ) +
								//	itPrecipice->nExcludeRiverID < 0 ? GetMaxCragHeight2( vPos, nID, itPrecipice->nID ) : GetMaxCragHeight( vPos, nID ) ) :
								//	( addPrec.minHeights[g - 1] + ( addPrec.minHeights[g] - addPrec.minHeights[g - 1] ) * k );4
								float fNH2;
								if ( addPrec.nExcludeRiverID < 0 )
									fNH2 = fTerraHeight + ( itPrecipice->nExcludeRiverID < 0 ? GetMaxCragHeight2(vPos, nID, itPrecipice->nID) : GetMaxCragHeight(vPos, nID) );
								else
									fNH2 = addPrec.minHeights[g - 1] + ( addPrec.minHeights[g] - addPrec.minHeights[g - 1] ) * k;

								const float fXH2 = addPrec.maxHeights[g - 1] + ( addPrec.maxHeights[g] - addPrec.maxHeights[g - 1] ) * k;
								if ( fXH2 < fNH2 )
									continue;
								const float fMinH2 = min( fNH2, fXH2 );
								const float fMaxH2 = max( fNH2, fXH2 );
/*!!!!!!!!*/		const int nNodeInd = PrecAddUniqueNode2( vPos, vNorm2, vNorm1/*vNorm1, vNorm2*/, min(fMinH1, fMinH2), max(fMaxH1, fMaxH2),
																												 addPrec.nID, itPrecipice->nID, addPrec.fDepth, itPrecipice->fDepth, addPrec.fDepthRand, itPrecipice->fDepthRand,
																												 addPrec.fRandX, itPrecipice->fRandX, addPrec.fRandY, itPrecipice->fRandY );

								SPrecipiceIntersect curInters;
								curInters.nNode = nNodeInd;
								curInters.vNorm = vNorm1;

								AddUnique( &(itPrecipice->intersectors), addPrec.nID );
								AddUnique( &(addPrec.intersectors), itPrecipice->nID );

								if ( (itPrecipice->nodes[i - 1] != nNodeInd) && (itPrecipice->nodes[i] != nNodeInd) )
								{
									if ( itPrecipice->nExcludeRiverID < 0 ) // crag
									{
										if ( addPrec.nExcludeRiverID < 0 )
										{
											if ( itPrecipice->minHeights[i - 1] <= itPrecipice->minHeights[i] )
											{
												curInters.fDist = t - DEF_EPS;
												curInters.fMinHeight = fMinH1;
												curInters.fMaxHeight = fMaxH1;
												precInters.push_back( curInters );
												//
												curInters.fDist = t;
												curInters.fMinHeight = min( fMaxH1, fMaxH2 );
												curInters.fMaxHeight = max( fMaxH1, fMaxH2 );
												precInters.push_back( curInters );
											}
											else
											{
												curInters.fDist = t - DEF_EPS;
												curInters.fMinHeight = min( fMaxH1, fMaxH2 );
												curInters.fMaxHeight = max( fMaxH1, fMaxH2 );
												precInters.push_back( curInters );
												//
												curInters.fDist = t;
												curInters.fMinHeight = fMinH1;
												curInters.fMaxHeight = fMaxH1;
												precInters.push_back( curInters );
											}
										}
										else
										{
											curInters.fDist = t;
											curInters.fMinHeight = fMinH1;
											curInters.fMaxHeight = fMaxH1;
											precInters.push_back( curInters );
										}
									}
									else // river
									{
										if ( itPrecipice->maxHeights[i - 1] <= itPrecipice->maxHeights[i] )
										{
											if ( addPrec.nExcludeRiverID < 0 )
											{
												curInters.fDist = t - DEF_EPS;
												curInters.fMinHeight = fMinH1;
												curInters.fMaxHeight = /*fMaxH1*/fMinH2;
												precInters.push_back( curInters );
											}
											//
											curInters.fDist = t;
											curInters.fMinHeight = min( fMinH1, fMinH2 );
											curInters.fMaxHeight = max( fMaxH1, fMaxH2 );
											precInters.push_back( curInters );
										}
										else
										{
											curInters.fDist = t - DEF_EPS;
											curInters.fMinHeight = min( fMinH1, fMinH2 );
											curInters.fMaxHeight = max( fMaxH1, fMaxH2 );
											precInters.push_back( curInters );
											//
											if ( addPrec.nExcludeRiverID < 0 )
											{
												curInters.fDist = t;
												curInters.fMinHeight = fMinH1;
												curInters.fMaxHeight = /*fMaxH1*/fMinH2;
												precInters.push_back( curInters );
											}
										}
									}
								}

								if ( (addPrec.nodes[g - 1] != nNodeInd) && (addPrec.nodes[g] != nNodeInd) )
								{
									if ( addPrec.nExcludeRiverID < 0 ) // crag
									{
										if ( itPrecipice->nExcludeRiverID < 0 )
										{
											if ( addPrec.minHeights[g - 1] <= addPrec.minHeights[g] )
											{
												AddToPrecipice( resPrecipice, nNodeInd, vNorm2, fMinH2, fMaxH2 );
												AddToPrecipice( resPrecipice, nNodeInd, vNorm2, min(fMaxH1, fMaxH2), max(fMaxH1, fMaxH2) );
											}
											else
											{
												AddToPrecipice( resPrecipice, nNodeInd, vNorm2, min(fMaxH1, fMaxH2), max(fMaxH1, fMaxH2) );
												AddToPrecipice( resPrecipice, nNodeInd, vNorm2, fMinH2, fMaxH2 );
											}
										}
										else
										{
											AddToPrecipice( resPrecipice, nNodeInd, vNorm2, fMinH2, fMaxH2 );
										}
									}
									else // river
									{
										if ( addPrec.maxHeights[g - 1] <= addPrec.maxHeights[g] )
										{
											if ( itPrecipice->nExcludeRiverID < 0 )
												AddToPrecipice( resPrecipice, nNodeInd, vNorm2, fMinH2, /*fMaxH2*/fMinH1 );
											AddToPrecipice( resPrecipice, nNodeInd, vNorm2, min(fMinH1, fMinH2), max(fMaxH1, fMaxH2) );
										}
										else
										{
											AddToPrecipice( resPrecipice, nNodeInd, vNorm2, min(fMinH1, fMinH2), max(fMaxH1, fMaxH2) );
											if ( itPrecipice->nExcludeRiverID < 0 )
												AddToPrecipice( resPrecipice, nNodeInd, vNorm2, fMinH2, /*fMaxH2*/fMinH1 );
										}
									}
								}
							}
						}
					}
					AddToPrecipice( resPrecipice, addPrec, addPrec.nodes.size() - 1 );
					SwapPrecipices( addPrec, resPrecipice );
				}

				AddToPrecipice( oldPrecipice, *itPrecipice, i - 1 );

				std::sort( precInters.begin(), precInters.end() );
				for ( std::vector<SPrecipiceIntersect>::const_iterator it = precInters.begin(); it != precInters.end(); ++it )
					AddToPrecipice( oldPrecipice, it->nNode, it->vNorm, it->fMinHeight, it->fMaxHeight );
			}

			AddToPrecipice( oldPrecipice, *itPrecipice, itPrecipice->nodes.size() - 1 );
			SwapPrecipices( *itPrecipice, oldPrecipice );
		}
	}
	terrainInfo.precipices.push_back( addPrec );
}

void CTerraGen::UpdateAllNeededPrecipices()
{
	// add intersectors to update queue
	static std::vector<int> resUpdated( 128 );
	resUpdated = updatedPrecipices;
	for ( std::vector<int>::const_iterator it = updatedPrecipices.begin(); it != updatedPrecipices.end(); ++it )
	{
		const STerrainInfo::SPrecipice *pPrec = FindPrecipice( *it );
		if ( pPrec )
		{
			for ( std::vector<int>::const_iterator itInters = pPrec->intersectors.begin(); itInters != pPrec->intersectors.end(); ++itInters )
			{
				if ( *itInters < 0x20000 )
					AddUnique( &resUpdated, *itInters );
				else
					AddUnique( &resUpdated, (*itInters & 0xffff) | 0x10000 ); // add only one bank of river
			}
		}
	}
	updatedPrecipices.swap( resUpdated );

	// remove all updated precipices
	for ( std::vector<int>::const_iterator it = updatedPrecipices.begin(); it != updatedPrecipices.end(); ++it )
	{
		RemovePrecipiceFromCollector( *it, true );
		if ( *it > 0xffff )
			RemovePrecipiceFromCollector( 0x20000 | ((*it) & 0xffff), true );
	}

	// add all updated precipices
	for ( std::vector<int>::const_iterator it = updatedPrecipices.begin(); it != updatedPrecipices.end(); ++it )
	{
		// crag
		if ( *it < 0x10000 )
		{
			const STerrainInfo::SCrag *pCrag = FindCragInfo( *it );
			if ( pCrag )
				AddPrecipiceToCollector(	pCrag->nID, pCrag->precVerts, pCrag->precHeights, pCrag->precNorms,
																	pCrag->pDesc->pRidgeMaterial, pCrag->pDesc->fRidgeTexGeomScale,
																	true, -1, pCrag->pDesc->pFootMaterial,
																	pCrag->pDesc->fDepth, pCrag->pDesc->fDepthRand, pCrag->pDesc->fRandX, pCrag->pDesc->fRandY,
																	pCrag->pDesc->bHasPeak );
		}
		else // river
		{
			if ( *it < 0x20000 ) // only for one coast for double-excluding
			{
				const STerrainInfo::SRiver *pRiver = FindRiverInfo( (*it) & 0xffff );
				if ( pRiver )
				{
					AddPrecipiceToCollector(	pRiver->nID | 0x10000,
																		pRiver->precVertsL, pRiver->precHeightsL, pRiver->precNormsL,
																		pRiver->pDesc->pPrecipiceMaterial, pRiver->pDesc->fRidgeTexGeomScale,
																		false, pRiver->nID, 0,
																		pRiver->pDesc->fDepth, pRiver->pDesc->fDepthRand, pRiver->pDesc->fRandX, pRiver->pDesc->fRandY,
																		pRiver->pDesc->bHasPeak );

					AddPrecipiceToCollector(	pRiver->nID | 0x20000,
																		pRiver->precVertsR, pRiver->precHeightsR, pRiver->precNormsR,
																		pRiver->pDesc->pPrecipiceMaterial, pRiver->pDesc->fRidgeTexGeomScale,
																		false, pRiver->nID, 0,
																		pRiver->pDesc->fDepth, pRiver->pDesc->fDepthRand, pRiver->pDesc->fRandX, pRiver->pDesc->fRandY,
																		pRiver->pDesc->bHasPeak );
				}
			}
		}
	}

	// create all vertices in nodes
	for ( std::vector<int>::const_iterator it = updatedPrecNodes.begin(); it != updatedPrecNodes.end(); ++it )
		CreateVerticesInPrecipiceNode( &(terrainInfo.precNodes[*it]), *it );
	updatedPrecNodes.resize( 0 );

	// create all meshes
	STerrainInfo::SPrecipice *pPrec;
	for ( std::vector<int>::const_iterator it = updatedPrecipices.begin(); it != updatedPrecipices.end(); ++it )
	{
		if ( pPrec = FindPrecipice(*it) )
			CreatePrecipiceMesh( pPrec, true );
		if ( *it > 0xffff )
		{
			if ( pPrec = FindPrecipice( 0x20000 | ((*it) & 0xffff)) ) 
				CreatePrecipiceMesh( pPrec, true );
		}
	}
	updatedPrecipices.resize( 0 );
}

void CTerraGen::RemapPrecipices()
{
	std::vector<int> precMap( terrainInfo.precNodes.size() );
	fill( precMap.begin(), precMap.end(), -1 );
	std::vector<STerrainInfo::SPrecipiceNode> newNodes;
	newNodes.reserve( terrainInfo.precNodes.size() );
	newNodes.resize( 0 );
	int nInd = 0;
	for ( std::vector<STerrainInfo::SPrecipiceNode>::const_iterator it = terrainInfo.precNodes.begin(); it != terrainInfo.precNodes.end(); ++it, ++nInd )
	{
		if ( it->nCount > 0 )
		{
			newNodes.push_back( *it );
			precMap[nInd] = newNodes.size() - 1;
		}
	}
	terrainInfo.precNodes = newNodes;
	for ( std::list<STerrainInfo::SPrecipice>::iterator itPrec = terrainInfo.precipices.begin(); itPrec != terrainInfo.precipices.end(); ++itPrec )
	{
		for ( std::vector<int>::iterator it = itPrec->nodes.begin(); it != itPrec->nodes.end(); ++it )
		{
			*it = precMap[*it];
			if ( *it < 0 )
			{
				NI_ASSERT( *it >= 0, "Wrong node is used in precipice" );
				*it = 0;
			}
		}
	}
}

void CTerraGen::AddToPrecipiceUpdateQueue( const int nID )
{
	AddUnique( &updatedPrecipices, nID );
}



