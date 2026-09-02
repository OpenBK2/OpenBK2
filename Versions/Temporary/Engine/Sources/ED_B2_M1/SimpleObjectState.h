#pragma once

#include "MapObjectState.h"


class CSimpleObjectState : public CMapObjectState
{
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CMapObjectMultiState;

	struct SSceneObject
	{
		unsigned nID;
		CVec3 vPosition;
		float fDirection;
	};
	typedef std::list<SSceneObject> CSceneObjectList;
	CSceneObjectList sceneObjectlist;
	std::string szRPGStatsTypeName;
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
	bool InsertObjectMouseMove( unsigned nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectLButtonUp( unsigned nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectRButtonUp( unsigned nFlags, const CVec3 &rTerrainPos );
	bool InsertObjectKeyDown( unsigned nChar, unsigned nFlags, const CVec3 &rTerrainPos );
};



