#pragma once

#include "../Misc/2DArray.h"
#include "../Misc/BitData.h"
#include "../B2_M1_Terrain/TerrUtils.h"

#define DEF_PATCH_SIZE_BITS 4
#define DEF_PATCH_SIZE ( 1 << DEF_PATCH_SIZE_BITS )
#define DEF_PATCH_PIX_PER_TILE 32
#define DEF_PATCH_PIX_PER_TILE_BITS 5
#define DEF_PATCH_PIX_PER_TILE_INV (1.0f / DEF_PATCH_PIX_PER_TILE)
#define DEF_PATCH_TEX_SIZE (DEF_PATCH_SIZE * DEF_PATCH_PIX_PER_TILE)
#define DEF_PATCH_TEX_SIZE_BITS (DEF_PATCH_SIZE_BITS + DEF_PATCH_PIX_PER_TILE_BITS)
#define DEF_TILE_SIZE VIS_TILE_SIZE
#define DEF_INV_TILE_SIZE (1.0f / DEF_TILE_SIZE)

#define DEF_PATCH_WORLD_SIZE ( (float)DEF_PATCH_SIZE * DEF_TILE_SIZE )
#define DEF_INV_PATCH_WORLD_SIZE ( 1.0f / DEF_PATCH_WORLD_SIZE )

#define DEF_WATER_BLENDING_ZERO_DIST ( DEF_TILE_SIZE * FP_SQRT_2 * 3 )
#define DEF_WATER_BLENDING_DIST ( DEF_TILE_SIZE * 4 + DEF_WATER_BLENDING_ZERO_DIST )
#define DEF_WATER_BLENDING_ZERO_COEFF ( DEF_WATER_BLENDING_ZERO_DIST / DEF_WATER_BLENDING_DIST )
#define DEF_WATER_BLENDING_NONZERO_COEFF ( 1.0f / ( 1.0f - DEF_WATER_BLENDING_ZERO_COEFF ) )

#define DEF_BREAK_TERRA_PATCHES_ONE_AXIS 4
#define DEF_BREAK_TERRA_PATCHES (DEF_BREAK_TERRA_PATCHES_ONE_AXIS * DEF_BREAK_TERRA_PATCHES_ONE_AXIS)
#define DEF_BREAK_TERRA_PATCHES_STEP ( DEF_PATCH_SIZE / DEF_BREAK_TERRA_PATCHES_ONE_AXIS )

