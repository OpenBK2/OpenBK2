#pragma once
#include "MapEditorLib/Interface_Exporter.h"

class CFontExporter : public IExporter
{
	OBJECT_NOCOPY_METHODS( CFontExporter );

	CFontExporter() {}
public:
	// IExporter
	bool StartExport( const std::string &rszObjectTypeName, bool bForce ) { return true; }
	void FinishExport( const std::string &rszObjectTypeName, bool bForce ) {}
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	//
	bool StartCheck( const std::string &rszObjectTypeName, bool bExport ) { return true; }
	void FinishCheck( const std::string &rszObjectTypeName, bool bExport ) {}
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bExport,
															EXPORT_TYPE exportType ) { return ER_SUCCESS; }
};

