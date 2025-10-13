#pragma once

#include "ED_Common/ChildFrameWndBase.h"

#include <cstdint>

class CCFCSceneB2 : public CChildFrameWndBase
{
	void ResetCamera( bool bAll );
	void UpdateCamera( uint32_t dwData );
	void UpdateCameraPosition( uint32_t dwData );
	void ClearScene();
	bool KeyPressed( UINT nChar, uint32_t dwData );

protected:

public:
	CCFCSceneB2();
	virtual ~CCFCSceneB2();

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, uint32_t dwData );
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


