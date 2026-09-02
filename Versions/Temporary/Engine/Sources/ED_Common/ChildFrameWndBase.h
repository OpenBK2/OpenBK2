#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

#include "ED_Common_export.h"

class ED_COMMON_EXPORT CChildFrameWndBase : public CWnd, public ICommandHandler
{
	static const int DEFAULT_REFRESH_RATE;
	
	bool bInputEnabled;
	bool bRunModeEnabled;
	bool bGameInputEnabled;
	bool bRenderEnabled;
	bool bShowStatistic;
	bool bShowMovieBorders;
	bool bEnableSceneUpdate;
	bool bEnableScroll;
	//
	int32_t nUpdateSceneTimer;
	int32_t nUpdateSceneTimerInterval;

	bool bIsSettingUp;
	bool bWasResized;

	unsigned GetUpdateSceneTimerID() { return 1; }
	void SetUpdateSceneTimer();
	void KillUpdateSceneTimer();
	void OnUpdateSceneTimer();
	//
	void AlignWndAspect();
	//
	void RemoveInput(); 
	void EnableInput( uintptr_t dwData );
	void EnableAutoUpdate( uintptr_t dwData );
	void EnableGameInput( struct IInterfaceCommand *pInterfaceCommand );
	void DisableGameInput();
	void GetDimensions( uintptr_t dwData );
	void EnableRunMode( uintptr_t dwData );
	void EnableRender( uintptr_t dwData );
	void EnableScrollbars( uintptr_t dwData );
	void EnableMouseCapture( uintptr_t dwData );

protected:
	CRect rectBorder1, rectBorder2, rectWindow, rectMain;

	virtual BOOL PreCreateWindow( CREATESTRUCT &rCreateStruct );
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnDestroy();
	afx_msg void OnTimer( UINT_PTR nIDEvent );
	//
	afx_msg void OnSetFocus			( CWnd* pOldWnd );
	afx_msg void OnKillFocus		( CWnd* pNewWnd );

	afx_msg void OnMouseMove		( unsigned nFlags, CPoint point );
	afx_msg BOOL OnMouseWheel		( unsigned nFlags, short zDelta, CPoint point );
	//
	afx_msg void OnLButtonDown	( unsigned nFlags, CPoint point );
	afx_msg void OnLButtonUp		( unsigned nFlags, CPoint point );
	afx_msg void OnLButtonDblClk( unsigned nFlags, CPoint point );
	//
	afx_msg void OnRButtonDown	( unsigned nFlags, CPoint point );
	afx_msg void OnRButtonUp		( unsigned nFlags, CPoint point );
	afx_msg void OnRButtonDblClk( unsigned nFlags, CPoint point );
	//
	afx_msg void OnMButtonDown	( unsigned nFlags, CPoint point );
	afx_msg void OnMButtonUp		( unsigned nFlags, CPoint point );
	afx_msg void OnMButtonDblClk( unsigned nFlags, CPoint point );
	//
	afx_msg void OnKeyDown			( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	afx_msg void OnKeyUp				( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	afx_msg void OnChar					( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	afx_msg void OnSysKeyDown		( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	afx_msg void OnSysKeyUp			( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	afx_msg void OnSysChar			( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	//
	afx_msg void OnContextMenu	( CWnd *pwnd, CPoint point );
	//
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnPaint();
	//
	afx_msg void OnSize( unsigned nType, int cx, int cy );
	afx_msg void OnHScroll( unsigned nSBCode, unsigned nPos, CScrollBar* pScrollBar );
	afx_msg void OnVScroll( unsigned nSBCode, unsigned nPos, CScrollBar* pScrollBar );

public:
	CChildFrameWndBase();
	virtual ~CChildFrameWndBase();

	bool IsSceneUpdateEnabled() { return bEnableSceneUpdate; }
	// ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	// methods
	virtual bool OnCreateChildFrameWnd() = 0;
	virtual void OnDestroyChildFrameWnd() = 0;
	virtual void OnPreDrawChildFrameWnd() = 0;
	virtual void OnDrawChildFrameWnd() = 0;
	virtual void OnResizeChildFrameWnd( int cx, int cy ) = 0;
	virtual void DrawFocus( CPaintDC *pDC ) = 0;
	virtual void DrawStatistic( CPaintDC *pDC ) = 0;
	virtual void DrawFrameBorders( CPaintDC *pDC ) = 0;

	DECLARE_MESSAGE_MAP()
};


