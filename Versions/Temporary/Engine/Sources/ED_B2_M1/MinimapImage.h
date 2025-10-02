#if !defined(__MINIMAP_IMAGE__)
#define __MINIMAP_IMAGE__
#pragma once

#include "..\Stats_B2_M1\DBMapInfo.h"
#include "DBMinimap.h"
#include "..\SceneB2\TerrainInfo.h"
#include "..\Image\ImageScale.h"

namespace NMinimapImage
{
enum ELayerType
{
	LAYER_BRIDGE		= 0,
	LAYER_BUILDING	= 1,
	LAYER_RIVER			= 2,
	LAYER_RAILOAD		= 3,
	LAYER_ROAD			= 4,
	LAYER_FLORA			= 5,
	LAYER_GRAG			= 6,
	LAYER_SWAMP			= 7,
	LAYER_LAKE			= 8,
	LAYER_OCEAN			= 9,
	LAYER_TERRAIN		= 10,
	LAYER_COUNT			= 11,
};
//
struct SLayerType
{
	hash_map<int,int> ndb2minimapMap;
	hash_map<int,int> minimap2ndbMap;

	SLayerType()
	{
		ndb2minimapMap[NDb::LAYER_BRIDGE]		= LAYER_BRIDGE;
		ndb2minimapMap[NDb::LAYER_BUILDING]	= LAYER_BUILDING;
		ndb2minimapMap[NDb::LAYER_RIVER]		= LAYER_RIVER;
		ndb2minimapMap[NDb::LAYER_RAILOAD]	= LAYER_RAILOAD;
		ndb2minimapMap[NDb::LAYER_ROAD]			= LAYER_ROAD;
		ndb2minimapMap[NDb::LAYER_FLORA]		= LAYER_FLORA;
		ndb2minimapMap[NDb::LAYER_GRAG]			= LAYER_GRAG;
		ndb2minimapMap[NDb::LAYER_SWAMP]		= LAYER_SWAMP;
		ndb2minimapMap[NDb::LAYER_LAKE]			= LAYER_LAKE;
		ndb2minimapMap[NDb::LAYER_OCEAN]		= LAYER_OCEAN;
		ndb2minimapMap[NDb::LAYER_TERRAIN]	= LAYER_TERRAIN;
		//
		minimap2ndbMap[LAYER_BRIDGE]		= NDb::LAYER_BRIDGE;
		minimap2ndbMap[LAYER_BUILDING]	= NDb::LAYER_BUILDING;
		minimap2ndbMap[LAYER_RIVER]			= NDb::LAYER_RIVER;
		minimap2ndbMap[LAYER_RAILOAD]		= NDb::LAYER_RAILOAD;
		minimap2ndbMap[LAYER_ROAD]			= NDb::LAYER_ROAD;
		minimap2ndbMap[LAYER_FLORA]			= NDb::LAYER_FLORA;
		minimap2ndbMap[LAYER_GRAG]			= NDb::LAYER_GRAG;
		minimap2ndbMap[LAYER_SWAMP]			= NDb::LAYER_SWAMP;
		minimap2ndbMap[LAYER_LAKE]			= NDb::LAYER_LAKE;
		minimap2ndbMap[LAYER_OCEAN]			= NDb::LAYER_OCEAN;
		minimap2ndbMap[LAYER_TERRAIN]		= NDb::LAYER_TERRAIN;
	}
	int GetMinimapLayer( NDb::EMinimapLayerType eMinimapLayerType ) const
	{ 
		hash_map<int,int>::const_iterator it = ndb2minimapMap.find( eMinimapLayerType );
		if ( it != ndb2minimapMap.end() )
		{
			return it->second;	
		}
		else
		{
			return -1;
		}
	}
	int GetNDbLayer( ELayerType eLayerType ) const
	{
		hash_map<int,int>::const_iterator it = minimap2ndbMap.find( eLayerType );
		if ( it != minimap2ndbMap.end() )
		{
			return it->second;	
		}
		else
		{
			return -1;
		}
	}
};

extern const SLayerType typeLayerType;

struct SScaleType
{
	hash_map<int,int> ndb2imageMap;

	SScaleType()
	{
		ndb2imageMap[NDb::IMAGE_SCALE_METHOD_DEFAULT]		= NImage::IMAGE_SCALE_METHOD_LANCZOS3;
		ndb2imageMap[NDb::IMAGE_SCALE_METHOD_FILTER]		= NImage::IMAGE_SCALE_METHOD_FILTER;
		ndb2imageMap[NDb::IMAGE_SCALE_METHOD_BOX]				= NImage::IMAGE_SCALE_METHOD_BOX;
		ndb2imageMap[NDb::IMAGE_SCALE_METHOD_TRIANGLE]	= NImage::IMAGE_SCALE_METHOD_TRIANGLE;
		ndb2imageMap[NDb::IMAGE_SCALE_METHOD_BELL]			= NImage::IMAGE_SCALE_METHOD_BELL;
		ndb2imageMap[NDb::IMAGE_SCALE_METHOD_BSPLINE]		= NImage::IMAGE_SCALE_METHOD_BSPLINE;
		ndb2imageMap[NDb::IMAGE_SCALE_METHOD_LANCZOS3]	= NImage::IMAGE_SCALE_METHOD_LANCZOS3;
		ndb2imageMap[NDb::IMAGE_SCALE_METHOD_MITCHELL]	= NImage::IMAGE_SCALE_METHOD_MITCHELL;
	}
	NImage::EImageScaleMethod GetImageScaleMethod( NDb::EImageScaleMethod eScaleMethod ) const
	{ 
		hash_map<int,int>::const_iterator it = ndb2imageMap.find( eScaleMethod );
		if ( it != ndb2imageMap.end() )
		{
			return (NImage::EImageScaleMethod)( it->second );	
		}
		else
		{
			return NImage::IMAGE_SCALE_METHOD_LANCZOS3;
		}
	}
};

extern const SScaleType typeScaleType;

enum ERoadType
{
	ROAD_RAILROAD		= 0,
	ROAD_ROAD				= 1,
	ROAD_COUNT			= 2,
};
//
struct SRoadType
{
	hash_map<int,int> ndb2roadMap;

