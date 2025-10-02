#pragma once

//#include "../ED_Common/StoreObjectExporter.h"
#include "../ED_Common/BasicExporter.h"

class CHPObjectRPGStatsExporter : public CBasicExporter
{
	void ExportSingleLightFX( IManipulator *pMan );
	void CreateSingleIcons( IManipulator *pMan, 
													const string &szObjectTypeName, 
													const string &szObjectName );
public:
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
};


