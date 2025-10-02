#if !defined(__SIMPLE_OBJECT_STATE__)
#define __SIMPLE_OBJECT_STATE__
#pragma once

#include "MapObjectState.h"


class CSimpleObjectState : public CMapObjectState
{
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CMapObjectMultiState;

	struct SSceneObject
	{
		UINT nID;
		CVec3 vPosition;
		float fDirection;
	};
	typedef list<SSceneObject> CSceneObjectList;
	CSceneObjectList sceneObjectlist;
	string szRPGStatsTypeName;
	CDBID rpgStatsDBID;
	//
	void ClearData();

	//конструкторы и операторы присваивания
	CSimpleObjectState( CMapObjectMultiState* _pParentState = 0 ) : CMapObjectState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, StrFmt( "CSimpleObjectState(): pParentState == 0" ) );
		ClearData();
	}
	//
	bool CanAddSimpleObject();
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

#endif // !defined(__SIMPLE_OBJECT_STATE__)

