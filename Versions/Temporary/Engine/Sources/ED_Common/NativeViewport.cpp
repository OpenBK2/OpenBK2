// The one translation unit in the editor that reaches past wx to the toolkit
// under it. See NativeViewport.h for what it is for and why it is alone.
//
// No stdafx.h: this needs none of it, and off Windows it includes X11 headers
// whose None, Status, Bool and True macros would then be loose in a
// precompiled header shared by the whole module.
#include "NativeViewport.h"

#include <boost/predef.h>

// wx first, and X11 after it: wx's headers do not expect X11's macros to
// already be defined, and X11 does not mind wx.
#include <wx/window.h>
#include <wx/toplevel.h>

#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <SDL3/SDL.h>
#include "Input/HostedInput.h"
#endif


#if BOOST_OS_WINDOWS

// A wxWindow already has the handle; there is nothing to make or let go of.
struct CNativeViewport::SImpl
{
	HWND hWindow = 0;
};


CNativeViewport::CNativeViewport() : pImpl( new SImpl )
{
}


CNativeViewport::~CNativeViewport() = default;


bool CNativeViewport::Attach( wxWindow *pWindow )
{
	pImpl->hWindow = ( pWindow != nullptr ) ? static_cast<HWND>( pWindow->GetHandle() ) : 0;
	return pImpl->hWindow != 0;
}


void CNativeViewport::Detach()
{
	pImpl->hWindow = 0;
}


void CNativeViewport::SetSize( int, int )
{
	// The window is the handle, and wx has already sized it.
}

void CNativeViewport::PumpEvents()
{
}

void CNativeViewport::OnKey( unsigned, bool )
{
}

void CNativeViewport::OnMouse( const wxMouseEvent & )
{
}

void CNativeViewport::SetFocus( bool )
{
}


void* CNativeViewport::GetHandle() const
{
	return pImpl->hWindow;
}

#else

