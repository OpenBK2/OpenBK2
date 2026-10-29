#pragma once

#include "System_export.h"

#include <string>

namespace NSettings
{

SYSTEM_EXPORT std::string GetApplicationDataDirectory();
SYSTEM_EXPORT std::string GetInstallationDirectory();

} // namespace NSettings
