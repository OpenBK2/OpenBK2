#pragma once
#include "ED_Common_export.h"
#include <string>

struct IManipulator;
namespace NDb { struct SModel; }
namespace NEditorGltf
{
struct SGrannyExportOptions
{
	bool separateTextures = true;
	// Only applies to separate files; embedded textures remain PNG/DDS.
	bool convertTexturesToTga = true;
	bool mirrorX = true;
};

// Read the selected Model and its mounted game/mod resources without modifying
// any database records. destination is a UTF-8 native filesystem path.
ED_COMMON_EXPORT bool ExportGrannyModel( IManipulator *resource, const std::string &destination,
	const SGrannyExportOptions &options, std::string *error );
ED_COMMON_EXPORT bool ExportGrannyModel( const NDb::SModel *model, const std::string &destination,
	const SGrannyExportOptions &options, std::string *error );
}
