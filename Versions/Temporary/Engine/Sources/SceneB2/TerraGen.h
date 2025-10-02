#pragma once

#include "SceneB2_export.h"

#include "VersionInfo.h"

namespace NDb
{
	struct SMaterial;
	struct STerrain;
	struct STerrainSpotInstance;
	struct SVSOInstance;
	struct SVSOPoint;
	struct SWater;
};
namespace NMeshData
{
	struct SMeshData;
};
namespace NGScene
{
	class IGameView;
	class IMaterial;
};
namespace NDebrisBuilder
{
	enum EMaskSmoothType { MASK_SMOOTH_SHARP, MASK_SMOOTH_MEDIUM, MASK_SMOOTH_BLUR };
	enum EMaskType { MASK_STATIC, MASK_DYNAMIC, MASK_AI_PASSABILITY };
};
namespace NTerraBrush
{
	enum ETerraBrushUpdate { TERRA_BRUSH_ADD, TERRA_BRUSH_SUB, TERRA_BRUSH_OVERRIDE };
}
struct SRoadGFXInfo;
struct SCragGFXInfo;
struct SPeakGFXInfo;
struct SFootGFXInfo;
struct SRiverGFXInfo;
struct STerraSpotGFXInfo;
struct STerraObjectInfo;
struct SPrecipiceGFXInfo;
class CTracksManager;
struct ITerraAIObserver;
struct STerrainInfo;

struct ITerraManager : public CObjectBase
{
	enum { tidTypeID = 0x10096401 };
	//
	//virtual void SetStreamPathes( const string &szDstPath, const string &szSrcPath ) = 0;
	virtual void Load( const NDb::STerrain *pDesc, CDataStream *pStream ) = 0;
	virtual void SetAIObserver( ITerraAIObserver *pObserver ) = 0;
	virtual ITerraAIObserver *GetAIObserver() = 0;
	virtual void UpdateAIInfo() = 0;
	//
	virtual void ReGenerate() = 0;
	virtual const NDb::STerrain *GetDesc() const = 0;
	//
	virtual void Save( CDataStream *pStream ) = 0;
	// road
	virtual void AddRoad( const NDb::SVSOInstance *pInstance ) = 0;
	virtual void UpdateRoad( const int nVSOID ) = 0;
	virtual void RemoveRoad( const int nVSOID ) = 0;
	// crag
	virtual void AddCrag( const NDb::SVSOInstance *pInstance ) = 0;
	virtual void UpdateCrag( const int nVSOID ) = 0;
	virtual void RemoveCrag( const int nVSOID ) = 0;
	// river
	virtual void AddRiver( const NDb::SVSOInstance *pInstance ) = 0;
	virtual void UpdateRiver( const int nVSOID ) = 0;
	virtual void RemoveRiver( const int nVSOID ) = 0;
	// terra object
	//virtual void AddTerraObject( const STerraObjectInfo *pObjInfo ) = 0;
	//virtual void UpdateTerraObject( const STerraObjectInfo *pObjInfo ) = 0;
	//virtual void RemoveTerraObject( const int nID ) = 0;
	// terra spot
	virtual void AddTerraSpot( const NDb::STerrainSpotInstance *pInstance ) = 0;
	virtual void UpdateTerraSpot( const int nSpotID ) = 0;
	virtual void RemoveTerraSpot( const int nSpotID ) = 0;
	// entrenchment
	virtual void AddEntrenchment( const vector<CVec2> &_ctrlPoints, const float _fWidth ) = 0;
	// debris creation
	virtual void CreateDebris( const string &szFileName, CArray2D<BYTE> *pImage, CVec2 *pOrigin,
														 const NDebrisBuilder::EMaskType maskType, const int nSmoothRadius,
														 const NDebrisBuilder::EMaskSmoothType smoothType = NDebrisBuilder::MASK_SMOOTH_SHARP ) = 0;
	// geometry editor
	virtual void ModifyTerraGeometryByBrush( const int nVisTileX, const int nVisTileY, bool bCenter, const CArray2D<float> &brush,
																					 const NTerraBrush::ETerraBrushUpdate terraBrushUpdate = NTerraBrush::TERRA_BRUSH_OVERRIDE ) = 0;
	virtual void GetTerraGeometryUpdateDifferences( int *pOffsX, int *pOffsY, CArray2D<float> *pDiffs ) = 0;
	virtual void UpdateAllObjectsInGeomModifyingArea() = 0;
	// water placement
	//virtual void SetSeaPlacement( const vector<NDb::SVSOPoint> &samples, const NDb::SWater *pWaterParams,
	//	const CVec2 &_vMidPoint, const bool bRemove ) = 0;
	//virtual void SetWaterZonePlacement( const vector<NDb::SVSOPoint> &samples, const NDb::SWater *pWaterParams,
	//	const bool bLake, const bool bRemove ) = 0;
	virtual void UpdateWater() = 0;
	//
	virtual float GetTerraHeight( const float x, const float y ) const = 0;
	virtual float GetRealTerraHeight( const float x, const float y ) const = 0;
	//
	virtual float GetTerraHeightFast( const int nTileX, const int nTileY ) const = 0;
	virtual float GetRealTerraHeightFast( const int nTileX, const int nTileY ) const = 0;
	//
	virtual void UpdateRiversDepthes() = 0;
	//
	// dynamic debris
	virtual void AddDynamicDebris( const CVec2 &vPos, const CVec2 &vSize, const float fAngle, const int nSmoothRad,
		const NDb::SMaterial *pMaterial ) = 0;
	//
	// terra former
	virtual void ApplyBridgeTerraForm( const CVec2 &_p1, const CVec2 &_p2, const float fWidth, const float fHeight ) = 0;
	virtual void ApplyObjectTerraForm( const CVec2 &_p1, const CVec2 &_p2, const CVec2 &_p3, const CVec2 &_p4 ) = 0;
	//
	virtual void UpdateTileAreaType( const float fXo, const float fYo, const CArray2D<BYTE> &mask,
																	 const NTerraBrush::ETerraBrushUpdate terraBrushUpdate = NTerraBrush::TERRA_BRUSH_OVERRIDE ) = 0;
	virtual void GetTileTypeUpdateDifferences( float *pOffsX, float *pOffsY, CArray2D<BYTE> *pDiffs ) = 0;
	virtual void FinalizeTexModifying() = 0;
	//
	// reflections
	virtual void ClampUnderRivers( NMeshData::SMeshData *pData ) = 0;
	//
	virtual void GetAreaTileTypes( CArray2D<BYTE> *pAreaTypes, const int nX1, const int nY1, const int nX2, const int nY2 ) = 0;
	virtual void GetAreaHeights( CArray2D<float> *pAreaHeights, const int nX1, const int nY1, const int nX2, const int nY2 ) = 0;
	//
	virtual void UpdateAfterTilesModifying() = 0;

