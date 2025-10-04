#include "stdafx.h"

#include "Misc/2Darray.h"
#include "3Dmotor/DBScene.h"
#include "stats_b2_m1/IconsSet.h"
#include "Misc/GeomMisc.h"
#include "Image/Targa.h"
#include "Image/ImageTGA.h"
#include "Image/ImageEffect.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MinimapImage.h"
#include "VSOManager.h"
#include "System/VFSOperations.h"

#include <zconf.h>

namespace NMinimapImage
{

const SLayerType typeLayerType;
const SScaleType typeScaleType;
const SRoadType typeRoadType;
const SLakeType typeLakeType;

struct SSetColorFunctional
{
	CArray2D<DWORD> *pDestImage;	
	DWORD dwColor;
	//
	SSetColorFunctional( CArray2D<DWORD> *_pDestImage, DWORD _dwColor )
		: pDestImage( _pDestImage ), dwColor( _dwColor )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
	}
	//
	void operator()( int nXIndex, int nYIndex )
	{ 
		( *pDestImage )[nYIndex][nXIndex] = dwColor;
	}
};


struct SBooleanSetColorFunctional
{
	CArray2D<DWORD> *pDestImage;	
	DWORD dwColor;
	//
	SBooleanSetColorFunctional( CArray2D<DWORD> *_pDestImage, DWORD _dwColor )
		: pDestImage( _pDestImage ), dwColor( _dwColor )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex )
	{ 
		( *pDestImage )[nYIndex][nXIndex] = dwColor;
		return true;
	}
};


// WBC = with bounds check
struct SWBCSetColorFunctional
{
	CArray2D<DWORD> *pDestImage;	
	DWORD dwColor;
	CTPoint<int> size;
	//
	SWBCSetColorFunctional( CArray2D<DWORD> *_pDestImage, DWORD _dwColor )
		: pDestImage( _pDestImage ), dwColor( _dwColor )
	{
		NI_ASSERT( pDestImage != 0, "Wrong parameter: pDestImage == 0" );
		size.x = pDestImage->GetSizeX();
		size.y = pDestImage->GetSizeY();
	}
	//
	void operator()( int nXIndex, int nYIndex )
	{ 
		if ( ( nXIndex >= 0 ) &&
				 ( nYIndex >= 0 ) &&
				 ( nXIndex < size.x ) &&
				 ( nYIndex < size.y ) )
		{
			( *pDestImage )[nYIndex][nXIndex] = dwColor;
		}
	}
};


const NDb::SMinimapLayer* GetMinimapLayer( ELayerType eLayerType, const NDb::SMinimap *pMinimap )
{
	NI_ASSERT( pMinimap != 0, "Wrong parameter: pMinimap == 0" );
	if ( typeLayerType.GetNDbLayer( eLayerType ) == ( -1 ) )
	{
		return 0;
	}
	NDb::EMinimapLayerType eNDbLayerType = (NDb::EMinimapLayerType)( typeLayerType.GetNDbLayer( eLayerType ) );
	for ( int nLayerIndex = 0; nLayerIndex < pMinimap->layers.size(); ++nLayerIndex )
	{
		if ( pMinimap->layers[nLayerIndex].eType == eNDbLayerType )
		{
			return &( pMinimap->layers[nLayerIndex] );
		}
	}
	return 0;
}


void GetNormale( CVec3 *pNormale, int nXIndex, int nYIndex, const STerrainInfo *pTerrainInfo )
{
	NI_ASSERT( pNormale != 0, "Wrong parameter: pNormale == 0" );
	NI_ASSERT( pTerrainInfo != 0, "Wrong parameter: pTerrainInfo == 0" );
	NI_ASSERT( ( nXIndex >= 0 ) &&
						 ( nYIndex >= 0 ) &&
						 ( nXIndex < ( pTerrainInfo->heights.GetSizeX() - 1 ) ) &&
						 ( nYIndex < ( pTerrainInfo->heights.GetSizeY() - 1 ) ),
						 //( nXIndex < ( pTerrainInfo->addHeights.GetSizeX() - 1 ) ) &&
						 //( nYIndex < ( pTerrainInfo->addHeights.GetSizeY() - 1 ) ),
						 //StrFmt( "Wrong point: height: (%d, %d), point: (%d, %d)",
							//			 pTerrainInfo->addHeights.GetSizeX(),
							//			 pTerrainInfo->addHeights.GetSizeY(),
							//			 nXIndex,
							//			 nYIndex ) );
						 StrFmt( "Wrong point: point: (%d, %d)",
										 nXIndex,
										 nYIndex ) );
	////const float fH00 = ( pTerrainInfo->heights[nYIndex + 0][nXIndex + 0] + pTerrainInfo->addHeights[nYIndex + 0][nXIndex + 0] );
	////const float fH10 = ( pTerrainInfo->heights[nYIndex + 0][nXIndex + 1] + pTerrainInfo->addHeights[nYIndex + 0][nXIndex + 1] );
	////const float fH01 = ( pTerrainInfo->heights[nYIndex + 1][nXIndex + 0] + pTerrainInfo->addHeights[nYIndex + 1][nXIndex + 0] );
	////const float fH11 = ( pTerrainInfo->heights[nYIndex + 1][nXIndex + 1] + pTerrainInfo->addHeights[nYIndex + 1][nXIndex + 1] );
	const float fH00 = ( pTerrainInfo->heights[nYIndex + 0][nXIndex + 0] );
	const float fH10 = ( pTerrainInfo->heights[nYIndex + 0][nXIndex + 1] );
	const float fH01 = ( pTerrainInfo->heights[nYIndex + 1][nXIndex + 0] );
	const float fH11 = ( pTerrainInfo->heights[nYIndex + 1][nXIndex + 1] );

	const CVec3 V1( VIS_TILE_SIZE, ( -1 ) * VIS_TILE_SIZE, fH10 - fH01 );
	const CVec3 V2( VIS_TILE_SIZE, VIS_TILE_SIZE, fH11 - fH00 );
	( *pNormale ) = V1 ^ V2;
	Normalize( pNormale );
}


void RenderLight( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, const STerrainInfo *pTerrainInfo, float fRatio, NImage::EImageScaleMethod eScaleMethod )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( pMapInfo != 0, "Wrong parameter: pTerrainInfo == 0" );
	NI_ASSERT( pTerrainInfo != 0, "Wrong parameter: pTerrainInfo == 0" );
	NI_ASSERT( pMapInfo->pLight != 0, "Wrong parameter: pMapInfo->pLight == 0" );
	if ( fabs( fRatio ) <= FP_EPSILON )
	{
		pImage->FillEvery( NImage::GRAY_LIGHTER_COLOR );
		return;
	}
	//map size in VIS tiles
	const CTPoint<int> size( pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH, pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH );
	//
	const float fHor = sin( ToRadian( pMapInfo->pLight->fPitch ) );
	CVec3 vLight = CVec3( fHor * cos( ToRadian( pMapInfo->pLight->fYaw ) ),
												fHor * sin( ToRadian( pMapInfo->pLight->fYaw ) ),
												-cos( ToRadian( pMapInfo->pLight->fPitch ) ) );
	Normalize( &vLight );
	vLight *= ( -1.0f );
	//
	BYTE nMinShade = 0xFF;
	BYTE nMaxShade = 0x00;
	CVec3 vNormale = VNULL3;
	BYTE nShade = 0;
	CArray2D<DWORD> shadeImage( size.x, size.y );
	shadeImage.FillZero();
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			GetNormale( &vNormale, nXIndex, nYIndex, pTerrainInfo );
			nShade = (BYTE)( ( 255.0f * Clamp( vNormale * vLight, 0.0f, 1.0f ) ) + 0.5f );
			if ( nShade > nMaxShade )
			{
				nMaxShade = nShade;
			}
			if ( nShade < nMinShade )
			{
				nMinShade = nShade;
			}
			shadeImage[nYIndex][nXIndex] = MakeARGBColor<DWORD>( 0xFF, nShade, nShade, nShade );
		}
	}
	NImage::FullColor( &shadeImage, fRatio );
	NImage::Scale( pImage, shadeImage, eScaleMethod );
}


