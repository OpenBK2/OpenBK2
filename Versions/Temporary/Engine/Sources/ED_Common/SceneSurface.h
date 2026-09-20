#pragma once

#include "MapEditorLib/Interface_Widget.h"

class CChildFrameWndBase;

// The window the 3D viewport lives in, in whichever toolkit this session draws
// it: the part of what CChildFrameWndBase was that is a window.
//
// CChildFrameWndBase keeps everything the viewport decides -- which input goes
// to the active input state and which to the game, when the scene steps and
// redraws, what the scene commands do -- and this is the little it needs of a
// window to do that. The window in turn hands CChildFrameWndBase its input as
// MFC's handlers received it, because that is the form the input states take
// it in: MK_ flags, virtual key codes, a key message's repeat count and flags,
// and points in client coordinates, except the wheel's and the context menu's,
// which Windows gives in screen coordinates.
//
// The viewport is a Direct3D 9 window, and the renderer, DirectInput and
// DirectSound are all given its HWND, so this names one.
struct ISceneSurface
{
	virtual ~ISceneSurface() {}

	// Makes the window as the contents of pFrame and hands it to pCore, whose
	// OnCreate runs while it is being made. False if it could not be made or
	// pCore refused it.
	virtual bool Create( IFrameWindow *pFrame, CChildFrameWndBase *pCore ) = 0;
	// What IMainFrame::SetChildFrameWindowContents is given.
	virtual IWidget* GetWidget() = 0;
	// The window the renderer draws in.
	virtual HWND GetHandle() const = 0;

	// Invalidate, erase and paint at once: CWnd::RedrawWindow() as called
	// without arguments.
	virtual void Redraw() = 0;
	// The same without the erase, which the scene update timer asks for.
	virtual void RedrawWithoutErase() = 0;
	virtual bool HasFocus() const = 0;
	virtual void Focus() = 0;
	// Whether the window this viewport is in is the active one, which is what
	// decides whether the game steps as the active application.
	virtual bool IsInActiveWindow() const = 0;
	// Throws away mouse and key input already queued for the window, so that
	// what the user did while a command ran is not acted on after it. The
	// editors ask for this through ID_SCENE_REMOVE_INPUT.
	virtual void DiscardPendingInput() = 0;
	virtual bool IsShown() const = 0;
	virtual CTPoint<int> GetClientSize() const = 0;
	// Moves and sizes the window, in its parent's client coordinates.
	virtual void Place( const CTRect<int> &rRect ) = 0;
	// Fills rRect of the window with color now, from outside a paint.
	virtual void Fill( const CTRect<int> &rRect, TWidgetColor color ) = 0;
	virtual void SetMouseCapture( bool bCapture ) = 0;
	// The scene update timer. One at a time; OnTimer on the core each tick.
	virtual bool StartUpdateTimer( int nInterval ) = 0;
	virtual void StopUpdateTimer() = 0;
	// Scroll bars, which only the UI screen editors turn on, over a virtual
	// screen larger than the window.
	virtual void ShowScrollBars( bool bShow ) = 0;
	virtual void SetScrollBarRange( bool bVertical, int nMin, int nMax ) = 0;
	virtual void GetScrollBarState( bool bVertical, int *pnMin, int *pnMax, int *pnPos ) const = 0;
	virtual void SetScrollBarPos( bool bVertical, int nPos ) = 0;
};


namespace NSceneSurface
{
	// The surface this session uses. Owned by the caller.
	ISceneSurface* Create();
}