	virtual float GetZ( float x, float y ) const = 0;
	virtual float GetTileHeight( int nX, int nY ) const = 0;
	virtual void UpdateZ( CVec3 *pvPos ) = 0;
	virtual DWORD GetNormal( const CVec2 &vPoint ) const = 0;
	virtual bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const = 0;
	virtual bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const = 0;
	virtual void InitHeights4Editor( int nSizeX, int nSizeY ) = 0;
	//
	virtual bool GetCragPrecVerts( vector<CVec3> *pVerts, int nVSOId ) = 0;
	//
	virtual int GetTilesCountX() const = 0;
	virtual int GetTilesCountY() const = 0;
	//
	virtual const STerrainInfo* const GetTerraInfo() const = 0;
};

struct ITerraGfxObserver : public CObjectBase
{
	// terrain itself
	virtual void UpdatePatchGeometry( vector<NMeshData::SMeshData> *pMeshData, const int nPatchInd ) = 0;
	virtual void UpdateBorderGeometry( vector<NMeshData::SMeshData> *pMeshData, const int nPatchInd ) = 0;
	// road
	virtual void AddRoad( SRoadGFXInfo *pRoadGfxInfo ) = 0;
	virtual bool UpdateRoad( const int nVSOID ) = 0;
	virtual void RemoveRoad( const int nVSOID ) = 0;
	virtual void RemoveAllRoads() = 0;
	// crag
	//virtual void AddCrag( const SCragGFXInfo *pCragGfxInfo ) = 0;
	//virtual bool UpdateCrag( const int nVSOID ) = 0;
	//virtual void RemoveCrag( const int nVSOID ) = 0;
	// peak
	virtual void AddPeak( SPeakGFXInfo *pPeakGfxInfo ) = 0;
	virtual bool UpdatePeak( const int nVSOID ) = 0;
	virtual void RemovePeak( const int nVSOID ) = 0;
	virtual void RemoveAllPeaks() = 0;
	// foot
	virtual void AddFoot( SFootGFXInfo *pFootGfxInfo ) = 0;
	virtual bool UpdateFoot( const int nVSOID ) = 0;
	virtual void RemoveFoot( const int nVSOID ) = 0;
	virtual void RemoveAllFoots() = 0;
	// river
	virtual void AddRiver( const SRiverGFXInfo *pRiverGfxInfo ) = 0;
	virtual bool UpdateRiver( const int nVSOID ) = 0;
	virtual void RemoveRiver( const int nVSOID ) = 0;
	virtual void RemoveAllRivers() = 0;
	// terraspot
	virtual void AddTerraSpot( STerraSpotGFXInfo *pTerraSpotGfxInfo ) = 0;
	virtual bool UpdateTerraSpot( const int nVSOID ) = 0;
	virtual void RemoveTerraSpot( const int nVSOID ) = 0;
	virtual void RemoveAllTerraSpots() = 0;
	// precipices
	virtual void AddPrecipice( SPrecipiceGFXInfo *pPrecipiceGfxInfo ) = 0;
	virtual bool UpdatePrecipice( const int nVSOID ) = 0;
	virtual void RemovePrecipice( const int nVSOID ) = 0;
	virtual void RemoveAllPrecipices() = 0;
};

namespace NScene
{
	extern const string SZ_TERRA_BIN_FILE_NAME;
	//
	void CreateTerrain( ITerraManager *pTerraManager, const NDb::STerrain *pDesc, const string &szMapFilePath );
	SCENEB2_EXPORT bool LoadTerrain( ITerraManager *pTerraManager, const NDb::STerrain *pDesc, const string &szMapFilePath );
	bool SaveTerrain( ITerraManager *pTerraManager, const string &szMapFilePath );
};

#define DEF_DEBRIS_SMOOTH_RADIUS 20


