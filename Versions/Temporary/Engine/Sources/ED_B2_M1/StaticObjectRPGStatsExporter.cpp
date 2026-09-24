#include "stdafx.h"

#include "StaticObjectRPGStatsExporter.h"

EXPORT_RESULT CStaticObjectRPGStatsExporter::ExportObject( IManipulator* pManipulator,
																													const std::string &rszObjectTypeName,
																													const std::string &rszObjectName,
																													bool bForce,
																													EXPORT_TYPE exportType )
{
	// Preserve failures from the shared object exporter.
	return CHPObjectRPGStatsExporter::ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType );
}


