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
	~CCFCSceneB2() override;

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;

	// CChildFrameWndBase
	bool OnCreateChildFrameWnd() override;
	void OnDestroyChildFrameWnd() override;
	void OnPreDrawChildFrameWnd() override;
	void OnDrawChildFrameWnd() override;
	void OnResizeChildFrameWnd( int cx, int cy ) override;
	void DrawFocus( IPaintContext *pPaintContext ) override;
	void DrawStatistic( IPaintContext *pPaintContext ) override;
	void DrawFrameBorders( IPaintContext *pPaintContext ) override;
};
