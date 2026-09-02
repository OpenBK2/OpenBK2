#pragma once

//#include "../ED_Common/StoreObjectExporter.h"
#include "ED_Common/BasicExporter.h"

class CHPObjectRPGStatsExporter : public CBasicExporter
{
	void ExportSingleLightFX( IManipulator *pMan );
	void CreateSingleIcons( IManipulator *pMan, 
													const std::string &szObjectTypeName, 
													const std::string &szObjectName );
public:
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
};