void RenderTerrain( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, const STerrainInfo *pTerrainInfo, NImage::EImageScaleMethod eScaleMethod )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( pMapInfo != 0, "Wrong parameter: pMapInfo == 0" );
	NI_ASSERT( pTerrainInfo != 0, "Wrong parameter: pTerrainInfo == 0" );
	//map size in VIS tiles
	const CTPoint<int> size( pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH, pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH );
	CArray2D<DWORD> terrainImage( size.x, size.y );
	terrainImage.FillZero();
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			const int nColorIndex = pTerrainInfo->tileTerraMap[nYIndex][nXIndex];
			if ( ( nColorIndex >= 0 ) && ( nColorIndex < pMapInfo->pTerraSet->terraTypes.size() ) )
			{
				terrainImage[nYIndex][nXIndex] = pMapInfo->pTerraSet->terraTypes[nColorIndex]->nColor;
			}
		}
	}
	NImage::Scale( pImage, terrainImage, eScaleMethod );
}


void RenderFlora( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, int nWoodRadius, const NDb::SMinimapLayer *pMinimapLayer )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( pMapInfo != 0, "Wrong parameter: pMapInfo == 0" );
	NI_ASSERT( pMinimapLayer != 0, "Wrong parameter: pMapInfo == 0" );
	//map size in AI tiles
	const CTPoint<int> size( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH );
	CArray2D<DWORD> floraImage( size.x, size.y );
	floraImage.FillZero();
	//
	SSetColorFunctional setColorFunctional( &floraImage, pMinimapLayer->nColor );
	SWBCSetColorFunctional wbcSetColorFunctional( &floraImage, pMinimapLayer->nColor );
	for ( int nObjectIndex = 0; nObjectIndex < pMapInfo->objects.size(); ++nObjectIndex )
	{
		if ( pMapInfo->objects[nObjectIndex].pObject == 0 )
		{
			continue;
		}
		if ( pMapInfo->objects[nObjectIndex].pObject->eGameType == NDb::SGVOGT_FLORA )
		{
			const int nXIndex = ( pMapInfo->objects[nObjectIndex].vPos.x / AI_TILE_SIZE + 0.5f );
			const int nYIndex = ( pMapInfo->objects[nObjectIndex].vPos.y / AI_TILE_SIZE + 0.5f );
			if ( ( nXIndex > nWoodRadius ) &&
					( nXIndex < ( size.x - nWoodRadius ) ) &&
					( nYIndex > nWoodRadius ) &&
					( nYIndex < ( size.y - nWoodRadius ) ) )
			{
				BresenhamFilledCircle( nXIndex, nYIndex, nWoodRadius, setColorFunctional );
			}
			else
			{
				BresenhamFilledCircle( nXIndex, nYIndex, nWoodRadius, wbcSetColorFunctional );
			}
		}
	}
	NImage::Scale( pImage, floraImage, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
}


void GetVSOColors( DWORD *pdwCenterColor, DWORD *pdwBorderColor, int nCenterColor, int nBorderColor, int nCenterWidth, int nColor )
{
	NI_ASSERT( pdwCenterColor != 0, "Wrong parameter: pdwCenterColor == 0" );
	NI_ASSERT( pdwBorderColor != 0, "Wrong parameter: pdwBorderColor == 0" );

	( *pdwCenterColor ) = nCenterColor;
	( *pdwBorderColor ) = nBorderColor;
	//
	if ( nCenterWidth == 0 )
	{
		( *pdwCenterColor ) = 0;
	}
	else if ( nCenterWidth == 100 )
	{
		( *pdwBorderColor ) = ( *pdwCenterColor );
		( *pdwCenterColor ) = 0;
	}
	if ( ( GetAlphaFromARGBColor( ( *pdwCenterColor ) ) == 0 ) &&
			 ( GetAlphaFromARGBColor( ( *pdwBorderColor ) ) == 0 ) )
	{
		( *pdwBorderColor ) = nColor;
		( *pdwCenterColor ) = 0;
	}
}


