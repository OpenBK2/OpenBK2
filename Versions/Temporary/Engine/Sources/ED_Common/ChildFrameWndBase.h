#pragma once

#include "..\MapEditorLib\Interface_CommandHandler.h"

class CChildFrameWndBase : public CWnd, public ICommandHandler
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
	LONG nUpdateSceneTimer;
	LONG nUpdateSceneTimerInterval;

	bool bIsSettingUp;
	bool bWasResized;

	UINT GetUpdateSceneTimerID() { return 1; }
	void SetUpdateSceneTimer();
	void KillUpdateSceneTimer();
	void OnUpdateSceneTimer();
	//
	void AlignWndAspect();
	//
	void RemoveInput(); 
	void EnableInput( DWORD dwData );
	void EnableAutoUpdate( DWORD dwData );
	void EnableGameInput( interface IInterfaceCommand *pInterfaceCommand );
	void DisableGameInput();
	void GetDimensions( DWORD dwData );
	void EnableRunMode( DWORD dwData );
	void EnableRender( DWORD dwData );
	void EnableScrollbars( DWORD dwData );
	void EnableMouseCapture( DWORD dwData );

protected:
	CRect rectBorder1, rectBorder2, rectWindow, rectMain;

	virtual BOOL PreCreateWindow( CREATESTRUCT &rCreateStruct );
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnDestroy();
	afx_msg void OnTimer( UINT nIDEvent );
	//
	afx_msg void OnSetFocus			( CWnd* pOldWnd );
	afx_msg void OnKillFocus		( CWnd* pNewWnd );

	afx_msg void OnMouseMove		( UINT nFlags, CPoint point );
	afx_msg BOOL OnMouseWheel		( UINT nFlags, short zDelta, CPoint point );
	//
	afx_msg void OnLButtonDown	( UINT nFlags, CPoint point );
	afx_msg void OnLButtonUp		( UINT nFlags, CPoint point );
	afx_msg void OnLButtonDblClk( UINT nFlags, CPoint point );
	//
	afx_msg void OnRButtonDown	( UINT nFlags, CPoint point );
	afx_msg void OnRButtonUp		( UINT nFlags, CPoint point );
	afx_msg void OnRButtonDblClk( UINT nFlags, CPoint point );
	//
	afx_msg void OnMButtonDown	( UINT nFlags, CPoint point );
	afx_msg void OnMButtonUp		( UINT nFlags, CPoint point );
	afx_msg void OnMButtonDblClk( UINT nFlags, CPoint point );
	//
	afx_msg void OnKeyDown			( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnKeyUp				( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnChar					( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnSysKeyDown		( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnSysKeyUp			( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnSysChar			( UINT nChar, UINT nRepCnt, UINT nFlags );
	//
	afx_msg void OnContextMenu	( CWnd *pwnd, CPoint point );
	//
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnPaint();
	//
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );

public:
	CChildFrameWndBase();
	virtual ~CChildFrameWndBase();

	bool IsSceneUpdateEnabled() { return bEnableSceneUpdate; }
	// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

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


