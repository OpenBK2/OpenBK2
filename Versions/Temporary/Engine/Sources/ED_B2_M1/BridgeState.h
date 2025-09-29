#if !defined(__BRIDGE_STATE__)
#define __BRIDGE_STATE__

#pragma once
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "..\sceneb2\scene.h"
#include "BridgeInfoData.h"
#include "MapObjectState.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // !defined(__BRIDGE_STATE__)