	SRoadType()
	{
		ndb2roadMap[0] = ROAD_RAILROAD;
		ndb2roadMap[1] = ROAD_ROAD;
	}
	ERoadType GetRoadType( int nRoadType ) const
	{ 
		hash_map<int,int>::const_iterator it = ndb2roadMap.find( nRoadType );
		if ( it != ndb2roadMap.end() )
		{
			return (ERoadType)( it->second );	
		}
		else
		{
			return ROAD_ROAD;
		}
	}
};

extern const SRoadType typeRoadType;

enum ELakeType
{
	LAKE_LAKE		= 0,
	LAKE_SWAMP	= 1,
	LAKE_COUNT	= 2,
};
//
struct SLakeType
{
	hash_map<int,int> ndb2lakeMap;

	SLakeType()
	{
		ndb2lakeMap[0] = LAKE_LAKE;
		ndb2lakeMap[1] = LAKE_SWAMP;
	}
	ELakeType GetLakeType( int nLakeType ) const
	{ 
		hash_map<int,int>::const_iterator it = ndb2lakeMap.find( nLakeType );
		if ( it != ndb2lakeMap.end() )
		{
			return (ELakeType)( it->second );	
		}
		else
		{
			return LAKE_LAKE;
		}
	}
};

extern const SLakeType typeLakeType;

typedef list<CVec2> CLakePointList;
typedef list<const CLakePointList*> CLakePointListPtrList;
struct SLakeInfo
{
	const NDb::SLakeDesc *pDescriptor;
	CLakePointList pointList;
	CLakePointListPtrList islandPointListPtrList;

	SLakeInfo() : pDescriptor( 0 ) {}
	SLakeInfo( const SLakeInfo &rLakeInfo )
		: pDescriptor( rLakeInfo.pDescriptor ),
			pointList( rLakeInfo.pointList ),
			islandPointListPtrList( rLakeInfo.islandPointListPtrList ) {}
	SLakeInfo& operator=( const SLakeInfo &rLakeInfo )
	{
		if( &rLakeInfo != this )
		{
			pDescriptor = rLakeInfo.pDescriptor;
			pointList = rLakeInfo.pointList;
			islandPointListPtrList = rLakeInfo.islandPointListPtrList;
		}
		return *this;
	}	
};
typedef list<SLakeInfo> CLakeList;


struct SCreateParameter
{
	string szImageFileName;
	CTPoint<int> size;
	//
	SCreateParameter() : size( 0, 0 ) {}
	SCreateParameter( const string &rszImageFileName,
										const CTPoint<int> &rSize )
		: szImageFileName( rszImageFileName ),
			size( rSize ) {}
	SCreateParameter( const SCreateParameter &rCreateParameter )
		: szImageFileName( rCreateParameter.szImageFileName ),
			size( rCreateParameter.size ) {}
	SCreateParameter& operator=( const SCreateParameter &rCreateParameter )
	{
		if( &rCreateParameter != this )
		{
			szImageFileName = rCreateParameter.szImageFileName;
			size = rCreateParameter.size;
		}
		return *this;
	}	
};
typedef vector<SCreateParameter> CCreateParameterList;

const NDb::SMinimapLayer* GetMinimapLayer( ELayerType eLayerType, const NDb::SMinimap *pMinimap );
void GetNormale( CVec3 *pNormale, int nXIndex, int nYIndex, const STerrainInfo *pTerrainInfo );

// particular layer renderers
void RenderLight( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, const STerrainInfo *pTerrainInfo, float fRatio, NImage::EImageScaleMethod eScaleMethod );
void RenderTerrain( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, const STerrainInfo *pTerrainInfo, NImage::EImageScaleMethod eScaleMethod );
void RenderFlora( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, int nWoodRadius, const NDb::SMinimapLayer *pMinimapLayer );
void RenderRiver( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer );
void RenderRoad( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, ERoadType eRoadType, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer );
void RenderObject( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, NDb::EObjGameType eObjGameType, bool bShowAllBuildingsPassability, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer );
void RenderBridge( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer );
void RenderOcean( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, const STerrainInfo *pTerrainInfo, const CLakeList &rLakeList, const CLakeList &rIslandList, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer );
void RenderLake( CArray2D<DWORD>* pImage, const NDb::SMapInfo *pMapInfo, const STerrainInfo *pTerrainInfo, ELakeType eLakeType, const CLakeList &rLakeList, DWORD dwMinAlpha, const NDb::SMinimapLayer *pMinimapLayer );

// Minimap creation
void Create( const NDb::SMapInfo *pMapInfo,
						 const STerrainInfo *pTerrainInfo,
						 const NDb::SMinimap *pMinimap,
						 const CCreateParameterList &rCreateParameterList );

}

#endif // !defined(__MINIMAP_IMAGE__)