void RenderRiver( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( pMapInfo != 0, "Wrong parameter: pMapInfo == 0" );
	//map size in IA tiles
	const CTPoint<int> size( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH );
	const CTRect<int> rect( 0, 0, size.x, size.y );
	//
	CArray2D<DWORD> riverImage( size.x, size.y );
	riverImage.FillZero();
	// render all borders
	for ( int nRiverIndex = 0; nRiverIndex < pMapInfo->rivers.size(); ++nRiverIndex )
	{
		if ( pMapInfo->rivers[nRiverIndex].pDescriptor == 0 )
		{
			continue;
		}
		const NDb::SRiverDesc *pRiverDesc = checked_cast<const NDb::SRiverDesc*>( pMapInfo->rivers[nRiverIndex].pDescriptor.GetPtr() );
		//
		DWORD dwCenterColor = 0;
		DWORD dwBorderColor = 0;
		GetVSOColors( &dwCenterColor,
									&dwBorderColor,
									pRiverDesc->nMiniMapCenterColor,
									pRiverDesc->nMiniMapBorderColor,
									pRiverDesc->nMiniMapCenterWidth,
									pMinimapLayer->nColor );
		//
		if ( GetAlphaFromARGBColor( dwBorderColor ) >= dwMinAlpha )
		{
			list<CVec3> polygon;
			SBooleanSetColorFunctional setColorFunctional( &riverImage, dwBorderColor );
			for ( int nPointIndex = 0; nPointIndex < ( pMapInfo->rivers[nRiverIndex].points.size() - 1 ); ++nPointIndex )
			{
				CVSOManager::GetBoundingPolygon( &polygon, pMapInfo->rivers[nRiverIndex].points, nPointIndex, CVSOManager::PT_BOTH, 1.0f );
				ApplyTilesInPolygon<SBooleanSetColorFunctional, list<CVec3>, CVec3>( rect, polygon, AI_TILE_SIZE, setColorFunctional );
			}
		}
	}
	// render all centers
	for ( int nRiverIndex = 0; nRiverIndex < pMapInfo->rivers.size(); ++nRiverIndex )
	{
		if ( pMapInfo->rivers[nRiverIndex].pDescriptor == 0 )
		{
			continue;
		}
		const NDb::SRiverDesc *pRiverDesc = checked_cast<const NDb::SRiverDesc*>( pMapInfo->rivers[nRiverIndex].pDescriptor.GetPtr() );
		//
		DWORD dwCenterColor = 0;
		DWORD dwBorderColor = 0;
		GetVSOColors( &dwCenterColor,
									&dwBorderColor,
									pRiverDesc->nMiniMapCenterColor,
									pRiverDesc->nMiniMapBorderColor,
									pRiverDesc->nMiniMapCenterWidth,
									pMinimapLayer->nColor );
		//
		if ( GetAlphaFromARGBColor( dwCenterColor ) >= dwMinAlpha )
		{
			const float fRelWidth = pRiverDesc->nMiniMapCenterWidth / 100.0f;
			list<CVec3> polygon;
			SBooleanSetColorFunctional setColorFunctional( &riverImage, dwCenterColor );
			for ( int nPointIndex = 0; nPointIndex < ( pMapInfo->rivers[nRiverIndex].points.size() - 1 ); ++nPointIndex )
			{
				CVSOManager::GetBoundingPolygon( &polygon, pMapInfo->rivers[nRiverIndex].points, nPointIndex, CVSOManager::PT_BOTH, fRelWidth );
				ApplyTilesInPolygon<SBooleanSetColorFunctional, list<CVec3>, CVec3>( rect, polygon, AI_TILE_SIZE, setColorFunctional );
			}
		}
	}
	NImage::Scale( pImage, riverImage, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
}


void RenderRoad( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, ERoadType eRoadType, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( pMapInfo != 0, "Wrong parameter: pMapInfo == 0" );
	//map size in AI tiles
	const CTPoint<int> size( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH );
	const CTRect<int> rect( 0, 0, size.x, size.y );
	//
	CArray2D<DWORD> roadImage( size.x, size.y );
	roadImage.FillZero();
	list<int> roadIndexList;
	for ( int nRoadIndex = 0; nRoadIndex < pMapInfo->roads.size(); ++nRoadIndex )
	{
		if ( pMapInfo->roads[nRoadIndex].pDescriptor == 0 )
		{
			continue;
		}
		const NDb::SRoadDesc *pRoadDesc = checked_cast<const NDb::SRoadDesc*>( pMapInfo->roads[nRoadIndex].pDescriptor.GetPtr() );
		if ( typeRoadType.GetRoadType( pRoadDesc->nType ) == eRoadType )
		{
			bool bNotInserted = true;
			for ( list<int>::iterator itRoadIndex = roadIndexList.begin(); itRoadIndex != roadIndexList.end(); ++itRoadIndex )
			{
				const NDb::SRoadDesc *pLocalRoadDesc = checked_cast<const NDb::SRoadDesc*>( pMapInfo->roads[*itRoadIndex].pDescriptor.GetPtr() );
				if ( pLocalRoadDesc->nPriority > pRoadDesc->nPriority )
				{
					roadIndexList.insert( itRoadIndex, nRoadIndex );
					bNotInserted = false;
					break;
				}
			}
			if ( bNotInserted )
			{
				roadIndexList.push_back( nRoadIndex );
			}
		}
	}
	// render all borders
	for ( list<int>::const_iterator itRoadIndex = roadIndexList.begin(); itRoadIndex != roadIndexList.end(); ++itRoadIndex )
	{
		int nRoadIndex = *itRoadIndex;
		const NDb::SRoadDesc *pRoadDesc = checked_cast<const NDb::SRoadDesc*>( pMapInfo->roads[nRoadIndex].pDescriptor.GetPtr() );
		//
		DWORD dwCenterColor = 0;
		DWORD dwBorderColor = 0;
		GetVSOColors( &dwCenterColor,
									&dwBorderColor,
									pRoadDesc->nMiniMapCenterColor,
									pRoadDesc->nMiniMapBorderColor,
									pRoadDesc->nMiniMapCenterWidth,
									pMinimapLayer->nColor );
		//
		if ( GetAlphaFromARGBColor( dwBorderColor ) >= dwMinAlpha )
		{
			list<CVec3> polygon;
			SBooleanSetColorFunctional setColorFunctional( &roadImage, dwBorderColor );
			for ( int nPointIndex = 0; nPointIndex < ( pMapInfo->roads[nRoadIndex].points.size() - 1 ); ++nPointIndex )
			{
				CVSOManager::GetBoundingPolygon( &polygon, pMapInfo->roads[nRoadIndex].points, nPointIndex, CVSOManager::PT_BOTH, 1.0f );
				ApplyTilesInPolygon<SBooleanSetColorFunctional, list<CVec3>, CVec3>( rect, polygon, AI_TILE_SIZE, setColorFunctional );
			}
		}
	}
	// render all centers
	for ( list<int>::const_iterator itRoadIndex = roadIndexList.begin(); itRoadIndex != roadIndexList.end(); ++itRoadIndex )
	{
		int nRoadIndex = *itRoadIndex;
		const NDb::SRoadDesc *pRoadDesc = checked_cast<const NDb::SRoadDesc*>( pMapInfo->roads[nRoadIndex].pDescriptor.GetPtr() );
		//
		DWORD dwCenterColor = 0;
		DWORD dwBorderColor = 0;
		GetVSOColors( &dwCenterColor,
									&dwBorderColor,
									pRoadDesc->nMiniMapCenterColor,
									pRoadDesc->nMiniMapBorderColor,
									pRoadDesc->nMiniMapCenterWidth,
									pMinimapLayer->nColor );
		//
		if ( GetAlphaFromARGBColor( dwCenterColor ) >= dwMinAlpha )
		{
			const float fRelWidth = pRoadDesc->nMiniMapCenterWidth / 100.0f;
			list<CVec3> polygon;
			SBooleanSetColorFunctional setColorFunctional( &roadImage, dwCenterColor );
			for ( int nPointIndex = 0; nPointIndex < ( pMapInfo->roads[nRoadIndex].points.size() - 1 ); ++nPointIndex )
			{
				CVSOManager::GetBoundingPolygon( &polygon, pMapInfo->roads[nRoadIndex].points, nPointIndex, CVSOManager::PT_BOTH, fRelWidth );
				ApplyTilesInPolygon<SBooleanSetColorFunctional, list<CVec3>, CVec3>( rect, polygon, AI_TILE_SIZE, setColorFunctional );
			}
		}
	}
	NImage::Scale( pImage, roadImage, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
}


void RenderObject( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, NDb::EObjGameType eObjGameType, bool bShowAllBuildingsPassability, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( pMapInfo != 0, "Wrong parameter: pMapInfo == 0" );
	//map size in AI tiles
	const CTPoint<int> size( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH );
	const CTRect<int> rect( 0, 0, size.x, size.y );
	CArray2D<DWORD> objectImage( size.x, size.y );
	objectImage.FillZero();
	//
	SWBCSetColorFunctional wbcSetColorFunctional( &objectImage, pMinimapLayer->nColor );
	for ( int nObjectIndex = 0; nObjectIndex < pMapInfo->objects.size(); ++nObjectIndex )
	{
		if ( pMapInfo->objects[nObjectIndex].pObject == 0 )
		{
			continue;
		}
		if ( pMapInfo->objects[nObjectIndex].pObject->eGameType == eObjGameType )
		{
			const NDb::SHPObjectRPGStats::SByteArray2 *pPassability = 0;
			CVec3 vPos = VNULL3;
			float fDirection = 0;
			CVec2 vOrigin = VNULL2;
			switch ( eObjGameType )
			{
				case NDb::SGVOGT_BUILDING:
				default:
				{
					const NDb::SBuildingRPGStats *pBuildingRPGStats = checked_cast<const NDb::SBuildingRPGStats*>( pMapInfo->objects[nObjectIndex].pObject.GetPtr() );
					if ( pBuildingRPGStats == 0 )
					{
						continue;
					}
					pPassability = &( pBuildingRPGStats->passability );
					vPos = pMapInfo->objects[nObjectIndex].vPos;
					fDirection = AI2VisRad( pMapInfo->objects[nObjectIndex].nDir ) + FP_PI2;
					vOrigin = pBuildingRPGStats->vOrigin;
					break;
				}
			}
			if ( pPassability != 0 )
			{
				CTRect<int> indices( 0, 0, pPassability->GetSizeX(), pPassability->GetSizeY() );
				NormalazeDirection( &fDirection );
				for ( int nYIndex = indices.miny; nYIndex < indices.maxy; ++ nYIndex )
				{
					for ( int nXIndex = indices.minx; nXIndex < indices.maxx; ++ nXIndex )
					{
						if ( bShowAllBuildingsPassability || ( ( *pPassability )[indices.maxy - nYIndex - 1 + indices.miny][nXIndex] != 0 ) )
						{
							CVec2 vTilePos( nXIndex * AI_TILE_SIZE + AI_TILE_SIZE / 2.0f - vOrigin.x,
															nYIndex * AI_TILE_SIZE + AI_TILE_SIZE / 2.0f - vOrigin.y );
							RotatePoint( &vTilePos, fDirection );
							vTilePos.x += vPos.x;
							vTilePos.y += vPos.y;
							wbcSetColorFunctional( vTilePos.x / AI_TILE_SIZE, vTilePos.y / AI_TILE_SIZE );
						}
					}
				}
			}
		}
	}
	// for close all holes int the image
	NImage::MarkEdge( &objectImage, pMinimapLayer->nColor, true, dwMinAlpha );
	NImage::Scale( pImage, objectImage, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
}


void RenderBridge( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( pMapInfo != 0, "Wrong parameter: pMapInfo == 0" );
	//map size in AI tiles
	const CTPoint<int> size( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH );
	const CTRect<int> rect( 0, 0, size.x, size.y );
	CArray2D<DWORD> objectImage( size.x, size.y );
	objectImage.FillZero();
	//
	SWBCSetColorFunctional wbcSetColorFunctional( &objectImage, pMinimapLayer->nColor );
	for ( int nObjectIndex = 0; nObjectIndex < pMapInfo->objects.size(); ++nObjectIndex )
	{
		if ( pMapInfo->objects[nObjectIndex].pObject == 0 )
		{
			continue;
		}
		if ( pMapInfo->objects[nObjectIndex].pObject->eGameType == NDb::SGVOGT_BRIDGE )
		{
			const NDb::SBridgeRPGStats *pBridgeRPGStats = checked_cast<const NDb::SBridgeRPGStats*>( pMapInfo->objects[nObjectIndex].pObject.GetPtr() );
			if ( pBridgeRPGStats == 0 )
			{
				continue;
			}
			CVec2 vHalfSize = VNULL2;
			if ( pMapInfo->objects[nObjectIndex].nFrameIndex == 0 )
			{
				vHalfSize = pBridgeRPGStats->end.vSize / 2.0f;
			}
			else
			{
				vHalfSize = pBridgeRPGStats->center.vSize / 2.0f;
			}
			const CVec3 vPos = pMapInfo->objects[nObjectIndex].vPos;
			float fDirection = AI2VisRad( pMapInfo->objects[nObjectIndex].nDir );
			NormalazeDirection( &fDirection );
			CTRect<int> indices( ( AI_TILE_SIZE / 2.0f - vHalfSize.x ) / AI_TILE_SIZE,
													 ( AI_TILE_SIZE / 2.0f - vHalfSize.y ) / AI_TILE_SIZE,
													 ( AI_TILE_SIZE / 2.0f + vHalfSize.x ) / AI_TILE_SIZE,
													 ( AI_TILE_SIZE / 2.0f + vHalfSize.y ) / AI_TILE_SIZE );
			for ( int nYIndex = indices.miny; nYIndex < indices.maxy; ++nYIndex )
			{
				for ( int nXIndex = indices.minx; nXIndex < indices.maxx; ++nXIndex )
				{
					CVec2 vTilePos( nXIndex * AI_TILE_SIZE + AI_TILE_SIZE / 2.0f,
													nYIndex * AI_TILE_SIZE + AI_TILE_SIZE / 2.0f );
					RotatePoint( &vTilePos, fDirection );
					vTilePos.x += vPos.x;
					vTilePos.y += vPos.y;
					wbcSetColorFunctional( vTilePos.x / AI_TILE_SIZE, vTilePos.y / AI_TILE_SIZE );
				}
			}
		}
	}
	// for close all holes int the image
	NImage::MarkEdge( &objectImage, pMinimapLayer->nColor, true, dwMinAlpha );
	NImage::Scale( pImage, objectImage, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
}


void GetLakeColors( DWORD *pdwCenterColor, DWORD *pdwBorderColor, int nCenterColor, int nBorderColor, int nGradientWidth, int nColor )
{
	NI_ASSERT( pdwCenterColor != 0, "Wrong parameter: pdwCenterColor == 0" );
	NI_ASSERT( pdwBorderColor != 0, "Wrong parameter: pdwBorderColor == 0" );

	( *pdwCenterColor ) = nCenterColor;
	( *pdwBorderColor ) = nBorderColor;
	//
	if ( nGradientWidth == 0 )
	{
		( *pdwBorderColor ) = 0;
	}
	if ( ( GetAlphaFromARGBColor( ( *pdwCenterColor ) ) == 0 ) &&
			 ( GetAlphaFromARGBColor( ( *pdwBorderColor ) ) == 0 ) )
	{
		( *pdwCenterColor ) = nColor;
		( *pdwBorderColor ) = 0;
	}
}


void RenderOcean( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, const STerrainInfo *pTerrainInfo, const CLakeList &rLakeList, const CLakeList &rIslandList, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( pMapInfo != 0, "Wrong parameter: pMapInfo == 0" );
	NI_ASSERT( pTerrainInfo != 0, "Wrong parameter: pTerrainInfo == 0" );
	if ( !pMapInfo->bHasCoast || pMapInfo->coast.controlPoints.empty() || ( pMapInfo->coast.pDescriptor == 0 ) || pTerrainInfo->seaMask.IsEmpty() )
	{
		return;
	}
	//map size in AI tiles
	const CTPoint<int> size( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH );
	CArray2D<DWORD> oceanImage( size.x, size.y );
	oceanImage.FillZero();
	//
	const NDb::SCoastDesc *pCoastDescriptor = dynamic_cast<const NDb::SCoastDesc*>( &( *( pMapInfo->coast.pDescriptor ) ) );
	if ( pCoastDescriptor == 0 )
	{
		return;
	}
	DWORD dwCenterColor = 0;
	DWORD dwBorderColor = 0;
	GetLakeColors( &dwCenterColor, 
								 &dwBorderColor,
								 pCoastDescriptor->nMiniMapCenterColor,
								 pCoastDescriptor->nMiniMapBorderColor,
								 pCoastDescriptor->nMiniMapGradientWidth,
								 pMinimapLayer->nColor );
	//
	if ( GetAlphaFromARGBColor( dwCenterColor ) < dwMinAlpha )
	{
		return;
	}
	const float fGradientWidth = pCoastDescriptor->nMiniMapGradientWidth * AI_TILE_SIZE * 1.0f;
	CLakePointList coastPointList;
	for ( int nCoastPointIndex = 0; nCoastPointIndex < pMapInfo->coast.points.size(); ++nCoastPointIndex )
	{
		coastPointList.push_back( CVec2( pMapInfo->coast.points[nCoastPointIndex].vPos.x,
																		 pMapInfo->coast.points[nCoastPointIndex].vPos.y ) );
	}
	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			if ( pTerrainInfo->seaMask[nYIndex / AI_TILES_IN_VIS_TILE][nXIndex / AI_TILES_IN_VIS_TILE] > 0 )
			{
				const CVec2 vTileCenter = CVec2( nXIndex * AI_TILE_SIZE * 1.0f + AI_TILE_SIZE / 2.0f,
																				 nYIndex * AI_TILE_SIZE * 1.0f + AI_TILE_SIZE / 2.0f );
				bool bNotLake = true;
				for ( CLakeList::const_iterator itLake = rLakeList.begin(); itLake != rLakeList.end(); ++itLake )
				{
					if ( ClassifyPolygon( itLake->pointList, vTileCenter ) != CP_OUTSIDE )
					{
						bNotLake = false;
					}
				}
				if ( bNotLake )
				{
					if ( ( GetAlphaFromARGBColor<DWORD>( dwBorderColor ) > 0 ) && ( pCoastDescriptor->nMiniMapGradientWidth > 0 ) )
					{
						float fDistance = fabs( PolygonDistance( coastPointList, vTileCenter, false ) );
						for ( CLakeList::const_iterator itIsland = rIslandList.begin(); itIsland != rIslandList.end(); ++itIsland )
						{
							float fIslandDistance = PolygonDistance( itIsland->pointList, vTileCenter, true );
							if ( fIslandDistance <= 0.0f )
							{
								fIslandDistance *= ( -1.0f );
								if ( fIslandDistance < fDistance )
								{
									fDistance = fIslandDistance;
								}
							}
						}
						if ( fDistance > fGradientWidth )
						{
							oceanImage[nYIndex][nXIndex] = dwCenterColor;
						}
						else
						{
							oceanImage[nYIndex][nXIndex] = GetARGBColorGradient( dwBorderColor, dwCenterColor, fDistance / fGradientWidth );
						}
					}
					else
					{
						oceanImage[nYIndex][nXIndex] = dwCenterColor;
					}
				}
			}
		}
	}
	NImage::Scale( pImage, oceanImage, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
}


void RenderLake( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, const STerrainInfo *pTerrainInfo, ELakeType eLakeType, const CLakeList &rLakeList, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer )
{
	NI_ASSERT( pImage != 0, "Wrong parameter: pImage == 0" );
	NI_ASSERT( pMapInfo != 0, "Wrong parameter: pMapInfo == 0" );
	NI_ASSERT( pTerrainInfo != 0, "Wrong parameter: pTerrainInfo == 0" );
	if ( pTerrainInfo->seaMask.IsEmpty() )
	{
		return;
	}
	//map size in AI tiles
	const CTPoint<int> size( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH );
	CArray2D<DWORD> lakeImage( size.x, size.y );
	lakeImage.FillZero();

	for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			if ( pTerrainInfo->seaMask[nYIndex / AI_TILES_IN_VIS_TILE][nXIndex / AI_TILES_IN_VIS_TILE] > 0 )
			{
				const CVec2 vTileCenter = CVec2( nXIndex * AI_TILE_SIZE * 1.0f + AI_TILE_SIZE / 2.0f,
																				 nYIndex * AI_TILE_SIZE * 1.0f + AI_TILE_SIZE / 2.0f );
				for ( CLakeList::const_iterator itLake = rLakeList.begin(); itLake != rLakeList.end(); ++itLake )
				{
					if ( typeLakeType.GetLakeType( itLake->pDescriptor->nType ) == eLakeType )
					{
						if ( ClassifyPolygon( itLake->pointList, vTileCenter ) != CP_OUTSIDE )
						{
							DWORD dwCenterColor = 0;
							DWORD dwBorderColor = 0;
							GetLakeColors( &dwCenterColor, 
														&dwBorderColor,
														itLake->pDescriptor->nMiniMapCenterColor,
														itLake->pDescriptor->nMiniMapBorderColor,
														itLake->pDescriptor->nMiniMapGradientWidth,
														pMinimapLayer->nColor );
							if ( ( GetAlphaFromARGBColor<DWORD>( dwBorderColor ) > 0 ) && ( itLake->pDescriptor->nMiniMapGradientWidth > 0 ) )
							{
								const float fGradientWidth = itLake->pDescriptor->nMiniMapGradientWidth * AI_TILE_SIZE * 1.0f;
								float fDistance = fabs( PolygonDistance( itLake->pointList, vTileCenter, true ) );
								for ( CLakePointListPtrList::const_iterator itIslandPointListPtr = itLake->islandPointListPtrList.begin(); itIslandPointListPtr != itLake->islandPointListPtrList.end(); ++itIslandPointListPtr )
								{
									float fIslandDistance = PolygonDistance( *( *itIslandPointListPtr ), vTileCenter, true );
									if ( fIslandDistance <= 0.0f )
									{
										fIslandDistance *= ( -1.0f );
										if ( fIslandDistance < fDistance )
										{
											fDistance = fIslandDistance;
										}
									}
								}
								if ( fDistance > fGradientWidth )
								{
									lakeImage[nYIndex][nXIndex] = dwCenterColor;
								}
								else
								{
									lakeImage[nYIndex][nXIndex] = GetARGBColorGradient( dwBorderColor, dwCenterColor, fDistance / fGradientWidth );
								}
							}
							else
							{
								lakeImage[nYIndex][nXIndex] = dwCenterColor;
							}
						}
					}
				}
			}
		}
	}
	NImage::Scale( pImage, lakeImage, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
}


void Create( const NDb::SMapInfo *pMapInfo,
						 const STerrainInfo *pTerrainInfo,
						 const NDb::SMinimap *pMinimap,
						 const CCreateParameterList &rCreateParameterList )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	NI_ASSERT( pUserData != 0, "Wrong parameter: pUserData == 0" );
	NI_ASSERT( pMapInfo != 0, "Wrong parameter: pMapInfo == 0" );
	NI_ASSERT( pTerrainInfo != 0, "Wrong parameter: pTerrainInfo == 0" );
	NI_ASSERT( pMinimap != 0, "Wrong parameter: pMinimap == 0" );
	NI_ASSERT( ( pTerrainInfo->tileTerraMap.GetSizeX() >= pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH ) &&
						 ( pTerrainInfo->tileTerraMap.GetSizeY() >= pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH ),
						 StrFmt( "Wrong terrain size: terrain: (%d, %d), mapInfo: (%d, %d)",
										 pTerrainInfo->tileTerraMap.GetSizeX(),
										 pTerrainInfo->tileTerraMap.GetSizeY(),
										 pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH,
										 pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH ) );
	//map size in VIS tiles
	const CTPoint<int> mapSize( pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH, pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH );
	//minimap image size in AI tiles
	const CTPoint<int> imageSize( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH );
	//bounding boxes for various methods
	const CTRect<int> mapRect( 0, 0, mapSize.x, mapSize.y );
	const CTRect<int> imageRect( 0, 0, imageSize.x, imageSize.y );
	//
	CLakeList lakeList;
	CLakeList islandList;
	for ( int nLakeIndex = 0; nLakeIndex < pMapInfo->lakes.size(); ++nLakeIndex )
	{
		const NDb::SLakeDesc *pLakeDescriptor = dynamic_cast<const NDb::SLakeDesc*>( &( *pMapInfo->lakes[nLakeIndex].pDescriptor ) );
		if ( pLakeDescriptor != 0 ) 
		{
			if ( pLakeDescriptor->bIsLake )
			{
				CLakeList::iterator itLake = lakeList.insert( lakeList.end(), SLakeInfo() );
				itLake->pDescriptor = pLakeDescriptor;
				for ( int nLakePointIndex = 0; nLakePointIndex < pMapInfo->lakes[nLakeIndex].points.size(); ++nLakePointIndex )
				{
					itLake->pointList.push_back( CVec2( pMapInfo->lakes[nLakeIndex].points[nLakePointIndex].vPos.x,
																							pMapInfo->lakes[nLakeIndex].points[nLakePointIndex].vPos.y ) );
				}
			}
			else
			{
				CLakeList::iterator itIsland = islandList.insert( islandList.end(), SLakeInfo() );
				itIsland->pDescriptor = pLakeDescriptor;
				for ( int nLakePointIndex = 0; nLakePointIndex < pMapInfo->lakes[nLakeIndex].points.size(); ++nLakePointIndex )
				{
					itIsland->pointList.push_back( CVec2( pMapInfo->lakes[nLakeIndex].points[nLakePointIndex].vPos.x,
																								pMapInfo->lakes[nLakeIndex].points[nLakePointIndex].vPos.y ) );
				}
			}
		}
	}
	for ( CLakeList::const_iterator itIsland = islandList.begin(); itIsland != islandList.end(); ++itIsland )
	{
		for ( CLakeList::iterator itLake = lakeList.begin(); itLake != lakeList.end(); ++itLake )
		{
			if ( ClassifyPolygon<CLakePointList, CVec2>( itLake->pointList, itIsland->pointList ) != CP_OUTSIDE )
			{
				itLake->islandPointListPtrList.push_back( &( itIsland->pointList ) );
			}
		}
	}
	//
	// create render layers
	vector<CArray2D<DWORD>*> layerList;
	for ( int nLayerIndex = 0; nLayerIndex < LAYER_COUNT; ++nLayerIndex )
	{
		vector<CArray2D<DWORD>*>::iterator itLayer = layerList.insert( layerList.end(), 0 );
	}
	// render layers
	{
		ELayerType eLayerType = LAYER_TERRAIN;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( mapSize.x, mapSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				RenderTerrain( pLayer, pMapInfo, pTerrainInfo, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
				//apply terrain shadows
				if ( pMinimap->bShowTerrainShades )
				{
					CArray2D<DWORD> lightImage( mapSize.x, mapSize.y );
					RenderLight( &lightImage, pMapInfo, pTerrainInfo, pMinimap->nTerrainShadeRatio / 100.0f, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
					NImage::Noise( pLayer, lightImage, true, pMinimap->nMinAlpha );
				}
			}
		}
	}
	{
		ELayerType eLayerType = LAYER_OCEAN;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				RenderOcean( pLayer, pMapInfo, pTerrainInfo, lakeList, islandList, pMinimap->nMinAlpha, pMinimapLayer );
			}
		}
	}
	{
		ELayerType eLayerType = LAYER_LAKE;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				RenderLake( pLayer, pMapInfo, pTerrainInfo, LAKE_LAKE, lakeList, pMinimap->nMinAlpha, pMinimapLayer );
			}
		}
	}
	{
		ELayerType eLayerType = LAYER_SWAMP;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				RenderLake( pLayer, pMapInfo, pTerrainInfo, LAKE_SWAMP, lakeList, pMinimap->nMinAlpha, pMinimapLayer );
			}
		}
	}
	{
		ELayerType eLayerType = LAYER_GRAG;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				//RenderCrag( pLayer, pMapInfo, pTerrainInfo, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
			}
		}
	}
	{
		ELayerType eLayerType = LAYER_FLORA;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				RenderFlora( pLayer, pMapInfo, pMinimap->nWoodRadius, pMinimapLayer );
			}
		}
	}
	{
		ELayerType eLayerType = LAYER_ROAD;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				RenderRoad( pLayer, pMapInfo, ROAD_ROAD, pMinimap->nMinAlpha, pMinimapLayer );
			}
		}
	}
	{
		ELayerType eLayerType = LAYER_RAILOAD;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				RenderRoad( pLayer, pMapInfo, ROAD_RAILROAD, pMinimap->nMinAlpha, pMinimapLayer );
			}
		}
	}
	{
		ELayerType eLayerType = LAYER_RIVER;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				RenderRiver( pLayer, pMapInfo, pMinimap->nMinAlpha, pMinimapLayer );
			}
		}
	}
	{
		ELayerType eLayerType = LAYER_BUILDING;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				RenderObject( pLayer, pMapInfo, NDb::SGVOGT_BUILDING, pMinimap->bShowAllBuildingsPassability, pMinimap->nMinAlpha, pMinimapLayer );
			}
		}
	}
	{
		ELayerType eLayerType = LAYER_BRIDGE;
		{
			NI_ASSERT( ( eLayerType >= 0 ) && ( eLayerType < LAYER_COUNT ), StrFmt( "Invalid layer index: %d", eLayerType ) );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( eLayerType, pMinimap );
			if ( pMinimapLayer != 0 )
			{
				layerList[eLayerType] = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				CArray2D<DWORD> *pLayer = layerList[eLayerType];
				pLayer->FillZero();
				RenderBridge( pLayer, pMapInfo, pMinimap->nMinAlpha, pMinimapLayer );
			}
		}
	}
	// mark edges
	for ( int nLayerIndex = 0; nLayerIndex < LAYER_COUNT; ++nLayerIndex )
	{
		const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( (ELayerType)( nLayerIndex ), pMinimap );
		if ( pMinimapLayer != 0 )
		{
			if ( ( GetAlphaFromARGBColor<DWORD>( pMinimapLayer->nBorderColor ) > pMinimap->nMinAlpha ) &&
					 ( pMinimapLayer->nBorderWidth > 0 ) )
			{
				CArray2D<DWORD> *pLayer = layerList[nLayerIndex];
				if ( pMinimapLayer->nBorderWidth > 0 )
				{
					NImage::MarkEdge( pLayer, pMinimapLayer->nBorderColor, false, pMinimap->nMinAlpha );
					for ( int nBorderIndex = 1; nBorderIndex < pMinimapLayer->nBorderWidth; ++nBorderIndex )
					{
						NImage::MarkEdge( pLayer, pMinimapLayer->nBorderColor, true, pMinimap->nMinAlpha );
					}
				}
			}
		}
	}
	// create alpha emboss layers
	vector<CArray2D<DWORD>*> aphaEmbossLayerList;
	for ( int nLayerIndex = 0; nLayerIndex < LAYER_COUNT; ++nLayerIndex )
	{
		vector<CArray2D<DWORD>*>::iterator itAphaEmbossLayer = aphaEmbossLayerList.insert( aphaEmbossLayerList.end(), 0 );
		( *itAphaEmbossLayer ) = 0;
		const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( (ELayerType)( nLayerIndex ), pMinimap );
		if ( pMinimapLayer != 0 )
		{
			if ( ( ( pMinimapLayer->embossPoint.nx != 0 ) ||
						 ( pMinimapLayer->embossPoint.ny != 0 ) ) &&
					 ( pMinimapLayer->nEmbossFilterSize > 0 ) )
			{
				const CArray2D<DWORD> *pLayer = layerList[nLayerIndex];
				( *itAphaEmbossLayer ) = new CArray2D<DWORD>( pLayer->GetSizeX(), pLayer->GetSizeY() );
				( *itAphaEmbossLayer )->FillZero();
				NImage::GetAlphaEmboss( ( *itAphaEmbossLayer ),
																( *pLayer ),
																CTPoint<int>( pMinimapLayer->embossPoint.nx, pMinimapLayer->embossPoint.ny ),
																pMinimapLayer->nEmbossFilterSize,
																pMinimap->nMinAlpha );
			}
		}
	}
	// create shadow layers
	vector<CArray2D<DWORD>*> shadowLayerList;
	for ( int nLayerIndex = 0; nLayerIndex < LAYER_COUNT; ++nLayerIndex )
	{
		vector<CArray2D<DWORD>*>::iterator itShadowLayer = shadowLayerList.insert( shadowLayerList.end(), 0 );
		( *itShadowLayer ) = 0;
		const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( (ELayerType)( nLayerIndex ), pMinimap );
		if ( pMinimapLayer != 0 )
		{
			if ( ( pMinimapLayer->shadowPoint.nx != 0 ) ||
					 ( pMinimapLayer->shadowPoint.ny != 0 ) )
			{
				const CArray2D<DWORD> *pLayer = layerList[nLayerIndex];
				( *itShadowLayer ) = new CArray2D<DWORD>( pLayer->GetSizeX(), pLayer->GetSizeY() );
				( *itShadowLayer )->FillZero();
				NImage::GetShadow( ( *itShadowLayer ),
													 ( *pLayer ),
													 CTPoint<int>( pMinimapLayer->shadowPoint.nx, pMinimapLayer->shadowPoint.ny ),
													 NImage::GRAY_LIGHTER_COLOR,
													 NImage::WHITE_COLOR,
													 pMinimap->nMinAlpha );
			}
		}
	}
	// write images for all sizes
	for ( CCreateParameterList::const_iterator itCreateParameter = rCreateParameterList.begin(); itCreateParameter != rCreateParameterList.end(); ++itCreateParameter )
	{
		NI_ASSERT( ( itCreateParameter->size.x != 0 ) &&
							 ( itCreateParameter->size.y != 0 ) &&
							 ( !itCreateParameter->szImageFileName.empty() ),
							 StrFmt( "Invalid image size: (%d, %d) or name: %s",
											 itCreateParameter->size.x,
											 itCreateParameter->size.y,
											 itCreateParameter->szImageFileName.c_str() ) );
		//Collect noises
		vector<CArray2D<DWORD>*> noisedLayerList;
		for ( int nLayerIndex = 0; nLayerIndex < LAYER_COUNT; ++nLayerIndex )
		{
			vector<CArray2D<DWORD>*>::iterator itNoisedLayer = noisedLayerList.insert( noisedLayerList.end(), 0 );
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( (ELayerType)( nLayerIndex ), pMinimap );
			if ( pMinimapLayer != 0 )
			{
				( *itNoisedLayer ) = new CArray2D<DWORD>( imageSize.x, imageSize.y );
				NImage::Scale( *itNoisedLayer, ( *( layerList[nLayerIndex] ) ), typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
				// Noise Layer
				const string szNoiseFileName = pUserData->constUserData.szExportSourceFolder + pMinimapLayer->szNoiseImage;
				CArray2D<DWORD> noise;
				CFileStream stream( NVFS::GetMainVFS(), szNoiseFileName );
				if ( stream.IsOk() )
				{
					if ( NImage::RecognizeFormatTGA( &stream ) )
					{
						NImage::LoadTGAImage( noise, &stream );
					}
				}
				if ( ( noise.GetSizeX() * noise.GetSizeY() ) != 0 )
				{
					if ( pMinimapLayer->bScaleNoise )
					{
						if ( ( imageSize.x != itCreateParameter->size.x ) ||
								 ( imageSize.y != itCreateParameter->size.y ) )
						{
							CArray2D<DWORD> scaledNoise( ( noise.GetSizeX() * imageSize.x ) / itCreateParameter->size.x + 0.5f,
																					 ( noise.GetSizeY() * imageSize.y ) / itCreateParameter->size.y + 0.5f );
							scaledNoise.FillZero();
							NImage::Scale( &scaledNoise, noise, typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) );
							NImage::Noise( ( *itNoisedLayer ), scaledNoise, true, pMinimap->nMinAlpha );
						}
						else
						{
							NImage::Noise( ( *itNoisedLayer ), noise, true, pMinimap->nMinAlpha );
						}
					}
					else
					{
						NImage::Noise( ( *itNoisedLayer ), noise, true, pMinimap->nMinAlpha );
					}
				}
			}
		}		
		//Compose image
		//for each layer:
		//* add layer
		//* apply alpha emboss
		//* add shadow
		bool bFirstLayerAdded = false;
		CArray2D<DWORD> minimapImage( imageSize.x, imageSize.y );
		for ( int nLayerIndex = ( LAYER_COUNT - 1 ); nLayerIndex >= 0; --nLayerIndex )
		{
			const NDb::SMinimapLayer *pMinimapLayer = GetMinimapLayer( (ELayerType)( nLayerIndex ), pMinimap );
			if ( ( pMinimapLayer != 0 ) && ( noisedLayerList[nLayerIndex] != 0 ) )
			{
				//add layer
				if ( bFirstLayerAdded )
				{
					NImage::FastAddImageByAlpha( &minimapImage, ( *( noisedLayerList[nLayerIndex] ) ), pMinimap->nMinAlpha );
				}
				else
				{
					minimapImage = ( *( noisedLayerList[nLayerIndex] ) );
					bFirstLayerAdded = true;
				}
				//apply alpha emboss
				if ( aphaEmbossLayerList[nLayerIndex] != 0 )
				{
					CArray2D<DWORD> *pAlphaEmbossLayer = aphaEmbossLayerList[nLayerIndex];
					if ( ( pAlphaEmbossLayer->GetSizeX() != imageSize.x ) ||
							 ( pAlphaEmbossLayer->GetSizeY() != imageSize.y ) )
					{
						CArray2D<DWORD> alphaEmboss( imageSize.x, imageSize.y );
						NImage::Scale( &alphaEmboss, ( *pAlphaEmbossLayer ), typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) ); 
						NImage::Noise( &minimapImage, alphaEmboss, true, pMinimap->nMinAlpha );
					}
					else
					{
						NImage::Noise( &minimapImage, ( *pAlphaEmbossLayer ), true, pMinimap->nMinAlpha );
					}
				}
				//add shadow
				if ( shadowLayerList[nLayerIndex] != 0 )
				{
					CArray2D<DWORD> *pShadowLayer = shadowLayerList[nLayerIndex];
					if ( ( pShadowLayer->GetSizeX() != imageSize.x ) ||
							 ( pShadowLayer->GetSizeY() != imageSize.y ) )
					{
						CArray2D<DWORD> shadow( imageSize.x, imageSize.y );
						NImage::Scale( &shadow, ( *pShadowLayer ), typeScaleType.GetImageScaleMethod( pMinimapLayer->eScaleMethod ) ); 
						NImage::Noise( &minimapImage, shadow, true, pMinimap->nMinAlpha );
					}
					else
					{
						NImage::Noise( &minimapImage, ( *pShadowLayer ), true, pMinimap->nMinAlpha );
					}
				}
				/**
				{
					const string szFileName = StrFmt( "C:\\B2\\Editor\\MinimapTest\\step%d.tga", nLayerIndex );
					CFileStream imageStream( szFileName, CFileStream::WIN_CREATE );
					NImage::FlipY( minimapImage );
					NImage::SaveAsTGA( minimapImage, &imageStream );
					NImage::FlipY( minimapImage );
				}
				/**/
			}
		}
		//compress and save image
		if ( ( imageSize.x != itCreateParameter->size.x ) || ( imageSize.y != itCreateParameter->size.y ) )
		{
			CArray2D<DWORD> compressedMinimapImage( itCreateParameter->size.x, itCreateParameter->size.y );
			NImage::Scale( &compressedMinimapImage, minimapImage, NImage::IMAGE_SCALE_METHOD_LANCZOS3 );
			minimapImage = compressedMinimapImage;
		}
		CFileStream imageStream( NVFS::GetMainFileCreator(), itCreateParameter->szImageFileName );
		NImage::FlipY( minimapImage );
		NImage::SaveAsTGA( minimapImage, &imageStream );
		// delete noises
		{
			int nIndex = 0;
			for ( vector<CArray2D<DWORD>*>::iterator itNoisedLayer = noisedLayerList.begin(); itNoisedLayer != noisedLayerList.end(); ++itNoisedLayer )
			{
				if ( ( *itNoisedLayer ) != 0 )
				{
					/**
					{
						const string szFileName = StrFmt( "C:\\B2\\Editor\\MinimapTest\\noised%d.tga", nIndex );
						CFileStream imageStream(  szFileName, CFileStream::WIN_CREATE );
						NImage::FlipY( *( *itNoisedLayer ) );
						NImage::SaveAsTGA( *( *itNoisedLayer ), &imageStream );
					}
					/**/
					delete ( *itNoisedLayer );
					( *itNoisedLayer ) = 0;
				}
				++nIndex;
			}
		}
	}
	// delete shadow layers
	{
		int nIndex = 0;
		for ( vector<CArray2D<DWORD>*>::iterator itShadowLayer = shadowLayerList.begin(); itShadowLayer != shadowLayerList.end(); ++itShadowLayer )
		{
			if ( ( *itShadowLayer ) != 0 )
			{
				/**
				{
					const string szFileName = StrFmt( "C:\\B2\\Editor\\MinimapTest\\shadow%d.tga", nIndex );
					CFileStream imageStream(  szFileName, CFileStream::WIN_CREATE );
					NImage::FlipY( *( *itShadowLayer ) );
					NImage::SaveAsTGA( *( *itShadowLayer ), &imageStream );
				}
				/**/
				delete ( *itShadowLayer );
				( *itShadowLayer ) = 0;
			}
			++nIndex;
		}
	}
	// delete alpha emboss layers
	{
		int nIndex = 0;
		for ( vector<CArray2D<DWORD>*>::iterator itAphaEmbossLayer = aphaEmbossLayerList.begin(); itAphaEmbossLayer != aphaEmbossLayerList.end(); ++itAphaEmbossLayer )
		{
			if ( ( *itAphaEmbossLayer ) != 0 )
			{
				/**
				{
					const string szFileName = StrFmt( "C:\\B2\\Editor\\MinimapTest\\alphaEmboss%d.tga", nIndex );
					CFileStream imageStream(  szFileName, CFileStream::WIN_CREATE );
					NImage::FlipY( *( *itAphaEmbossLayer ) );
					NImage::SaveAsTGA( *( *itAphaEmbossLayer ), &imageStream );
				}
				/**/
				delete ( *itAphaEmbossLayer );
				( *itAphaEmbossLayer ) = 0;
			}
			++nIndex;
		}
	}
	// delete render layers
	{
		int nIndex = 0;
		for ( vector<CArray2D<DWORD>*>::iterator itLayer = layerList.begin(); itLayer != layerList.end(); ++itLayer )
		{
			if ( ( *itLayer ) != 0 )
			{
				/**
				{
					const string szFileName = StrFmt( "C:\\B2\\Editor\\MinimapTest\\layer%d.tga", nIndex );
					CFileStream imageStream(  szFileName, CFileStream::WIN_CREATE );
					NImage::FlipY( *( *itLayer ) );
					NImage::SaveAsTGA( *( *itLayer ), &imageStream );
				}
				/**/
				delete ( *itLayer );
				( *itLayer ) = 0;
			}
			++nIndex;
		}
	}
}

}

