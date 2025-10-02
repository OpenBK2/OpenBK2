#if !defined(__OBJECTRPGSTATS_EXPORTER__)
#define __OBJECTRPGSTATS_EXPORTER__

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
														 const string &rszObjectTypeName,
														 const string &rszObjectName,
														 bool bExport,
														 EXPORT_TYPE exportType );
};

#endif // !defined(__OBJECTRPGSTATS_EXPORTER__)

