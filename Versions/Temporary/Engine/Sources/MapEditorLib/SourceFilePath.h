#pragma once

#include "MapEditorLib_export.h"

#include <string>

namespace NEditorSource
{
// Store files inside the active mod, Data or export source folder relative to that root.
// Files elsewhere keep their absolute path until the builder imports them.
MAPEDITORLIB_EXPORT std::string MakeRelative( const std::string &path, bool includeSource = true );
MAPEDITORLIB_EXPORT std::string Resolve( const std::string &path );
} // namespace NEditorSource
