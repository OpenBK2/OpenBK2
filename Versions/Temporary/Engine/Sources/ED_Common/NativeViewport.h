#pragma once

#include <memory>

class wxWindow;
class wxMouseEvent;

// The window handle the renderer is given for the viewport, made out of a wx
// window.
//
// On Windows this is nothing: a wxWindow has an HWND and that is the handle.
// Off Windows it is the whole of the viewport's platform problem, and the
// reason this is a class rather than a function.
//
// DXVK Native here is built with the SDL3 WSI backend -- libdxvk_d3d9.so
// dlopens libSDL3.so.0 and calls SDL_Vulkan_CreateSurface -- and it types HWND
// as HANDLE, which is void*. What is behind that pointer is an SDL_Window, as
// port/window.h says for the game's own window. So the viewport needs an
// SDL_Window that lives inside a wx widget rather than being a top level
// window of its own, and SDL3 will make one by wrapping an existing X11
// window: SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER.
//
// A wx widget is a GTK widget, and a GTK3 widget can be made to have an X11
// window of its own with gdk_window_ensure_native. GTK4 removed that, which is
// one of the reasons this port is on GTK3.
//
// Wayland is deliberately not attempted. There are no nested windows there;
// wl_subsurface is the only in-window embedding, subsurfaces are not clipped
// by their parent's geometry, and their position commits with the parent, so a
// scrolling MDI editor is the worst case for them. Run under XWayland instead.
// If native Wayland is ever wanted the answer is to render offscreen and let
// the toolkit composite, not SDL_PROP_WINDOW_CREATE_WAYLAND_SURFACE_ROLE_CUSTOM.
//
// Declared without naming GTK, X11 or SDL so that their headers stay in one
// translation unit: X11 in particular defines None, Status, Bool and True as
// macros, and this header is included by a file that includes most of the
// editor.
//
// scripts/port/surfaceprobe.c is the standalone proof of every step, and of
// the one trap in it, which SetSize below exists for.
class CNativeViewport
{
	struct SImpl;
	// Out of line, and the destructor with it: unique_ptr needs SImpl complete
	// to destroy it, and it is not complete here.
	std::unique_ptr<SImpl> pImpl;

public:
	CNativeViewport();
	~CNativeViewport();

	// Makes the handle for pWindow, which has to exist already -- a wx window
	// has no native window of its own until it is created. False if the handle
	// could not be made, which off Windows means the session is not X11.
	bool Attach( wxWindow *pWindow );
	// Lets the handle go. Called before the wx window is destroyed.
	void Detach();

	// Tells the handle how big the window is now.
	//
	// **This is not optional off Windows.** An SDL window wrapping a foreign
	// X11 window has no size of its own: SDL reports uninitialised numbers for
	// it and returns success while doing so, so nothing says it is wrong, and
	// a swapchain built from them is built from noise. Passing the size as a
	// creation property does not help; only SDL_SetWindowSize does. Attach
	// calls this once, and the viewport calls it on every resize.
	void SetSize( int nWidth, int nHeight );

	// Service the renderer's SDL wrapper even while the viewport lacks focus.
	// GTK owns the event loop, so the game's SDL window pump never runs here.
	void PumpEvents();
	// GTK's hardware key code, as wxKeyEvent::GetRawKeyFlags() supplies it.
	void OnKey( unsigned nHardwareCode, bool bDown );
	void OnMouse( const wxMouseEvent &event );
	void SetFocus( bool bFocused );

	// The handle itself: an HWND on Windows, an SDL_Window* off it, and null
	// before Attach or after Detach. void* rather than HWND so that this
	// header needs no windows.h.
	void* GetHandle() const;
};
