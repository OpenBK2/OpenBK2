#include "stdafx.h"

#include "SdlVideo.h"

#include <SDL3/SDL.h>

namespace NSdl
{
namespace
{
bool InitVideo()
{
	// The identifier is what a Wayland compositor matches against an installed
	// desktop file, so it has to be the basename of Game/blitzkrieg2.desktop.in
	// and nothing else. That match is how the window gets an icon on a
	// compositor without xdg_toplevel_icon_v1, which is most of them; it is also
	// what groups the window in a dock and names it in a screen recorder.
	//
	// Set before SDL_InitSubSystem here, which covers the window. It may already
	// be too late for audio, since flessd can bring that up first from wherever
	// the sound engine is touched, and PulseAudio reads the name when the stream
	// opens. Nothing reads it earlier than that, so a stream that opens first
	// carries SDL's default name and the window is unaffected.
	SDL_SetAppMetadata( "Blitzkrieg II", 0, "blitzkrieg2" );
	// SDL_InitSubSystem, never SDL_Init: third_party/flessd already holds
	// SDL_INIT_AUDIO, and the subsystem calls are reference counted so neither
	// side disturbs the other. For the same reason nothing here calls
	// SDL_Quit, which would take audio down with it.
	if ( SDL_InitSubSystem( SDL_INIT_VIDEO ) )
	{
		return true;
	}
	csSystem << CC_RED << "SDL video is unavailable: " << SDL_GetError() << endl;
	return false;
}

bool InitJoystick()
{
	if ( SDL_InitSubSystem( SDL_INIT_JOYSTICK ) )
	{
		return true;
	}
	// Not red: no stick attached is not a fault, and the game plays without one.
	csSystem << "SDL joysticks are unavailable: " << SDL_GetError() << endl;
	return false;
}
}

bool EnsureVideo()
{
	// Computed on first call rather than at load, so this is safe to reach from
	// a static constructor in another translation unit, where the order between
	// the two would otherwise be undefined.
	//
	// Not released on the way out. Releasing needs a last user, and there is no
	// point in the shutdown where one can be identified: the window, the cursor
	// and whatever a mod created are all holders, and they do not come down in
	// a known order. The subsystem lives until the process does.
	static const bool bReady = InitVideo();
	return bReady;
}

bool EnsureJoystick()
{
	// Computed on first call, for the same reason EnsureVideo's is, and never
	// released for the same reason either.
	static const bool bReady = InitJoystick();
	return bReady;
}
}
