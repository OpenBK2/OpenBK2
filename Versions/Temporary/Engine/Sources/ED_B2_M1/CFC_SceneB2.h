#if !defined(__CHILD_WINDOW__INPUT__)
#define __CHILD_WINDOW__INPUT__
#pragma once

#include "..\ED_Common\ChildFrameWndBase.h"

class CCFCSceneB2 : public CChildFrameWndBase
{
	void ResetCamera( bool bAll );
	void UpdateCamera( DWORD dwData );
	void UpdateCameraPosition( DWORD dwData );
	void ClearScene();
	bool KeyPressed( UINT nChar, DWORD dwData );

protected:

public:
	CCFCSceneB2();
	virtual ~CCFCSceneB2();

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	// CChildFrameWndBase
	bool OnCreateChildFrameWnd();
	void OnDestroyChildFrameWnd();
	void OnPreDrawChildFrameWnd();
	void OnDrawChildFrameWnd();
	void OnResizeChildFrameWnd( int cx, int cy );
	void DrawFocus( CPaintDC *pDC );
	void DrawStatistic( CPaintDC *pDC );
	void DrawFrameBorders( CPaintDC *pDC );
};

#endif // !defined(__CHILD_WINDOW__INPUT__)
