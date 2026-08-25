#include "stdafx.h"

#include "SdlVideo.h"

#include <SDL3/SDL.h>

namespace NSdl
{
namespace
{
bool InitVideo()
{
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
}