namespace
{
	SDL_Scancode ScanCode( unsigned nHardwareCode )
	{
		// Use the unshifted key on GTK's current layout. wx's character code
		// changes with Shift and merges left/right modifiers, breaking chords.
		guint key = 0;
		gdk_keymap_translate_keyboard_state( gdk_keymap_get_for_display( gdk_display_get_default() ),
			nHardwareCode, static_cast<GdkModifierType>( 0 ), 0, &key, nullptr, nullptr, nullptr );
		if ( key >= GDK_KEY_F1 && key <= GDK_KEY_F12 )
			return static_cast<SDL_Scancode>( SDL_SCANCODE_F1 + key - GDK_KEY_F1 );
		switch ( key )
		{
			case GDK_KEY_Control_L: return SDL_SCANCODE_LCTRL;
			case GDK_KEY_Control_R: return SDL_SCANCODE_RCTRL;
			case GDK_KEY_Shift_L: return SDL_SCANCODE_LSHIFT;
			case GDK_KEY_Shift_R: return SDL_SCANCODE_RSHIFT;
			case GDK_KEY_Alt_L: case GDK_KEY_Meta_L: return SDL_SCANCODE_LALT;
			case GDK_KEY_Alt_R: case GDK_KEY_Meta_R: case GDK_KEY_ISO_Level3_Shift: return SDL_SCANCODE_RALT;
			case GDK_KEY_Super_L: return SDL_SCANCODE_LGUI;
			case GDK_KEY_Super_R: return SDL_SCANCODE_RGUI;
			case GDK_KEY_Left: return SDL_SCANCODE_LEFT;
			case GDK_KEY_Right: return SDL_SCANCODE_RIGHT;
			case GDK_KEY_Up: return SDL_SCANCODE_UP;
			case GDK_KEY_Down: return SDL_SCANCODE_DOWN;
			case GDK_KEY_Home: return SDL_SCANCODE_HOME;
			case GDK_KEY_End: return SDL_SCANCODE_END;
			case GDK_KEY_Page_Up: return SDL_SCANCODE_PAGEUP;
			case GDK_KEY_Page_Down: return SDL_SCANCODE_PAGEDOWN;
			case GDK_KEY_Insert: return SDL_SCANCODE_INSERT;
			case GDK_KEY_Delete: return SDL_SCANCODE_DELETE;
			case GDK_KEY_Escape: return SDL_SCANCODE_ESCAPE;
			case GDK_KEY_Return: return SDL_SCANCODE_RETURN;
			case GDK_KEY_Tab: case GDK_KEY_ISO_Left_Tab: return SDL_SCANCODE_TAB;
			case GDK_KEY_BackSpace: return SDL_SCANCODE_BACKSPACE;
			case GDK_KEY_Pause: return SDL_SCANCODE_PAUSE;
			case GDK_KEY_Print: return SDL_SCANCODE_PRINTSCREEN;
			case GDK_KEY_Caps_Lock: return SDL_SCANCODE_CAPSLOCK;
			case GDK_KEY_Num_Lock: return SDL_SCANCODE_NUMLOCKCLEAR;
			case GDK_KEY_Scroll_Lock: return SDL_SCANCODE_SCROLLLOCK;
			case GDK_KEY_KP_0: case GDK_KEY_KP_Insert: return SDL_SCANCODE_KP_0;
			case GDK_KEY_KP_1: case GDK_KEY_KP_End: return SDL_SCANCODE_KP_1;
			case GDK_KEY_KP_2: case GDK_KEY_KP_Down: return SDL_SCANCODE_KP_2;
			case GDK_KEY_KP_3: case GDK_KEY_KP_Page_Down: return SDL_SCANCODE_KP_3;
			case GDK_KEY_KP_4: case GDK_KEY_KP_Left: return SDL_SCANCODE_KP_4;
			case GDK_KEY_KP_5: case GDK_KEY_KP_Begin: return SDL_SCANCODE_KP_5;
			case GDK_KEY_KP_6: case GDK_KEY_KP_Right: return SDL_SCANCODE_KP_6;
			case GDK_KEY_KP_7: case GDK_KEY_KP_Home: return SDL_SCANCODE_KP_7;
			case GDK_KEY_KP_8: case GDK_KEY_KP_Up: return SDL_SCANCODE_KP_8;
			case GDK_KEY_KP_9: case GDK_KEY_KP_Page_Up: return SDL_SCANCODE_KP_9;
			case GDK_KEY_KP_Add: return SDL_SCANCODE_KP_PLUS;
			case GDK_KEY_KP_Subtract: return SDL_SCANCODE_KP_MINUS;
			case GDK_KEY_KP_Multiply: return SDL_SCANCODE_KP_MULTIPLY;
			case GDK_KEY_KP_Divide: return SDL_SCANCODE_KP_DIVIDE;
			case GDK_KEY_KP_Enter: return SDL_SCANCODE_KP_ENTER;
			case GDK_KEY_KP_Decimal: case GDK_KEY_KP_Delete: return SDL_SCANCODE_KP_PERIOD;
			default: return SDL_GetScancodeFromKey( gdk_keyval_to_unicode( key ), nullptr );
		}
	}
}

struct CNativeViewport::SImpl
{
	SDL_Window *pSdlWindow = nullptr;
	wxWindow *pWindow = nullptr;
	bool bFocused = false;
	bool keys[SDL_SCANCODE_COUNT] = {};
	wxPoint mousePosition;
	bool bHasMousePosition = false;
	// The size SDL was last told, so it is not told again for nothing. See
	// SetSize for why that matters more than it looks.
	int nWidth = 0;
	int nHeight = 0;
};


CNativeViewport::CNativeViewport() : pImpl( new SImpl )
{
}


CNativeViewport::~CNativeViewport()
{
	Detach();
}


