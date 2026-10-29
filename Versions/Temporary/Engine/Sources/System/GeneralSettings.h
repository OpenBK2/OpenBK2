#pragma once

#include "System_export.h"

#include <string>

namespace NSettings
{

// The on-disk format stays shared across platforms. Only discovery of these two
// roots is platform specific; the installation is never a profile write target.
class SYSTEM_EXPORT CGeneralSettings
{
	std::string szInstallRoot;
	std::string szProfilesRoot;

public:
	CGeneralSettings( const std::string &szInstall, const std::string &szProfiles );
	bool PrepareProfiles( std::string *pszError ) const;
	const std::string &GetInstallRoot() const;
	const std::string &GetProfilesRoot() const;
	std::string ResolveConfigPath( const std::string &szPath, const std::string &szIncludingFile = "" ) const;
	std::string ResolveResourceID( const std::string &szPath ) const;
};

// Explicit roots also let command-line tools and tests use isolated profiles.
SYSTEM_EXPORT bool InitializeGeneralSettings( const CGeneralSettings &settings, std::string *pszError );
SYSTEM_EXPORT bool InitializeGeneralSettings( std::string *pszError );
SYSTEM_EXPORT const CGeneralSettings *GetGeneralSettings();

} // namespace NSettings
