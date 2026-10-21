#pragma once

//#include "../ED_Common/StoreObjectExporter.h"
#include "ED_Common/BasicExporter.h"

#include "ED_B2_M1_export.h"

class CHPObjectRPGStatsExporter : public CBasicExporter
{
	void CreateSingleIcons( IManipulator *pMan, const std::string &szObjectTypeName, const std::string &szObjectName );

public:
	// Import light metadata independently of surface-point and icon generation.
	ED_B2_M1_EXPORT static void ExportSingleLightFX( IManipulator *pMan );

	EXPORT_RESULT ExportObject( IManipulator *pManipulator,
	                            const std::string &rszObjectTypeName,
	                            const std::string &rszObjectName,
	                            bool bForce,
	                            EXPORT_TYPE exportType ) override;
};
