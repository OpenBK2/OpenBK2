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
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	bool StartCheck( const std::string &rszObjectTypeName, bool bExport ) { return true; }
	void FinishCheck( const std::string &rszObjectTypeName, bool bExport ) {}
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bExport,
															EXPORT_TYPE exportType ) { return ER_SUCCESS; }
};

}

