#include "stdafx.h"

#include "StaticObjectRPGStatsExporter.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


EXPORT_RESULT CStaticObjectRPGStatsExporter::ExportObject( IManipulator* pManipulator,
																													const string &rszObjectTypeName,
																													const string &rszObjectName,
																													bool bForce,
																													EXPORT_TYPE exportType )
{
	CHPObjectRPGStatsExporter::ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType );
	//
	return ER_SUCCESS;
}


