
#pragma once

#include "ObjectBaseRPGStatsExporter.h"

class CObjectRPGStatsExporter : public CObjectBaseRPGStatsExporter
{
	OBJECT_NOCOPY_METHODS( CObjectRPGStatsExporter );
	
	CObjectRPGStatsExporter() {}
protected:
	bool NeedCreatePassability() { return true; }
public:
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
														 const std::string &rszObjectTypeName,
														 const std::string &rszObjectName,
														 bool bExport,
														 EXPORT_TYPE exportType );
};


