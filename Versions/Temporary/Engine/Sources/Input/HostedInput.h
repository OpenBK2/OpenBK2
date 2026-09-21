#pragma once

#include "Input_export.h"
#include <SDL3/SDL.h>

namespace NInput
{
// A toolkit embedding an SDL window owns keyboard focus and key delivery.
// Submit its SDL_SCANCODE_COUNT key states on each key/focus transition.
// Pass nullptr on detach. Joysticks continue to use SDL's event watch.
INPUT_EXPORT void UpdateHostedKeyboard( SDL_Window *pWindow, const bool *pKeys, bool bFocused );
// The host's mouse events use SDL's units/button numbering, without posting
// synthetic events to SDL's global queue or interfering with the toolkit.
INPUT_EXPORT void ProcessHostedMouse( SDL_Window *pWindow, const SDL_Event &event );
}
