#pragma once

#include "ED_Common/BasicExporter.h"
#include "ED_Common/Tools_Granny.h"

namespace NModelExporter
{

class CModelExporter : public CBasicExporter
{
	OBJECT_NOCOPY_METHODS( CModelExporter );
	//
	bool MakeMaterialsList( IManipulator* pModelMan, IManipulator* pGeomMan, CGrannyFileInfoGuard &pInfo );
	CModelExporter() {}
public:
	// IExporter
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	bool StartCheck( const string &rszObjectTypeName, bool bExport ) { return true; }
	void FinishCheck( const string &rszObjectTypeName, bool bExport ) {}
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bExport,
															EXPORT_TYPE exportType ) { return ER_SUCCESS; }
};

}