// basement storage  

/**
// rotate image
CArray2D<DWORD> minimapRotate( minimapImage.GetSizeX() + minimapImage.GetSizeY() - 1, 
																minimapImage.GetSizeX() + minimapImage.GetSizeY() - 1 );
minimapRotate.FillZero();
for ( int x = 0;  x < minimapImage.GetSizeX(); ++x )
{
	for ( int y = 0; y < minimapImage.GetSizeY(); ++y )
	{
		const int _x = x + y;
		const int _y = x + minimapImage.GetSizeY() - y - 1;
		const DWORD dwColor11 = minimapImage[y][x];
		minimapRotate[_y][_x] = dwColor11;
		if ( ( x > 0 ) && ( y > 0 ) )
		{
			const DWORD dwColor10 = minimapImage[y - 1][x];
			const DWORD dwColor01 = minimapImage[y][x - 1];
			const DWORD dwColor00 = minimapImage[y - 1][x - 1];
			const DWORD dwR = ( GetRedFromARGBColor( dwColor00 ) + GetRedFromARGBColor(dwColor01 ) + GetRedFromARGBColor( dwColor10 ) + GetRedFromARGBColor( dwColor11 ) ) / 4;
			const DWORD dwG = ( GetGreenFromARGBColor( dwColor00 ) + GetGreenFromARGBColor( dwColor01 ) + GetGreenFromARGBColor( dwColor10 ) + GetGreenFromARGBColor( dwColor11 ) )  / 4;
			const DWORD dwB = ( GetBlueFromARGBColor( dwColor00 ) + GetBlueFromARGBColor( dwColor01 ) + GetBlueFromARGBColor( dwColor10 ) + GetBlueFromARGBColor( dwColor11 ) ) /4;
			minimapRotate[_y][_x - 1] = MakeARGBColor<DWORD>( 0xFF, dwR, dwG, dwB );
		}
	}
}
NImage::Scale( &minimapImage, minimapRotate, NImage::IMAGE_SCALE_METHOD_LANCZOS3 );
CFileStream imageStream(  itCreateParameter->szImageFileName, CFileStream::CREATE );
//NImage::FlipY( minimapImage );
NImage::SaveAsTGA( minimapImage, &imageStream );
/**/

