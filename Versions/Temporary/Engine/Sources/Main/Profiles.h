#pragma once

#include "Main_export.h"

namespace NProfile
{
MAIN_EXPORT void LoadProfile();
MAIN_EXPORT void SaveProfile();
MAIN_EXPORT bool AddProfile( const std::wstring &szName );
// can change to non existing profile then one will be added
MAIN_EXPORT void ChangeProfile( const std::wstring &szProfile );
MAIN_EXPORT bool RemoveProfile( const std::wstring &szName );
void ResetToDefault();
MAIN_EXPORT void GetAllProfiles( std::vector<std::wstring> *pRes );
MAIN_EXPORT std::wstring GetCurrentProfileName();
MAIN_EXPORT std::string GetCurrentProfileDir();
//! Where the current profile's savegames and replays live, with a trailing
//! separator, so a caller appends a file name and has a path.
//!
//! Named here rather than spelled at each of the places that opens one. There
//! were three spellings of the savegame directory and they disagreed off
//! Windows, where a backslash is an ordinary character in a file name rather
//! than a separator: the .sav, the .sfo and the directory the save list scans
//! each came out as a different name, so saving appeared to work and nothing
//! was ever listed to load.
MAIN_EXPORT std::string GetSaveDir();
MAIN_EXPORT std::string GetReplayDir();
}