bool CNativeViewport::Attach( wxWindow *pWindow )
{
	Detach();
	if ( pWindow == nullptr )
	{
		return false;
	}
	GtkWidget *const pWidget = pWindow->GetHandle();
	if ( pWidget == nullptr )
	{
		return false;
	}
	// A widget has no GdkWindow until it is realized, and wx does not realize
	// one until it is shown.
	gtk_widget_realize( pWidget );
	GdkWindow *const pGdkWindow = gtk_widget_get_window( pWidget );
	if ( pGdkWindow == nullptr )
	{
		return false;
	}
	// GTK3 draws most widgets into an ancestor's window and gives them none of
	// their own. This forces one, which is what has an XID.
	gdk_window_ensure_native( pGdkWindow );
	if ( !GDK_IS_X11_WINDOW( pGdkWindow ) )
	{
		// A Wayland session. Nothing here works on one; see NativeViewport.h.
		return false;
	}

	// SDL_InitSubSystem rather than SDL_Init, and never SDL_Quit: the
	// subsystem calls are reference counted and something else in the process
	// may hold video already. System/SdlVideo.cpp says the same for the game.
	if ( !SDL_InitSubSystem( SDL_INIT_VIDEO ) )
	{
		return false;
	}

	const Window xid = gdk_x11_window_get_xid( pGdkWindow );
	// The window has to exist on the server, not just in GTK's request buffer,
	// before anything on another connection names it. GTK holds its requests
	// until the main loop next flushes, and the SDL window made below is on
	// SDL's own connection, as is the Vulkan surface DXVK creates from it: the
	// surface call asks the server about this XID, gets BadWindow for a window
	// that has not been created yet, and comes back as VK_ERROR_OUT_OF_HOST_MEMORY,
	// which surfaces as CreateDevice failing with D3DERR_NOTAVAILABLE.
	//
	// A sync rather than a flush, since a flush only puts the bytes on the wire
	// and says nothing about when the server acts on them, and the requests here
	// come from a different connection than the ones that must already be done.
	gdk_display_sync( gdk_window_get_display( pGdkWindow ) );

	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER, static_cast<Sint64>( xid ) );
	// GTK owns this child window's position. SDL's non-resizable X11 resize
	// path also moves and raises the window using SDL's cached coordinates,
	// which go stale when wx lays out the docking panes. Mark the wrapper as
	// resizable so SetSize only resizes it, preserving GTK's position and
	// stacking order instead of jumping back over neighbouring panes.
	SDL_SetBooleanProperty( props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true );
	// What DXVK will ask this window for.
	SDL_SetBooleanProperty( props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true );
	pImpl->pSdlWindow = SDL_CreateWindowWithProperties( props );
	SDL_DestroyProperties( props );
	if ( pImpl->pSdlWindow == nullptr )
	{
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
		return false;
	}

	// The size, which the wrapped window does not carry. See SetSize.
	pImpl->pWindow = pWindow;
	NInput::UpdateHostedKeyboard( pImpl->pSdlWindow, pImpl->keys, false );
	const wxSize size = pWindow->GetClientSize();
	SetSize( size.x, size.y );
	return true;
}


void CNativeViewport::Detach()
{
	if ( pImpl->pSdlWindow != nullptr )
	{
		SetFocus( false );
		NInput::UpdateHostedKeyboard( nullptr, nullptr, false );
		SDL_DestroyWindow( pImpl->pSdlWindow );
		pImpl->pSdlWindow = nullptr;
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
	}
	// So a later Attach does not take the previous window's size for its own.
	pImpl->nWidth = 0;
	pImpl->nHeight = 0;
	pImpl->pWindow = nullptr;
}


void CNativeViewport::SetSize( int nWidth, int nHeight )
{
	if ( ( pImpl->pSdlWindow == nullptr ) || ( nWidth <= 0 ) || ( nHeight <= 0 ) )
	{
		return;
	}
	// Only when it has actually changed, and that is not a micro-optimisation.
	//
	// SDL_SetWindowSize on a window wrapping a foreign X11 one goes through
	// X11_ExternalResizeMoveSync, which waits up to 100 ms for the window
	// manager to act on the request -- "wait a brief time to see if the window
	// manager decided to let the move or resize happen", in SDL's own words. It
	// breaks out early only once the window changes, and this window's size is
	// GTK's to decide rather than the manager's, so a call that asks for the
	// size it already has can spend the whole 100 ms. On the thread the editor
	// lays out and loads on, a handful of those is a visible stall.
	if ( ( nWidth == pImpl->nWidth ) && ( nHeight == pImpl->nHeight ) )
	{
		return;
	}
	pImpl->nWidth = nWidth;
	pImpl->nHeight = nHeight;
	// The whole reason this method exists; see the header. Without it SDL
	// answers for this window with uninitialised numbers and reports success.
	SDL_SetWindowSize( pImpl->pSdlWindow, nWidth, nHeight );
}


void* CNativeViewport::GetHandle() const
{
	return pImpl->pSdlWindow;
}

