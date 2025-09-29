#include "StdAfx.h"
//
//#include "../Stats_B2_M1/Vis2AI.h"
//
//#include "TerraAIObserver.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void CTerraAIObserver::AddVSO( const NDb::SVSOInstance *pInstance )
//{
//	list<SVector> tiles;
//	for ( int j = 0; j < pInstance->points.size() - 1; ++j )
//	{
//		list<SVector> temp;
//		GetTilesUnderVSO( pInstance, j, 1.0f, &temp, SSingleSide() );
//		tiles.splice( tiles.end(), temp );
//	}
//	const NDb::STerrainAIProperties prop = pInstance->pDescriptor->aIProperty;
//	///pTerrain->AddTiles( tiles, (EAIClasses)prop.nAIPassabilityClass, prop.fPassability, prop.nSoilType, prop.bCanEntrench );
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void CTerraAIObserver::AddCrag( const NDb::SVSOInstance *pInstance )
//{
//	NDb::SVSOInstance *pCrag = 0;
//	(*pCrag) = (*pInstance);
//	for ( int i = 0; i < pCrag->points.size(); ++i )
//		pCrag->points[i].fWidth = AI_TILE_SIZE * 2.0f;
//
//	AddVSO( pCrag );
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void CTerraAIObserver::AddWaterLine( const NDb::SVSOInstance *pInstance )
//{
//	list<SVector> coastTiles;
//	list<SVector> waterTiles;
//	waterTiles.clear();
//
//	for ( int j = 0; j < pInstance->points.size() - 1; ++j )
//	{
//		list<SVector> temp;
//		GetTilesUnderVSO( pInstance, j, 1.0f, &temp, SSingleSide(true, -64, 288) );
//		coastTiles.splice( coastTiles.end(), temp );
//	}
//
//	pTerrain->AddMarineTiles( coastTiles, SVectorStripeObject::ESP_DUST | SVectorStripeObject::ESP_TRACE,
//		waterTiles, SVectorStripeObject::ESP_SPLASH );
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void CTerraAIObserver::GetTilesUnderVSO( const NDb::SVSOInstance *pVSO, const int j, const float fCoeff,
//																				 list<SVector> *pTiles, const SSingleSide &singleSide, bool bInverse )
//{
//	const float fTileSize = (float)(pAIMap->GetTileSize());
//	NDb::SVSOInstance revVSO = (*pVSO);
//
//	const CVec2 vCenter1( revVSO.points[j].vPos.x, revVSO.points[j].vPos.y );
//	CVec2 vCenter2;
//	if ( j + 1 < revVSO.points.size() )
//		vCenter2.Set( revVSO.points[j + 1].vPos.x, revVSO.points[j + 1].vPos.y );
//	else
//		vCenter2.Set( revVSO.points[0].vPos.x, revVSO.points[0].vPos.y );
//
//	CVec2 vDir( vCenter2 - vCenter1 );
//	Normalize( &vDir );
//
//	const CVec2 vNorm1( revVSO.points[j].vNorm.x, revVSO.points[j].vNorm.y );
//	CVec2 vNorm2;
//	if ( j + 1 < revVSO.points.size() )
//		vNorm2.Set( revVSO.points[j + 1].vNorm.x, revVSO.points[j + 1].vNorm.y );
//	else
//		vNorm2.Set( revVSO.points[0].vNorm.x, revVSO.points[0].vNorm.y );
//
//	CVec2 v[4];
//	if ( singleSide.bSingleSide )
//	{
//		v[1] = vCenter1 + singleSide.fOffset1 * vNorm1; 
//		v[2] = vCenter2 + singleSide.fOffset1 * vNorm2;
//		v[0] = vCenter1 + singleSide.fOffset2 * vNorm1;
//		v[3] = vCenter2 + singleSide.fOffset2 * vNorm2;
//	}
//	else
//	{
//		v[1] = vCenter1 - vNorm1 * revVSO.points[j].fWidth * fCoeff;
//		v[2] = vCenter2 - vNorm2 * revVSO.points[j + 1].fWidth * fCoeff;
//		v[0] = vCenter1 + vNorm1 * revVSO.points[j].fWidth * fCoeff;
//		v[3] = vCenter2 + vNorm2 * revVSO.points[j + 1].fWidth * fCoeff;
//	}
//
//	v[0] -= vDir * 64;
//	v[1] -= vDir * 64;
//	v[2] += vDir * 64;
//	v[3] += vDir * 64;
//
//	const float fDist12 = fabs( v[2] - v[1] );
//	if ( fDist12 < 2.5 * fTileSize )
//	{
//		if ( fabs(fDist12) < FP_EPSILON )
//			v[2] = v[1];
//		else
//			v[2] = v[1] + ( v[2] - v[1] ) * ( 2.5 * fTileSize / fDist12 );
//	}
//	const float fDist03 = fabs( v[3] - v[0] );
//	if ( fDist03 < 2.5 * fTileSize )
//	{
//		if ( fabs(fDist03) < FP_EPSILON )
//			v[3] = v[0];
//		else
//			v[3] = v[0] + ( v[3] - v[0] ) * ( 2.5 * fTileSize / fDist03 );
//	}
//
//	// отсортировать точки против часовой стрелки
//	const CVec2 vCenter = ( v[0] + v[1] + v[2] + v[3] ) / 4.0f;
//	for ( int i = 0; i < 3; ++i )
//	{
//		for ( int j = i + 1; j < 4; ++j )
//		{
//			const WORD wDirI = GetDirectionByVector( v[i] - vCenter );
//			const WORD wDirJ = GetDirectionByVector( v[j] - vCenter );
//
//			if ( wDirI > wDirJ )
//				swap( v[i], v[j] );
//		}
//	}
//
//	pAIMap->GetTilesCoveredByQuadrangle( v[0], v[1], v[2], v[3], pTiles );
//}
////////////////////////////////////////////////////////////////////////////
//void CTerraAIObserver::AddRoad( const NDb::SVSOInstance *pInstance )
//{
//	AddVSO( pInstance );
//}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void CTerraAIObserver::AddRiver( const NDb::SVSOInstance *pInstance )
//{
//	AddVSO( pInstance );
//}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void CTerraAIObserver::SetTerraTypes( const vector<NDb::STerrainAIProperties> &params )
//{
//	pTerrain->PrepareTerraTypes( params.size() );
//	for ( int i = 0; i < params.size(); ++i )
//		pTerrain->SetTerraTypes( i, params[i].fPassability, params[i].nAIClass, params[i].nSoilType, !params[i].bCanEntrench );
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void CTerraAIObserver::UpdateTypes( const int nX1, const int nY1, const int nX2, const int nY2,
//																		const CArray2D<BYTE> &types )
//{
//	if ( !types.IsEmpty() ) 
//		pTerrain->UpdateTypes( nX1, nY1, nX2, nY2, types );
//	pTerrain->StartInitMode();
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void CTerraAIObserver::UpdateZ( CVec3 *pvPos )
//{
//	pAIMap->GetHeights()->UpdateZ( pvPos );
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//float CTerraAIObserver::GetZ( float x, float y ) const
//{
//	return pHeights->GetZ( x, y );
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//float CTerraAIObserver::GetTileHeight( int nX, int nY ) const
//{
//	return pHeights->GetTileHeight( nX, nY );
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DWORD CTerraAIObserver::GetNormal( const CVec2 &vPoint ) const
//{
//	return pHeights->GetNormal( vPoint );
//}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//bool CTerraAIObserver::GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const
//{
//	return pHeights->GetIntersectionWithTerrain( pvResult, vBegin, vEnd );
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//bool CTerraAIObserver::GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const
//{
//	return pHeights->GetIntersectionWithTerrainForEditor( pvResult, vBegin, vEnd );
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void CTerraAIObserver::InitHeights4Editor( int nSizeX, int nSizeY )
//{
//	pHeights->Init4Editor( nSizeX, nSizeY, 32 );
//}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
