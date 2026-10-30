#pragma once

#include "System_export.h"

#include <string>

namespace NSettings
{

SYSTEM_EXPORT std::string GetApplicationDataDirectory();
// Apply the installation's optional app_name.txt beneath an explicit data root.
// This overload also lets tests use isolated directories without changing the environment.
SYSTEM_EXPORT std::string GetApplicationDataDirectory( const std::string &szInstallRoot,
                                                       const std::string &szDataRoot );
SYSTEM_EXPORT std::string GetInstallationDirectory();

} // namespace NSettings