void CNativeViewport::PumpEvents()
{
	if ( pImpl->pSdlWindow == nullptr )
		return;
	auto *pTop = wxDynamicCast( wxGetTopLevelParent( pImpl->pWindow ), wxTopLevelWindow );
	SetFocus( pImpl->pWindow->HasFocus() && pTop != nullptr && pTop->IsActive() );
	// MapNotify and ConfigureNotify update SDL's visibility and pixel size.
	// Without this, CheckBackBufferSize considers the mapped GTK child hidden
	// and leaves rendering and picking at the initial 1024x768 resolution.
	// Input's event watch records device transitions before we drain the queue.
	SDL_Event event;
	while ( SDL_PollEvent( &event ) )
	{
	}
}

void CNativeViewport::SetFocus( bool bFocused )
{
	if ( pImpl->pSdlWindow == nullptr || pImpl->bFocused == bFocused )
		return;
	pImpl->bFocused = bFocused;
	pImpl->bHasMousePosition = false;
	for ( bool &key : pImpl->keys )
		key = false;
	if ( bFocused )
	{
		// A modifier can already be held when the viewport gains wx focus.
		// Snapshot only on entry; individual key events preserve short presses.
		char keymap[32];
		XQueryKeymap( gdk_x11_display_get_xdisplay( gdk_display_get_default() ), keymap );
		for ( unsigned code = 8; code < 256; ++code )
		{
			if ( keymap[code / 8] & ( 1 << ( code % 8 ) ) )
				pImpl->keys[ScanCode( code )] = true;
		}
	}
	NInput::UpdateHostedKeyboard( pImpl->pSdlWindow, pImpl->keys, bFocused );
}

void CNativeViewport::OnKey( unsigned nHardwareCode, bool bDown )
{
	if ( pImpl->pSdlWindow == nullptr || !pImpl->bFocused )
		return;
	const SDL_Scancode code = ScanCode( nHardwareCode );
	if ( code == SDL_SCANCODE_UNKNOWN )
		return;
	pImpl->keys[code] = bDown;
	NInput::UpdateHostedKeyboard( pImpl->pSdlWindow, pImpl->keys, true );
}

void CNativeViewport::OnMouse( const wxMouseEvent &event )
{
	if ( pImpl->pSdlWindow == nullptr )
		return;
	if ( event.Entering() || event.Leaving() )
	{
		pImpl->bHasMousePosition = false;
		return;
	}
	SDL_Event input = {};
	input.common.timestamp = SDL_GetTicksNS();
	// Screen positions keep moving a docking pane from becoming camera motion.
	const wxPoint position = pImpl->pWindow->ClientToScreen( event.GetPosition() );
	if ( event.Moving() || event.Dragging() )
	{
		input.type = SDL_EVENT_MOUSE_MOTION;
		if ( pImpl->bHasMousePosition )
		{
			input.motion.xrel = position.x - pImpl->mousePosition.x;
			input.motion.yrel = position.y - pImpl->mousePosition.y;
		}
	}
	else if ( event.GetEventType() == wxEVT_MOUSEWHEEL )
	{
		if ( event.GetWheelAxis() != wxMOUSE_WHEEL_VERTICAL || event.GetWheelDelta() == 0 )
			return;
		input.type = SDL_EVENT_MOUSE_WHEEL;
		input.wheel.y = static_cast<float>( event.GetWheelRotation() ) / event.GetWheelDelta();
	}
	else if ( event.ButtonDown() || event.ButtonDClick() || event.ButtonUp() )
	{
		input.type = event.ButtonUp() ? SDL_EVENT_MOUSE_BUTTON_UP : SDL_EVENT_MOUSE_BUTTON_DOWN;
		switch ( event.GetButton() )
		{
			case wxMOUSE_BTN_LEFT: input.button.button = SDL_BUTTON_LEFT; break;
			case wxMOUSE_BTN_RIGHT: input.button.button = SDL_BUTTON_RIGHT; break;
			case wxMOUSE_BTN_MIDDLE: input.button.button = SDL_BUTTON_MIDDLE; break;
			case wxMOUSE_BTN_AUX1: input.button.button = SDL_BUTTON_X1; break;
			case wxMOUSE_BTN_AUX2: input.button.button = SDL_BUTTON_X2; break;
			default: return;
		}
	}
	pImpl->mousePosition = position;
	pImpl->bHasMousePosition = true;
	NInput::ProcessHostedMouse( pImpl->pSdlWindow, input );
}

#endif
