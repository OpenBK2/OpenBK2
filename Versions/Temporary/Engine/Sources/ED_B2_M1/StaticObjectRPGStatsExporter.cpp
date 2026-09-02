#include "stdafx.h"

#include "StaticObjectRPGStatsExporter.h"

EXPORT_RESULT CStaticObjectRPGStatsExporter::ExportObject( IManipulator* pManipulator,
																													const std::string &rszObjectTypeName,
																													const std::string &rszObjectName,
																													bool bForce,
																													EXPORT_TYPE exportType )
{
	CHPObjectRPGStatsExporter::ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType );
	//
	return ER_SUCCESS;
}


