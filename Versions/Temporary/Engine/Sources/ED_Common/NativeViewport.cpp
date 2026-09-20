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

#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <SDL3/SDL.h>
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


void* CNativeViewport::GetHandle() const
{
	return pImpl->hWindow;
}

#else

struct CNativeViewport::SImpl
{
	SDL_Window *pSdlWindow = nullptr;
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
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER, static_cast<Sint64>( xid ) );
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
	const wxSize size = pWindow->GetClientSize();
	SetSize( size.x, size.y );
	return true;
}


void CNativeViewport::Detach()
{
	if ( pImpl->pSdlWindow != nullptr )
	{
		SDL_DestroyWindow( pImpl->pSdlWindow );
		pImpl->pSdlWindow = nullptr;
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
	}
}


void CNativeViewport::SetSize( int nWidth, int nHeight )
{
	if ( ( pImpl->pSdlWindow == nullptr ) || ( nWidth <= 0 ) || ( nHeight <= 0 ) )
	{
		return;
	}
	// The whole reason this method exists; see the header. Without it SDL
	// answers for this window with uninitialised numbers and reports success.
	SDL_SetWindowSize( pImpl->pSdlWindow, nWidth, nHeight );
}


void* CNativeViewport::GetHandle() const
{
	return pImpl->pSdlWindow;
}

#endif
