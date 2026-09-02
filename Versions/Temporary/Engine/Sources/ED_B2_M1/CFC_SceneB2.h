#pragma once

#include "ED_Common/ChildFrameWndBase.h"

#include <cstdint>

class CCFCSceneB2 : public CChildFrameWndBase
{
	void ResetCamera( bool bAll );
	void UpdateCamera( uintptr_t dwData );
	void UpdateCameraPosition( uintptr_t dwData );
	void ClearScene();
	bool KeyPressed( unsigned nChar, uintptr_t dwData );

protected:

public:
	CCFCSceneB2();
	virtual ~CCFCSceneB2();

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

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