struct STerrainInfo
{
	struct STile
	{
		vector<CVec3fEx> vertices;
		vector<float> addHeights;
		vector<STriangle> triangles;
		//
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &vertices );
			saver.Add( 2, &addHeights );
			saver.Add( 3, &triangles );
			return 0;
		}
	};
	//
	CArray2D<STile> tiles;						// terrain tiles													[N]x[M]
	CArray2D<float> heights;					// terrain heights												[N+1]x[M+1]
	CArray2D<float> addHeights;				// increased heights											[N+1]x[M+1]
	int nNumPatchesX, nNumPatchesY;		// number of patches on X & Y dimensions
	int nDescID;											// ID of terrain descriptor

	// roads part
	struct SRoad
	{
		int nID;
		CVec2 vBBMin, vBBMax;
		//
		bool operator == ( const SRoad &v ) const { return nID == v.nID; }
		//
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &nID );
			saver.Add( 2, &vBBMin );
			saver.Add( 3, &vBBMax );
			return 0;
		}
	};
	// VSO Point
	struct SVSOPoint
	{
		CVec3 vPos;
		CVec3 vNorm;
		float fWidth;
		float fOpacity;
		bool bKeyPoint;
		float fRadius;
		float fReserved;

		SVSOPoint()
			: vPos( VNULL3 ),
			vNorm( VNULL3 ),
			fWidth( 0.0f ),
			fOpacity( 0.0f ),
			bKeyPoint( false ),
			fRadius( 0.0f ),
			fReserved( 0.0f )
		{}
		SVSOPoint( const NDb::SVSOPoint &rPoint )
			: vPos( rPoint.vPos ),
			vNorm( rPoint.vNorm ),
			fWidth( rPoint.fWidth ),
			fOpacity( rPoint.fOpacity ),
			bKeyPoint( rPoint.bKeyPoint ),
			fRadius( rPoint.fRadius ),
			fReserved( rPoint.fReserved )
		{}

		SVSOPoint& operator= ( const NDb::SVSOPoint &rPoint )
		{
			this->vPos = rPoint.vPos;
			this->vNorm = rPoint.vNorm;
			this->fWidth = rPoint.fWidth;
			this->fOpacity = rPoint.fOpacity;
			this->bKeyPoint = rPoint.bKeyPoint;
			this->fRadius = rPoint.fRadius;
			this->fReserved = rPoint.fReserved;

			return *this;
		}

		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &fOpacity );
			saver.Add( 2, &fWidth );
			saver.Add( 3, &fReserved );
			saver.Add( 4, &fRadius );
			saver.Add( 5, &vPos );
			saver.Add( 6, &bKeyPoint );
			saver.Add( 7, &vNorm );
			return 0;
		}
	};
	// crags part
	struct SCrag
	{
		int nID;
		int nRandSeed;
		vector<STerrainInfo::SVSOPoint> sampPoints;
		vector<CVec3fEx> ridge;
		vector<CVec3fEx> samples;
		CVec2i vSampMin, vSampMax;
		CVec2i vRidgeMin, vRidgeMax;
		CVec2 vBBMin, vBBMax;
		vector<CVec3> precVerts;
		vector<float> precHeights;
		vector<CVec3> precNorms;
		CDBPtr<NDb::SCragDesc> pDesc;
		//
		bool operator == ( const SCrag &v ) const { return nID == v.nID; }
		//
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &nID );
			saver.Add( 2, &nRandSeed );
			saver.Add( 3, &sampPoints );
			saver.Add( 4, &ridge );
			saver.Add( 5, &samples );
			saver.Add( 6, &vSampMin );
			saver.Add( 7, &vSampMax );
			saver.Add( 8, &vRidgeMin );
			saver.Add( 9, &vRidgeMax );
			saver.Add( 10, &vBBMin );
			saver.Add( 11, &vBBMax );
			saver.Add( 12, &precVerts );
			saver.Add( 13, &precHeights );
			saver.Add( 14, &precNorms );
			saver.Add( 15, &pDesc );
			return 0;
		}
	};
	// peaks part
	struct SPeak
	{
		int nID;
		vector< vector<STerrainInfo::SVSOPoint> > points;
		CVec2 vBBMin, vBBMax;
		//
		bool operator == ( const SPeak &v ) const { return nID == v.nID; }
		//
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &nID );
			saver.Add( 2, &points );
			saver.Add( 3, &vBBMin );
			saver.Add( 4, &vBBMax );
			return 0;
		}
	};
	// foots part
	struct SFoot
	{
		int nID;
		vector< vector<STerrainInfo::SVSOPoint> > points;
		float fTexGeomScale;
		CDBPtr<NDb::SMaterial> pFootMaterial;
		//
		bool operator == ( const SFoot &v ) const { return nID == v.nID; }
		//
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &nID );
			saver.Add( 2, &points );
			saver.Add( 3, &fTexGeomScale );
			saver.Add( 4, &pFootMaterial );
			return 0;
		}
	};
	// rivers part
	struct SRiver
	{
		int nID;
		int nRandSeed;
		vector<CVec3fEx> samples;
		vector<CVec3> precVertsL, precVertsR;
		vector<float> precHeightsL, precHeightsR;
		vector<CVec3> precNormsL, precNormsR;
		CVec2i vSampMin, vSampMax;
		CVec2 vBBMin, vBBMax;
		CDBPtr<NDb::SRiverDesc> pDesc;
		vector<CVec3fEx> ridgeL;
		vector<CVec3fEx> ridgeR;
		int nWaterSamplesNum;
		int nWater2SamplesNum;
		//
		bool operator == ( const SRiver &v ) const { return nID == v.nID; }
		//
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &nID );
			saver.Add( 2, &nRandSeed );
			saver.Add( 3, &samples );
			saver.Add( 4, &precVertsL );
			saver.Add( 5, &precVertsR );
			saver.Add( 6, &precHeightsL );
			saver.Add( 7, &precHeightsR );
			saver.Add( 8, &precNormsL );
			saver.Add( 9, &precNormsR );
			saver.Add( 10, &vSampMin );
			saver.Add( 11, &vSampMax );
			saver.Add( 12, &vBBMin );
			saver.Add( 13, &vBBMax );
			saver.Add( 14, &pDesc );
			saver.Add( 15, &ridgeL );
			saver.Add( 16, &ridgeR );
			saver.Add( 17, &nWaterSamplesNum );
			saver.Add( 18, &nWater2SamplesNum );
			return 0;
		}
	};
	// terraspots part
	struct STerraSpot
	{
		int nID;
		CVec2i vMin, vMax;
		//
		bool operator == ( const STerraSpot &v ) const { return nID == v.nID; }
		//
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &nID );
			saver.Add( 2, &vMin );
			saver.Add( 3, &vMax );
			return 0;
		}
	};
	// precipice node
	struct SPrecipiceNode
	{
		CVec2 vPos;
		CVec3 vNorm;
		float fMinHeight;
		float fMaxHeight;
		int nCount;
		vector<CVec3> verts;
		list<int> precs;
		float fDepth;
		float fDepthRand;
		float fRandX, fRandY;
		//
		int operator &( IBinSaver &saver )
		{
			saver.Add( 1, &vPos );
			saver.Add( 2, &vNorm );
			saver.Add( 3, &fMinHeight );
			saver.Add( 4, &fMaxHeight );
			saver.Add( 5, &nCount );
			saver.Add( 6, &verts );
			saver.Add( 7, &precs );
			saver.Add( 8, &fDepth );
			saver.Add( 9, &fDepthRand );
			saver.Add( 10, &fRandX );
			saver.Add( 11, &fRandY );
			return 0;
		}
	};
	// precipice
	struct SPrecipice
	{
		int nID;
		int nExcludeRiverID;
		vector<int> nodes;
		vector<CVec3> norms;
		vector<float> minHeights;
		vector<float> maxHeights;
		vector<BYTE> visibles;
		vector<int> intersectors;
		CVec2 vMin, vMax;
		CDBPtr<NDb::SMaterial> pMaterial;
		CDBPtr<NDb::SMaterial> pFootMaterial;
		float fTexGeomScale;
		BYTE bStayedOnTerrain;
		float fDepth;
		float fDepthRand;
		float fRandX, fRandY;
		bool bHasNotPeak;
		//
		bool operator == ( const SPrecipice &v ) const { return nID == v.nID; }
		//
		int operator &( IBinSaver &saver )
		{
			saver.Add( 1, &nID );
			saver.Add( 2, &nExcludeRiverID );
			saver.Add( 3, &nodes );
			saver.Add( 4, &norms );
			saver.Add( 5, &minHeights );
			saver.Add( 6, &maxHeights );
			saver.Add( 7, &visibles );
			saver.Add( 8, &intersectors );
			saver.Add( 9, &vMin );
			saver.Add( 10, &vMax );
			saver.Add( 11, &pMaterial );
			saver.Add( 12, &pFootMaterial );
			saver.Add( 13, &fTexGeomScale );
			saver.Add( 14, &bStayedOnTerrain );
			saver.Add( 15, &fDepth );
			saver.Add( 16, &fDepthRand );
			saver.Add( 17, &fRandX );
			saver.Add( 18, &fRandY );
			saver.Add( 19, &bHasNotPeak );
			return 0;
		}
	};

	list<SRoad> roads;									// roads array
	list<SCrag> crags;									// crags array
	list<SPeak> peaks;									// peaks array
	list<SFoot> foots;
	list<SRiver> rivers;								// rivers array
	list<STerraSpot> terraspots;				// terraspots array
	vector<SPrecipiceNode> precNodes;		// precipice nodes
	list<SPrecipice> precipices;				// precipices
	CArray2D<BYTE> seaMask;

	struct SOptimizedTile
	{
		int nIndex;
		vector<STriangle> triangles;
		vector<CVec3> vertices;
		vector<float> addHeights;
		//
		int operator&( IBinSaver &saver )
		{
			saver.Add( 15, &nIndex );
			saver.Add( 16, &triangles );
			saver.Add( 17, &vertices );
			saver.Add( 18, &addHeights );
			return 0;
		}
	};

	vector<SOptimizedTile> optimizedTiles;
	CArray2D<short int> optimizedHeights;
	CArray2D<short int> optimizedAddHeights;
	CArray2D1Bit optimizedSeaMask;
	//
	CArray2D<BYTE> tileTerraMap;
	int nRecreateRandSeed;
	//
	CArray2D<float> riverHeights;
	//
	CArray2D<float> waterHeightCoeffs;
	CArray2D<float> waterAddHeights;
	vector<BYTE> waterAddHeightsPacked;
	//////////////////////////////////////////////////////////////////////////
	//
	int operator&( IBinSaver &saver )
	{
		//saver.Add( 1, &tiles );
		saver.Add( 2, &heights );
		saver.Add( 3, &addHeights );
		saver.Add( 4, &nNumPatchesX );
		saver.Add( 5, &nNumPatchesY );
		saver.Add( 6, &nDescID );
		saver.Add( 20, &roads );
		saver.Add( 21, &crags );
		saver.Add( 22, &rivers );
		saver.Add( 23, &terraspots );
		//saver.Add( 24, &peaks );
		saver.Add( 25, &precNodes );
		saver.Add( 26, &precipices );
		saver.Add( 27, &seaMask );
		saver.Add( 28, &foots );
		saver.Add( 31, &optimizedTiles );
		saver.Add( 32, &optimizedHeights );
		saver.Add( 33, &optimizedAddHeights );
		saver.Add( 34, &optimizedSeaMask );
		saver.Add( 35, &tileTerraMap );
		saver.Add( 36, &nRecreateRandSeed );
		saver.Add( 37, &riverHeights );
		saver.Add( 39, &waterAddHeightsPacked );
		return 0;
	}
};

struct STerraObjectInfo
{
	int nID;
	CVec3 vPos;
	float fAngle;
	CVec2 vOrigin;
	string szMaskFileName;
	string szTexFileName;
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &nID );
		saver.Add( 2, &vPos );
		saver.Add( 3, &fAngle );
		saver.Add( 4, &vOrigin );
		saver.Add( 5, &szMaskFileName );
		saver.Add( 6, &szTexFileName );
		return 0;
	}
};


