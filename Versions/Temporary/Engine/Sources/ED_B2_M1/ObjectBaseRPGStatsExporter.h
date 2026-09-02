#pragma once

#include "StaticObjectRPGStatsExporter.h"

class CObjectBaseRPGStatsExporter : public CStaticObjectRPGStatsExporter
{
	bool ExportDynamicDebris( IManipulator *pManipulator, const std::string &szObjectName );
protected:
	virtual bool NeedCreatePassability() = 0;
public:
	// CStaticObjectRPGStatsExporter
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
};



