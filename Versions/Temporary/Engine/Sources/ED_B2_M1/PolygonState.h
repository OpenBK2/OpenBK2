#pragma once

#include "Tools_SceneDraw.h"
#include "MapEditorLib/MultiInputState.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapInfoStoreInputState.h"
#include "libdb/Manipulator.h"

#include <cstdint>

class CPolygonSelectState : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CPolygonState;
	
	class CPolygonState* pParentState;
	
	//конструкторы и операторы присваивания
	CPolygonSelectState() : pParentState( 0 )
  {
		NI_ASSERT( pParentState != 0, "CPolygonSelectState(): Invalid parameter: pParentState == 0" );
	}
	CPolygonSelectState( CPolygonState* _pParentState ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CPolygonSelectState(): Invalid parameter: pParentState == 0" );
	}

	//IInputState interface
	void OnLButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
};


class CPolygonEditState : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CPolygonState;
	
	class CPolygonState* pParentState;
	
	//конструкторы и операторы присваивания
	CPolygonEditState() : pParentState( 0 )
  {
		NI_ASSERT( pParentState != 0, "CPolygonEditState(): Invalid parameter: pParentState == 0" );
	}
	CPolygonEditState( CPolygonState* _pParentState ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CPolygonEditState(): Invalid parameter: pParentState == 0" );
	}

	//IInputState interface
	void OnMouseMove			( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnKeyDown				( UINT nChar, UINT nRepCnt, UINT nFlags );
};


class CPolygonAddState : public CDefaultInputState
{
	friend class CMultiInputState;
	friend class CPolygonState;
	
	class CPolygonState* pParentState;
	
	//конструкторы и операторы присваивания
	CPolygonAddState() : pParentState( 0 )
  {
		NI_ASSERT( pParentState != 0, "CPolygonAddState(): Invalid parameter: pParentState == 0" );
	}
	CPolygonAddState( CPolygonState* _pParentState ) : pParentState( _pParentState )
	{
		NI_ASSERT( pParentState != 0, "CPolygonAddState(): Invalid parameter: pParentState == 0" );
	}

	void InsertPolygon();

	//IInputState interface
	void OnMouseMove			( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnKeyDown				( UINT nChar, UINT nRepCnt, UINT nFlags );
};


class CPolygonState : public CMultiInputState, public ICommandHandler
{
	friend class CPolygonStoreInputState;
	friend class CPolygonSelectState;
	friend class CPolygonEditState;
	friend class CPolygonAddState;
	//
	enum EInputStates
	{
		IS_SELECT	= 0,
		IS_EDIT		= 1,
		IS_ADD		= 2,
	};

protected:
	static const float CONTROL_POINT_RADIUS;
	static const int CONTROL_POINT_PARTS;
	static const float CENTER_POINT_RADIUS;
	static const uint32_t CONTROL_POINT_COLOR;
	static const uint32_t CONTROL_LINE_COLOR;
	//
	typedef vector<UINT> CPolygonIDList;
	typedef vector<CVec3> CControlPointList;

public:
	enum EUpdateType
	{
		UT_START								= 0,
		UT_START_MOVE						= 1,
		UT_CONTINUE_MOVE				= 2,
		UT_FINISH_MOVE					= 3,
		UT_CHANGE_POINT_NUMBER	= 4,
		UT_FINISH								= 5,
		UT_CANCEL								= 6,
	};
	//
	enum EMoveType
	{
		MT_SINGLE = 0,
		MT_MULTI = 1,
		MT_ALL = 2,
	};
	//
protected:
	// Для перекрытия метода Draw
	// для работы SelectState
	// для работы AddState
	// для работы EditState
	CPolygonIDList pickPolygonIDList;							// список ID выделенных полигонов
	CControlPointList controlPointList;						// контрольные точки полигона при добавлении,
																								// сохраняем начальное положение контрольных точек при редактировании
	int nSelectedIndex;														// текущий выделенный полигон (индекс его ID в массиве выделенных полигонов) 
	int nSelectedControlPoint;										// текущая выделенная точка в полигоне
	CMapInfoStoreInputState *pStoreInputState;
	CSceneDrawTool sceneDrawTool;

