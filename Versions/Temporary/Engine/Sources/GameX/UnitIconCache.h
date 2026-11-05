#pragma once

#include "GameX_export.h"

#include <string>

// Sharper unit icons.
//
// A vehicle's icon (MechUnitRPGStats.IconTexture) is a 42x42 picture made by
// the model editor, and the UI, laid out for 1024x768, stretches it to the
// screen. This replaces it, through 3Dmotor's ITextureOverride, with a picture
// of the same unit at a whole multiple of that size, enough for the screen it
// is shown on, kept in a disk cache under the user's application data. The UI
// lays it out exactly as before.
//
// The game variable unit_icons_hd = 0 turns it off.
namespace NUnitIcons
{

// Installs the texture source; once, at start
void Initialize();

// Forgets what it knows about the database: after a mod is attached or detached
void Reset();

// The multiple of the 42 pixel icon the current screen needs: the smallest
// whole one at least as large as the UI stretches the icon; 1 for none
GAMEX_EXPORT int GetScale();

// Where the cache keeps the picture of the unit a MechUnitRPGStats record
// describes, at nScale; for the installation and mod attached now. IconRender
// --cache fills it ahead of time.
GAMEX_EXPORT std::string GetCacheFile( const std::string &szStatsPath, int nScale );

} // namespace NUnitIcons
