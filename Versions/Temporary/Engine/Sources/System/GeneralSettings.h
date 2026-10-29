#pragma once

#include "System_export.h"

#include <string>

namespace NSettings
{

// The on-disk format stays shared across platforms. Root discovery is platform
// specific; writable Profiles and Editor directories live outside the defaults.
class SYSTEM_EXPORT CGeneralSettings
{
	std::string szInstallRoot;
	std::string szProfilesRoot;

public:
	CGeneralSettings( const std::string &szInstall, const std::string &szProfiles );
	bool PrepareProfiles( std::string *pszError ) const;
	bool PrepareEditor( std::string *pszError ) const;
	bool ClearEditorIconCache( std::string *pszError ) const;
	const std::string &GetInstallRoot() const;
	const std::string &GetProfilesRoot() const;
	std::string GetEditorRoot() const;
	// Physical editor files only; Editor/... database IDs still belong to the VFS.
	std::string ResolveEditorPath( const std::string &szPath ) const;
	std::string ResolveConfigPath( const std::string &szPath, const std::string &szIncludingFile = "" ) const;
	std::string ResolveResourceID( const std::string &szPath ) const;
};

// Explicit roots also let command-line tools and tests use isolated profiles.
SYSTEM_EXPORT bool InitializeGeneralSettings( const CGeneralSettings &settings, std::string *pszError );
SYSTEM_EXPORT bool InitializeGeneralSettings( std::string *pszError );
SYSTEM_EXPORT const CGeneralSettings *GetGeneralSettings();
SYSTEM_EXPORT std::string ResolveEditorPath( const std::string &szPath );

} // namespace NSettings
