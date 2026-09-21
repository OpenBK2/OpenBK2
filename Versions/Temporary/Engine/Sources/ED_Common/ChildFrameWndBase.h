#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_Widget.h"

#include <cstdint>

#include "ED_Common_export.h"

struct ISceneSurface;

// What a scroll bar has been asked to do. CChildFrameWndBase reads these where
// it read WM_HSCROLL's and WM_VSCROLL's SB_ codes; turning the toolkit's scroll
// notification into one of them is the surface's job, which is what keeps the
// codes out of everything above it.
//
// Back and forward rather than left and up: the horizontal and vertical sets
// were the same six actions under twelve names, and the frame handled both with
// the same two switches.
enum EScrollAction
{
	SCROLL_TO_START,			// SB_LEFT, SB_TOP
	SCROLL_TO_END,				// SB_RIGHT, SB_BOTTOM
	SCROLL_LINE_BACK,			// SB_LINELEFT, SB_LINEUP
	SCROLL_LINE_FORWARD,	// SB_LINERIGHT, SB_LINEDOWN
	SCROLL_PAGE_BACK,			// SB_PAGELEFT, SB_PAGEUP
	SCROLL_PAGE_FORWARD,	// SB_PAGERIGHT, SB_PAGEDOWN
	SCROLL_THUMB,					// SB_THUMBPOSITION, SB_THUMBTRACK
	SCROLL_OTHER,					// anything else, which the frame ignores
};


// The 3D viewport: what goes to the active input state and what to the game,
// when the scene steps and redraws, and the CHID_SCENE commands.
//
// This was a CWnd with a message map. The window is now an ISceneSurface
// (SceneSurface.h), MFC's or wx's, which calls the On... methods below with
// its messages as MFC's handlers received them, after doing what the CWnd base
// class did with each. Nothing here names a toolkit, and the order of every
// call is the one the message handlers made.
//
// The surface is known between OnCreate and OnDestroy. A command that arrives
// outside that span, when there is no window to act on, does nothing.
class ED_COMMON_EXPORT CChildFrameWndBase : public ICommandHandler
{
	static const int DEFAULT_REFRESH_RATE;

	ISceneSurface *pSurface;

	bool bInputEnabled;
	bool bRunModeEnabled;
	bool bGameInputEnabled;
	bool bRenderEnabled;
	bool bShowStatistic;
	bool bShowMovieBorders;
	bool bEnableSceneUpdate;
	bool bEnableScroll;
	//
	bool bUpdateSceneTimer;
	int32_t nUpdateSceneTimerInterval;

	bool bIsSettingUp;
	bool bWasResized;

	void SetUpdateSceneTimer();
	void KillUpdateSceneTimer();
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
	CTRect<int> rectBorder1, rectBorder2, rectWindow, rectMain;

	// The window, between OnCreate and OnDestroy; null outside them.
	ISceneSurface* Surface() const { return pSurface; }
	// Invalidate, erase and paint the window now, if there is one.
	void Redraw();

public:
	CChildFrameWndBase();
	virtual ~CChildFrameWndBase();

	bool IsSceneUpdateEnabled() { return bEnableSceneUpdate; }
	bool IsScrollEnabled() const { return bEnableScroll; }
	// While the window is moving itself to keep the game's aspect: the size
	// and paint messages that causes are not the viewport's to act on.
	bool IsSettingUp() const { return bIsSettingUp; }

	// From the surface. OnCreate is called while the window is being made and
	// refuses it by returning false; OnDestroy while it is being destroyed.
	bool OnCreate( ISceneSurface *_pSurface );
	void OnDestroy();
	// The scene update timer's tick.
	void OnTimer();
	//
	void OnSetFocus			( IWidget *pOldWidget );
	void OnKillFocus		( IWidget *pNewWidget );

	void OnMouseMove		( unsigned nFlags, const CTPoint<int> &rPoint );
	// rPoint in viewport coordinates, as the input state's terrain picker expects, and
	// bDefaultResult what the window's default handling of the wheel answered:
	// the input state only hears of a wheel the window did something with.
	bool OnMouseWheel		( unsigned nFlags, short zDelta, const CTPoint<int> &rPoint, bool bDefaultResult );
	//
	void OnLButtonDown	( unsigned nFlags, const CTPoint<int> &rPoint );
	void OnLButtonUp		( unsigned nFlags, const CTPoint<int> &rPoint );
	void OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rPoint );
	//
	void OnRButtonDown	( unsigned nFlags, const CTPoint<int> &rPoint );
	void OnRButtonUp		( unsigned nFlags, const CTPoint<int> &rPoint );
	void OnRButtonDblClk( unsigned nFlags, const CTPoint<int> &rPoint );
	//
	void OnMButtonDown	( unsigned nFlags, const CTPoint<int> &rPoint );
	void OnMButtonUp		( unsigned nFlags, const CTPoint<int> &rPoint );
	void OnMButtonDblClk( unsigned nFlags, const CTPoint<int> &rPoint );
	//
	void OnKeyDown			( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	void OnKeyUp				( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	//
	// rScreenPoint in screen coordinates, or ( -1, -1 ) from the keyboard.
	void OnContextMenu	( const CTPoint<int> &rScreenPoint );
	//
	// A paint in two halves around the surface's paint context. BeginPaint
	// lays the window out and prepares the renderer, or answers false for a
	// paint that arrives while the window is moving itself, which the surface
	// only validates. Paint draws the input state's overlay, the scene and the
	// front-end's own marks through the context.
	bool BeginPaint();
	void Paint( IPaintContext *pPaintContext );
	//
	void OnSize( int cx, int cy );
	void OnHScroll( EScrollAction eAction, unsigned nPos );
	void OnVScroll( EScrollAction eAction, unsigned nPos );

	// ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	// methods
	virtual bool OnCreateChildFrameWnd() = 0;
	virtual void OnDestroyChildFrameWnd() = 0;
	virtual void OnPreDrawChildFrameWnd() = 0;
	virtual void OnDrawChildFrameWnd() = 0;
	virtual void OnResizeChildFrameWnd( int cx, int cy ) = 0;
	virtual void DrawFocus( IPaintContext *pPaintContext ) = 0;
	virtual void DrawStatistic( IPaintContext *pPaintContext ) = 0;
	virtual void DrawFrameBorders( IPaintContext *pPaintContext ) = 0;
};


