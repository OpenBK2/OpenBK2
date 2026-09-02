#pragma once

#include "HPObjectRPGStatsExporter.h"

class CStaticObjectRPGStatsExporter : public CHPObjectRPGStatsExporter
{
public:
	// IExporter
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	//
};


