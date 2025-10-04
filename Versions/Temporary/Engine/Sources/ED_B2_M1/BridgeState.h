
#pragma once
#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "SceneB2/Scene.h"
#include "BridgeInfoData.h"
#include "MapObjectState.h"

#include <zconf.h>

class CBridgeState : public CMapObjectState
{
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CMapObjectMultiState;
	
	static const int START_TERMINATOR_INDEX;
	static const int FINISH_TERMINATOR_INDEX;
	static const int TERMINATOR_COUNT;
	//
	SObjectSet objectSet;
	NMapInfoEditor::CSceneIDList sceneIDlist;
	CVec3 vStart;
	CVec3 vEnd;
	NMapInfoEditor::CBridgeCenterPointList bridgeElementCenterPointList;
	CVec3 vDirection;
	CVec2 vEndSize;
	CVec2 vCenterSize;
	bool bPlaceBridge;
	//
	void ClearScene();
	void ClearData();
	//
	void InsertBridge( NMapInfoEditor::SBridgeInfo::EDirection direction, bool bFixStartPoint, bool bPlace );

	//конструкторы и операторы присваивания
	CBridgeState( CMapObjectMultiState* _pParentState = 0 ) : CMapObjectState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, StrFmt( "CBridgeState(): pParentState == 0" ) );
		ClearData();
	}
	//
	bool CanAddBridge();
protected:
	// CMapObjectState
	bool IsDrawSceneDrawTool() { return true; }
	//
	void InsertObjectEnter();
	void InsertObjectLeave();
	void InsertObjectDraw( class CPaintDC *pPaintDC );
	//	
	bool InsertObjectMouseMove( UINT nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectLButtonDown( UINT nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectRButtonUp( UINT nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectKeyDown( UINT nChar, UINT nFlags, const CVec3 &rTerrainPos );
};



