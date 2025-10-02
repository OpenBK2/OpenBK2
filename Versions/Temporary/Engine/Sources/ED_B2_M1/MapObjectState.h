#pragma once

#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/iconsset.h"
#include "../libdb/manipulator.h"
#include "../MapEditorLib/MultiInputState.h"
#include "MapInfoStoreInputState.h"
#include "Tools_SceneDraw.h"
#include "EditorScene.h"
#include "../MapEditorLib/Interface_CommandHandler.h"

namespace NMapInfoEditor
{
	struct SObjectInfoCollector;
}
class CMapInfoEditor;


class CMapObjectSelectState : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CMapObjectState;
	
	class CMapObjectState* pParentState;

	//конструкторы и операторы присваивания
	CMapObjectSelectState() : pParentState( 0 )
  {
		NI_ASSERT( pParentState != 0, "CMapObjectSelectState(): Invalid parameter: pParentState == 0" );
	}
	CMapObjectSelectState( CMapObjectState* _pParentState ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CMapObjectSelectState(): Invalid parameter: pParentState == 0" );
	}

	void OnMouseButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint, UINT nButtonType );
	void OnMouseButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint, UINT nButtonType );

	void UpdateSelectionBySelector( bool bShiftPressed );
	//IInputState interface
	void OnMouseMove		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDown	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonUp		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonDown	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnKeyDown			( UINT nChar, UINT nRepCnt, UINT nFlags );
};


class CMapObjectEditState : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CMapObjectState;
	
	class CMapObjectState* pParentState;

	//конструкторы и операторы присваивания
	CMapObjectEditState() : pParentState( 0 )
  {
		NI_ASSERT( pParentState != 0, "CMapObjectEditState(): Invalid parameter: pParentState == 0" );
	}
	CMapObjectEditState( CMapObjectState* _pParentState ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CMapObjectEditState(): Invalid parameter: pParentState == 0" );
	}

	void RecalculateSelection( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OperateSelection( UINT nFlags, const CTPoint<int> &rMousePoint, bool bSave );

	//IInputState interface
	void OnMouseMove		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDown	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonUp		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonDown	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnKeyDown			( UINT nChar, UINT nRepCnt, UINT nFlags );
};


class CMapObjectAddState : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CMapObjectState;
	
	class CMapObjectState* pParentState;

	//конструкторы и операторы присваивания
	CMapObjectAddState() : pParentState( 0 )
  {
		NI_ASSERT( pParentState != 0, "CMapObjectAddState(): Invalid parameter: pParentState == 0" );
	}
	CMapObjectAddState( CMapObjectState* _pParentState ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CMapObjectAddState(): Invalid parameter: pParentState == 0" );
	}

	//IInputState interface
	void OnSetFocus				( class CWnd* pNewWnd );
	void OnKillFocus			( class CWnd* pOldWnd );
	//
	void OnMouseMove			( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	void OnLButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	void OnKeyDown				( UINT nChar, UINT nRepCnt, UINT nFlags );
	void OnKeyUp					( UINT nChar, UINT nRepCnt, UINT nFlags );
};


class CMapObjectPasteState : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CMapObjectState;
	
	class CMapObjectState* pParentState;

	//конструкторы и операторы присваивания
	CMapObjectPasteState() : pParentState( 0 )
  {
		NI_ASSERT( pParentState != 0, "CMapObjectPasteState(): Invalid parameter: pParentState == 0" );
	}
	CMapObjectPasteState( CMapObjectState* _pParentState ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CMapObjectPasteState(): Invalid parameter: pParentState == 0" );
	}
	
	//IInputState interface
	void Enter();
	void Leave();

	void OnMouseMove		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonUp		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnKeyDown			( UINT nChar, UINT nRepCnt, UINT nFlags );
};


class CMapObjectState : public CMultiInputState, public ICommandHandler
{
	friend class CMapObjectStoreInputState;
	friend class CMapObjectSelectState;
	friend class CMapObjectEditState;
	friend class CMapObjectAddState;
	friend class CMapObjectPasteState;
	//
protected:
	static const DWORD SELECTION_LINE_COLOR;

	enum EInputStates
	{
		IS_SELECT	= 0,
		IS_EDIT		= 1,
		IS_ADD		= 2,
		IS_PASTE	= 3,
	};

protected:
	class CMapObjectMultiState* pParentState;
	CMapInfoStoreInputState *pStoreInputState;

	struct SSelector
	{
		bool bTerrainSelector;
		CTRect<int> frameRect;
		CVec3 vTerrainPos0;
		CVec3 vTerrainPos1;
		bool bValid;
		//
		SSelector() 
			: bTerrainSelector( true ),
				frameRect( 0, 0, 0, 0 ),
				vTerrainPos0( VNULL3 ),
				vTerrainPos1( VNULL3 ),
				bValid( false ) {}
		inline void Clear()
		{
			bTerrainSelector = true;
			frameRect.minx = 0;
			frameRect.miny = 0;
			frameRect.maxx = 0;
			frameRect.maxy = 0;
			vTerrainPos0 = VNULL3;
			vTerrainPos1 = VNULL3;
			bValid = false;
		}
		inline bool IsValid() { return bValid; }
		inline bool IsTerrainSelector() { return bTerrainSelector; }
	};

