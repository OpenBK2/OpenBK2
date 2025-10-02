#if !defined(__SPOT_STATE__)
#define __SPOT_STATE__
#pragma once

#include "MapObjectState.h"
#include "..\B2_M1_Terrain\DBTerrainSpot.h"


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
	CSpotState( CMapObjectMultiState* _pParentState = 0 ) : CMapObjectState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, StrFmt( "CSpotState(): pParentState == 0" ) );
	}
	//
	bool CanAddSpot();
	void ModifySpotSquare( NDb::STerrainSpotInstance *pTerrainSpotInstance, const CVec3 &rvPosition, float fDirection );
	void CreateSpotPointsList( list<CVec3> *pPointList, const NDb::STerrainSpotInstance &rTerrainSpotInstance );

protected:
	// CMapObjectState
	bool IsDrawSceneDrawTool() { return true; }
	//
	void InsertObjectEnter();
	void InsertObjectLeave();
	void InsertObjectDraw( class CPaintDC *pPaintDC );
	//	
	bool InsertObjectMouseMove( UINT nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectLButtonUp( UINT nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectRButtonUp( UINT nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectKeyDown( UINT nChar, UINT nFlags, const CVec3 &rTerrainPos );
};

#endif // !defined(__SPOT_STATE__)


