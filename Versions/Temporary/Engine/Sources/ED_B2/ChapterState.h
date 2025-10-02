#pragma once

#include "../MapEditorLib/DefaultInputState.h"
#include "../GameX/InterfaceChapterMapMenuHelper.h"

class CChapterEditor;
interface IScreen;
interface IWindow;
interface IPotentialLines;
class CMaskManipulator;
interface IView;
interface IButton;
interface ITextView;
namespace NDb
{
	struct SWindowMSButton;
}

class CChapterState : public CDefaultInputState
{
	//Данные специфичные для данного редактрора
	CPtr<IScreen> pScreen;
	CPtr<IButton> pMissionBtn;
	CPtr<IButton> pBigMissionBtn;
	CObj<SChapterMapMenuHelper> pHelper;
	SChapterMapMenuHelper::SMission *pPickedMission;
	bool bDragging;
	CVec2 vStartPos;
	CVec2 vPrevPos;
	CPtr<IPotentialLines> pFrontLines;
	CVec2 vChapterMapSize;
	SChapterMapMenuHelper::SArrow *pPickedArrow;
	CObj<CMaskManipulator> pMaskManipulator;
	// Данные общего назначения 
	CChapterEditor *pChapterEditor;
	CPtr<ITextView> pNumberView;
	CPtr<ITextView> pBigNumberView;
	CDBPtr<NDb::SChapter> pChapter;
private:
	void LoadChapterMap();
	void CreateScreen();
	bool MoveMission( SChapterMapMenuHelper::SMission *pMission, const CVec2 &vPos, bool bSave );
	bool MoveArrow( SChapterMapMenuHelper::SArrow *pArrow, const CVec2 &vPos, bool bSave );
	void UpdateFrontLines();
	void SetMaskManipulatorMission( SChapterMapMenuHelper::SMission *pMission );
	void SetMaskManipulatorArrow( SChapterMapMenuHelper::SArrow *pArrow );
	void ClearMaskManipulator();
	void UpdateMaskManipulator();
	IView* ClearView();
	void SetView( IView *pView );
	void OnKeyArrows( int nDeltaX, int nDeltaY );
public:
	CChapterState( CChapterEditor *pChapterEditor );
	
	void UpdateView();
	
	//IInputState
	void Enter();
	void Leave();
	void PostDraw( class CPaintDC *pPaintDC );
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
};


