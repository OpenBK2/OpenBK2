#pragma once
#include "..\MapEditorLib\Interface_Exporter.h"

class CFontExporter : public IExporter
{
	OBJECT_NOCOPY_METHODS( CFontExporter );

	CFontExporter() {}
public:
	// IExporter
	bool StartExport( const string &rszObjectTypeName, bool bForce ) { return true; }
	void FinishExport( const string &rszObjectTypeName, bool bForce ) {}
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	//
	bool StartCheck( const string &rszObjectTypeName, bool bExport ) { return true; }
	void FinishCheck( const string &rszObjectTypeName, bool bExport ) {}
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bExport,
															EXPORT_TYPE exportType ) { return ER_SUCCESS; }
};
