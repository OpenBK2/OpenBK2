#pragma once

#include <cstdint>

/**
#include "MapInfoEditorData.h"
#include "BridgeInfoData.h"
#include "EntrenchmentInfoData.h"

struct SClipboardObjectInfo : public NMapInfoEditor::SObjectCreateInfo
{
	std::string szName;
};


struct SClipboardEntrenchment : public NMapInfoEditor::SEntrenchmentCreateInfo
{
};


struct SClipboardBridge : public NMapInfoEditor::SBridgeCreateInfo
{
	CVec2 vCenterSize;
	CVec2 vEndSize;
};


struct SClipboardVSO
{
	std::string szType;
	int nTypeID;
	int nDescID;
	NDb::SVSOInstance vsoInstance;
	///
	SClipboardVSO() :
		nTypeID( -1 ),
		nDescID( -1 )
	{
	}
};


struct SClipboardTerraSpot
{
	std::string szType;
	int nTypeID;
	int nDescID;
	NDb::STerrainSpotInstance spotInstance;
};


class CMapClip
{
	std::vector<CVec3> pasteRegion;
	//
	CArray2D<float> heights;
	CArray2D<uint8_t> tilesType;
	//
	std::vector<SClipboardVSO> vsoArray;
	//
	std::vector<SClipboardEntrenchment> entrenchments;
	std::vector<SClipboardBridge> bridges;
	std::vector<SClipboardTerraSpot> spots;
	//
	std::vector<SClipboardObjectInfo> clipBuffer;
	//	

public:
	NMapInfoEditor::SMapInfo *pMapInfo;
	//
	CMapClip()
	{
		Clear();
		pMapInfo = 0;
	}
	//
	void Clear()
	{
		clipBuffer.clear();
		pasteRegion.clear();
		heights.Clear();
		vsoArray.clear();
		entrenchments.clear();
		bridges.clear();
		tilesType.Clear();
		spots.clear();
	}
	//
	void SetRegion( const CVec3 &vCenter, const std::vector<CVec3> &controlPoints )
	{
		if ( controlPoints.empty() )
			return;
		CVec3 vCurPoint = (controlPoints.front() - vCenter);
		pasteRegion.push_back( vCurPoint );
		for ( int i = 1; i < controlPoints.size(); ++i )
		{
			CVec3 p = controlPoints[i] - vCenter;
			if ( fabs(p-vCurPoint) > 0.01 )
			{
				pasteRegion.push_back( p );
			}
			vCurPoint = p;
		}
	}
	//
	void AddObj( SClipboardObjectInfo &oi )
	{
		clipBuffer.push_back( oi );
	}
	//
	int GetObjNum() const
	{
		return clipBuffer.size();
	}
	//
	const SClipboardObjectInfo* GetObj( int nIdx ) const
	{
		if ( nIdx < 0 || nIdx >= clipBuffer.size() )
			return 0;
		return &clipBuffer[nIdx];
	}
	//
	void GetRegion( std::vector<CVec3> *pPolyline, const CVec3 &vCenter ) const
	{
		if ( !pPolyline )
			return;
		pPolyline->clear();
		for ( int i = 0; i < pasteRegion.size(); ++i )
		{
			CVec3 p = pasteRegion[i] + vCenter;
			pPolyline->push_back( p );
		}
	}
	//
	const std::vector<CVec3>& GetRegion() const
	{
		return pasteRegion;
	}
	//
	void SetHeightsDimension( int nDimX, int nDimY )
	{
		heights.SetSizes( nDimX, nDimY );
	}
	//
	void SetHeight( int nX, int nY, float fZ )
	{
		if ( nX < 0 || nX >= heights.GetSizeX() )
			return;
		if ( nY < 0 || nY >= heights.GetSizeY() )
			return;
		//
		heights[nY][nX] = fZ;
	}
	//
	float GetHeight( int nX, int nY )
	{
		if ( nX < 0 || nX >= heights.GetSizeX() )
			return 0;
		if ( nY < 0 || nY >= heights.GetSizeY() )
			return 0;
		//
		return heights[nY][nX];
	}
	//
	void AddVso( const NDb::SVSOInstance &vsoInstance, const std::string &szType, int nTypeID, int nDescID )
	{
		SClipboardVSO vso;
		vso.vsoInstance = vsoInstance;
		vso.szType = szType;
		vso.nTypeID = nTypeID;
		vso.nDescID = nDescID;
		vsoArray.push_back( vso );
	}
	//
	int GetVSONum() const
	{
		return vsoArray.size();
	}
	//
	const SClipboardVSO* GetVSO( int nIdx ) const
	{
		if ( nIdx < 0 || nIdx >= vsoArray.size() )
		{
			return 0;
		}
		return &vsoArray[ nIdx ];
	}
	//
	bool IsEmpty()
	{
		return !(( pasteRegion.size() >= 3 ) &&
			( !clipBuffer.empty() || !heights.IsEmpty() || 
			!vsoArray.empty() || !entrenchments.empty() ||
			!bridges.empty() || !tilesType.IsEmpty() || !spots.empty() ));
	}
	//
	void AddEntrenchment( const SClipboardEntrenchment &trench )
	{
		entrenchments.push_back( trench );
	}
	//
	int GetEntrenchmentNum() const
	{
		return entrenchments.size();
	}
	//
	const SClipboardEntrenchment* GetEntrenchment( int nIdx ) const
	{
		if ( nIdx < 0 || nIdx >= entrenchments.size() )
			return 0;
		return &entrenchments[nIdx];
	}
	//
	void AddBridge( const SClipboardBridge &bridge )
	{
		bridges.push_back( bridge );
	}
	//
	int GetBridgesNum()
	{
		return bridges.size();
	}
	//
	const SClipboardBridge* GetBridge( int nIdx ) const
	{
		if ( nIdx < 0 || nIdx >= bridges.size() )
			return 0;
		return &bridges[nIdx];
	}
	//
	void SetTilesTypeDimension( int nDimX, int nDimY )
	{
		tilesType.SetSizes( nDimX, nDimY );
	}
	//
	void SetTileType( int nX, int nY, char val )
	{
		if ( nX < 0 || nX >= tilesType.GetSizeX() )
			return;
		if ( nY < 0 || nY >= tilesType.GetSizeY() )
			return;
		//
		tilesType[nY][nX] = val;
	}
	//
	char GetTileType( int nX, int nY )
	{
		if ( nX < 0 || nX >= tilesType.GetSizeX() )
			return 0;
		if ( nY < 0 || nY >= tilesType.GetSizeY() )
			return 0;
		//
		return tilesType[nY][nX];
	}
	//
	int GetSpotsNum() const
	{
		return spots.size();
	}
	//
	void AddSpot( const SClipboardTerraSpot &spot )
	{
		spots.push_back( spot );
	}
	//
	const SClipboardTerraSpot* GetSpot( int nIdx ) const
	{
		if ( nIdx < 0 || nIdx >= spots.size() )
			return 0;
		return &spots[nIdx];
	}
	//
	bool SaveMapClipToDB( CObjectBaseController *pObjectController ) const;
	bool LoadMapClipFromDB( int nMapClipID );
	bool SavePasteRegionToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const;
	bool LoadPasteRegionFromDB( IManipulator *pManipulator );
	bool SaveHeightsToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const;
	bool LoadHeightsFromDB( IManipulator *pManipulator );
	bool SaveTilesTypeToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const;
	bool LoadTilesTypeFromDB( IManipulator *pManipulator );
	bool SaveVSOToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const;
	bool LoadVSOFromDB( IManipulator *pManipulator );
	bool SaveEntrenchmentsToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const;
	bool LoadEntrenchmentsFromDB( IManipulator *pManipulator );
	bool SaveBridgesToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const;
	bool LoadBridgesFromDB( IManipulator *pManipulator );
	bool SaveSpotsToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const;
	bool LoadSpotsFromDB( IManipulator *pManipulator );
	bool SaveObjectsToDB(  CObjectBaseController *pObjectController, IManipulator *pManipulator ) const;
	bool LoadObjectsFromDB( IManipulator *pManipulator );
};

/**/

