#pragma once

#include "Main_export.h"

namespace NProfile
{
MAIN_EXPORT void LoadProfile();
MAIN_EXPORT void SaveProfile();
MAIN_EXPORT bool AddProfile( const wstring &szName );
// can change to non existing profile then one will be added
MAIN_EXPORT void ChangeProfile( const wstring &szProfile );
MAIN_EXPORT bool RemoveProfile( const wstring &szName );
void ResetToDefault();
MAIN_EXPORT void GetAllProfiles( vector<wstring> *pRes );
MAIN_EXPORT wstring GetCurrentProfileName();
MAIN_EXPORT string GetCurrentProfileDir();
}

