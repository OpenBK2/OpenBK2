#pragma once

#include "System/XmlSaver.h"

#include "MapEditorLib_export.h"

#include <string>

namespace NEditorDefaults
{
// A database path, shared by game installations and mods, rather than a local filename.
inline constexpr char COMMON_DEPLOY_TEMPLATE[] = "Reinforcements/DeployTemplate/Game/Common_DeployTemplate.xdb";

struct SOptions
{
	std::string szDeployTemplate = COMMON_DEPLOY_TEMPLATE;

	int operator&( IXmlSaver &xs )
	{
		xs.Add( "DeployTemplate", &szDeployTemplate );
		return 0;
	}
};

MAPEDITORLIB_EXPORT SOptions LoadOptions();
MAPEDITORLIB_EXPORT bool SaveOptions( SOptions options );
} // namespace NEditorDefaults