	UINT nSelectedSceneID;
	SSelector selector;
	CSceneDrawTool sceneDrawTool;
	bool bDrawShootAreas;
	//
	void ClearSelection();
	int GetNextSceneID( int nSceneID, list<int> *pSceneIDList );

	//IInputState interface
	virtual void Enter();
	virtual void Leave();
	virtual void Draw( class CPaintDC *pPaintDC );
	virtual void PostDraw( class CPaintDC *pPaintDC );
	
	virtual void OnSetFocus( class CWnd* pNewWnd );

	// CMapObjectState
	// Возможно какое либо редактирование
	virtual bool CanEdit();
	// Нарисовать draw tool или добавить к ним чеще чего-нибудь и потом нарисовать ( в перекрытом методе Draw() необходимо отрисовать sceneDrawTool )
	virtual bool IsDrawSceneDrawTool() = 0;
	// Установить всоты объекта в ноль
	virtual void SetSelectionHeightsToZero( bool bSave );
	// Установить линк
	virtual void InsertSelectionLink( UINT nLinkToSceneID );
	// Удалить линки
	virtual void RemoveSelectionLinks();
	// Удалить линк
	virtual void RemoveSelectionLinkTo();
	// Удалить объект
	virtual void RemoveSelection();
	// Переместить объект, используя указаную точку на карте
	virtual void MoveSelection( const CVec3 &rvPosition, bool bExactPosition, bool bIgnoreDifference, bool bSave ); 
	// Повернуть объект, используя указаную точку на карте
	virtual void RotateSelection( const CVec3 &rvPosition, bool bExactDirection, bool bIgnoreDifference, bool bSave ); 
	// Вызывается из стейта добавления объекта ( если вернули true, переключится в IS_SELECT и автоматически обновится сцена ) 
	virtual void InsertObjectEnter					() {}
	virtual void InsertObjectLeave					() {}
	virtual void InsertObjectDraw						( class CPaintDC *pPaintDC ) {}
	//
	virtual bool InsertObjectSetFocus				( class CWnd* pNewWnd ) { return false; }
	virtual bool InsertObjectKillFocus			( class CWnd* pOldWnd ) { return false; }
	//
	virtual bool InsertObjectMouseMove			( UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectLButtonDown		( UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectLButtonUp			( UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectLButtonDblClk	( UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectRButtonDown		( UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectRButtonUp			( UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectRButtonDblClk	( UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectMButtonDown		( UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectMButtonUp			( UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectMButtonDblClk	( UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectKeyDown				( UINT nChar, UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	virtual bool InsertObjectKeyUp					( UINT nChar, UINT nFlags, const CVec3 &rTerrainPos ) { return false; }
	//
	//virtual void DrawShootAreas() {}
public:
	//
	//конструкторы и операторы присваивания
	CMapObjectState( class CMapObjectMultiState *_pParentState = 0 ) : pParentState( _pParentState ), nSelectedSceneID( INVALID_NODE_ID ), bDrawShootAreas( true )
	{
		pStoreInputState = new CMapInfoStoreInputState();
		NI_ASSERT( pStoreInputState != 0, StrFmt( "CMapObjectState(): pStoreInputState == 0" ) );
		
		int nStateIndex = INVALID_INPUT_STATE_INDEX;
		
		CMapObjectSelectState *pMapObjectSelectState = new CMapObjectSelectState( this );
		nStateIndex = AddInputState( pMapObjectSelectState );
		NI_ASSERT( nStateIndex == IS_SELECT, StrFmt( "CMapObjectState(): Wrong state number: %d (%d)", nStateIndex, IS_SELECT ) );

		CMapObjectEditState *pMapObjectEditState = new CMapObjectEditState( this );
		nStateIndex = AddInputState( pMapObjectEditState );
		NI_ASSERT( nStateIndex == IS_EDIT, StrFmt( "CMapObjectState(): Wrong state number: %d, (%d)", nStateIndex, IS_EDIT ) );
		
		CMapObjectAddState *pMapObjectAddState = new CMapObjectAddState( this );
		nStateIndex = AddInputState( pMapObjectAddState );
		NI_ASSERT( nStateIndex == IS_ADD, StrFmt( "CMapObjectState(): Wrong state number: %d, (%d)", nStateIndex, IS_ADD ) );

		CMapObjectPasteState *pMapObjectPasteState = new CMapObjectPasteState( this );
		nStateIndex = AddInputState( pMapObjectPasteState );
		NI_ASSERT( nStateIndex == IS_PASTE, StrFmt( "CMapObjectState(): Wrong state number: %d (%d)", nStateIndex, IS_PASTE ) );

		SetActiveInputState( IS_SELECT, true, false );
	}
	//
	~CMapObjectState()
	{
		if ( pStoreInputState )
		{
			delete pStoreInputState;
		}
		pStoreInputState = 0;
	}

	inline class CMapObjectMultiState* GetParentState() { return pParentState; }

	void SwitchToAddState();
	//
	virtual NMapInfoEditor::SObjectInfoCollector* GetObjectInfoCollector();
	virtual CMapInfoEditor* GetMapInfoEditor();

	// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
};



