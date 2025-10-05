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
}

