#pragma once

#include "../misc/2darray.h"
#include "../stats_b2_m1/iconsset.h"
#include "../sceneb2/scene.h"
#include "SimpleObjectState.h"

#include <zconf.h>

//
//
//				FENCE DESIGN TOOL
//
//

class CFenceDesignTool
{
	bool bEnabled;			// готов к работе
	bool bComplete;			// больше ничего менять нельзя - можно генерить забор
	CVec3 vRayOrigin;		// точка начала куска забора
	float fRayDir;			// направление куска
	bool bFitToAIGrid;	// привязывать к сетке
	int nNumSections;		// текущее число число секций 
	float fSectionLen;	// длина одной секции

	CVec3 GetFencePlace( const CVec3 &p );	// ищет центр ближайшего к p тайла (есди bFitToAIGrd == false тогда просто возвращает p)
	void Complete();												// завершить текущий кусок

public:
	bool bRay;					// зафиксирована точка начала куска забора или нет
	CVec3 vLastPos;

	CFenceDesignTool() : bEnabled( false ) {} 
	//
	void SetUp( bool bFitToAIGrd, const NDb::SFenceRPGStats *pStats );
	void Clear();
	//
	bool ProcessLClickPoint( const CVec3 &p );
	bool ProcessRClickPoint( const CVec3 &p );
	bool ProcessEscape();
	bool ProcessMovePoint( const CVec3 &p );
	//
	bool IsComplete();
	float GetCurSectionLen() { return fSectionLen; }
	///
	struct SFenceSectionInfo
	{
		CVec3 vPosition;
		float fDirection;
	};
	void GetSectionsInfo( vector<SFenceSectionInfo> *pSectionInfo, NDb::SFenceRPGStats::EFencePlacementMode ePlacementMode ); 
	///
};


//
//
//	FENCE STATE
//
//

class CFenceState : public CMapObjectState
{
	friend class CMultiInputState;
	friend class CMapInfoState;
	//
	NMapInfoEditor::CSceneIDList tmpSceneObjectsIDList;
	CFenceDesignTool designTool;
	//
	void InsertFence();
	void ClearData();
	//
	// выбранный для постановки забор
	struct SSelectedFenceInfo
	{ 
		string szRPGStatsTypeName;
		CDBID rpgStatsDBID;
		const NDb::SFenceRPGStats *pFenceRPGStats;
		SSelectedFenceInfo() : pFenceRPGStats( 0 ) {}
	};	
	SSelectedFenceInfo selectedFenceInfo;
	void RefreshSelectedFenceInfo();
	//
	int InsertFenceSection( NMapInfoEditor::SObjectCreateInfo *pSectionCreateInfo, IEditorScene *pEditorScene, IManipulator *pManipulator, CObjectBaseController *pObjectController );
	//
	void ClearScene();
	void FillScene( UINT nFlags, const CVec3 &rTerrainPos );
	//
public:
	CFenceState( CMapObjectMultiState* _pParentState = 0 ) : CMapObjectState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, StrFmt( "CFenceState(): pParentState == 0" ) );
		ClearData();
	}
	//
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
	//
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	//
};


