#pragma once

#include "GameX/InterfaceChapterMapMenuHelper.h"
#include "MapEditorLib/DefaultInputState.h"

class CChapterEditor;
struct IScreen;
struct IWindow;
struct IPotentialLines;
class CMaskManipulator;
struct IView;
struct IButton;
struct ITextView;

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
	IView *ClearView();
	void SetView( IView *pView );
	void OnKeyArrows( int nDeltaX, int nDeltaY );

public:
	CChapterState( CChapterEditor *pChapterEditor );

	void UpdateView();

	//IInputState
	void Enter() override;
	void Leave() override;
	void PostDraw( IPaintContext *pPaintDC ) override;
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override;
};
