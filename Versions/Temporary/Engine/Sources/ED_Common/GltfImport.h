#pragma once
#include "ED_Common_export.h"
#include <string>

namespace NEditorGltf
{
// Copy a model package into the active data/mod folder and return its portable
// reference. External models are placed beside the destination XDB resource.
ED_COMMON_EXPORT bool ImportModelFile( const std::string &objectName, const std::string &source,
	std::string *reference, std::string *error );
}