	// Вынесены в этот раздел для успешной работы класов - наследников
	inline void ValidateSelectedIndex()
	{
		if ( ( nSelectedIndex < 0 ) || ( nSelectedIndex >= pickPolygonIDList.size() ) )
		{
			nSelectedIndex = 0;
		}
	}
	inline int GetSelectedPolygonID()
	{ 
		if ( ( nSelectedIndex < 0 ) || ( nSelectedIndex >= pickPolygonIDList.size() ) )
		{
			return INVALID_NODE_ID;
		}
		else
		{
			return pickPolygonIDList[nSelectedIndex];
		}
	}

	void OnDelete();

	//IInputState interface
	virtual void Enter();
	virtual void Leave();
	virtual void Draw( class CPaintDC *pPaintDC );
	virtual void OnSetFocus( class CWnd* pNewWnd );

	//virtual void OnSetFocus( class CWnd* pNewWnd );
	// CPolygonState
	// Сразу ли обновлять полигон после постановки или вклучать фазу редактирования
	virtual bool SkipEnterAfterInsert() = 0;
	// Возможно какое либо редактирование
	virtual bool CanEdit() = 0;
	// Можно ли создать новый полигон
	virtual bool CanInsertPolygon() = 0;
	// Замкнут полигон или не замкнут
	virtual bool IsClosedPolygon() = 0;
	// Нарисовать draw tool или добавить к ним чеще чего-нибудь и потом нарисовать
	virtual bool IsDrawSceneDrawTool() = 0;
	// Получить способ редактирования
	virtual EMoveType GetMoveType() = 0;
	// Получить минимальное и максимальное количество control points
	virtual void GetBounds( int *pnMinCount, int *pnMaxCount ) = 0;
	// Получить контрольные точки указанного полигона
	virtual CControlPointList* GetControlPoints( int nPolygonID ) = 0;
	// Обработать массив контрольных точек перед добавлением полигона
	virtual bool PrepareControlPoints( CControlPointList *pControlPointList ) = 0;
	// По точке получить выделенные полигоны (набор их ID)
	virtual void PickPolygon( const CVec3 &rvPos, CPolygonIDList *pPickPolygonIDList ) = 0;
	// Обновить полигон (были изменены его контрольгные точки)
	virtual void UpdatePolygon( int nPolygonID, EUpdateType eEpdateType ) = 0;
	// Добавить полигон (возвращает ID добавленного полигона)
	virtual UINT InsertPolygon( const CControlPointList &rControlPointList ) = 0;
	// Удалить полигон по указанному ID
	virtual void RemovePolygon( int nPolygonID ) = 0;
public:
	//
	//конструкторы и операторы присваивания
	CPolygonState() : nSelectedIndex( INVALID_NODE_ID ), nSelectedControlPoint( INVALID_NODE_ID )
	{
		pStoreInputState = new CMapInfoStoreInputState();
		NI_ASSERT( pStoreInputState != 0, StrFmt( "CPolygonState(): pStoreInputState == 0" ) );
		
		int nStateIndex = INVALID_INPUT_STATE_INDEX;
		
		CPolygonSelectState *pPolygonSelectState = new CPolygonSelectState( this );
		nStateIndex = AddInputState( pPolygonSelectState );
		NI_ASSERT( nStateIndex == IS_SELECT, StrFmt( "CPolygonState(): Wrong state number: %d (%d)", nStateIndex, IS_SELECT ) );

		CPolygonEditState *pPolygonEditState = new CPolygonEditState( this );
		nStateIndex = AddInputState( pPolygonEditState );
		NI_ASSERT( nStateIndex == IS_EDIT, StrFmt( "CPolygonState(): Wrong state number: %d, (%d)", nStateIndex, IS_EDIT ) );
		
		CPolygonAddState *pPolygonAddState = new CPolygonAddState( this );
		nStateIndex = AddInputState( pPolygonAddState );
		NI_ASSERT( nStateIndex == IS_ADD, StrFmt( "CPolygonState(): Wrong state number: %d, (%d)", nStateIndex, IS_ADD ) );

		SetActiveInputState( IS_SELECT, true, false );
	}
	//
	~CPolygonState()
	{
		if ( pStoreInputState )
		{
			delete pStoreInputState;
		}
		pStoreInputState = 0;
	}

	//ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, uint32_t dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
};



