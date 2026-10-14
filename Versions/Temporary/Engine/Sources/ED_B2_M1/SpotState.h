#pragma once

#include "B2_M1_Terrain/DBTerrainSpot.h"
#include "MapObjectState.h"

#include <fmt/format.h>

class CSpotState : public CMapObjectState
{
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CMapObjectMultiState;

	NDb::STerrainSpotInstance terrainSpotInstance;

	void ClearData()
	{
		terrainSpotInstance.nSpotID = INVALID_NODE_ID;
		terrainSpotInstance.pDescriptor = 0;
		terrainSpotInstance.points.clear();
	}

	//конструкторы и операторы присваивания
	CSpotState( CMapObjectMultiState *_pParentState = 0 )
		: CMapObjectState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, fmt::format( "CSpotState(): pParentState == 0" ) );
	}

	//
	bool CanAddSpot();
	void ModifySpotSquare( NDb::STerrainSpotInstance *pTerrainSpotInstance, const CVec3 &rvPosition, float fDirection );
	void CreateSpotPointsList( std::list<CVec3> *pPointList, const NDb::STerrainSpotInstance &rTerrainSpotInstance );

protected:
	// CMapObjectState
	bool IsDrawSceneDrawTool() override
	{
		return true;
	}

	//
	void InsertObjectEnter() override;
	void InsertObjectLeave() override;
	void InsertObjectDraw( IPaintContext *pPaintDC ) override;
	//
	bool InsertObjectMouseMove( unsigned nFlags, const CVec3 &rTerrainPos ) override;
	bool InsertObjectLButtonUp( unsigned nFlags, const CVec3 &rTerrainPos ) override;
	bool InsertObjectRButtonUp( unsigned nFlags, const CVec3 &rTerrainPos ) override;
	bool InsertObjectKeyDown( unsigned nChar, unsigned nFlags, const CVec3 &rTerrainPos ) override;
};
