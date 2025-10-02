#if !defined(__ENTRENCHMENT_EXPORTER__)
#define __ENTRENCHMENT_EXPORTER__
#pragma once

#include "..\ED_Common\StoreObjectExporter.h"

class CEntrenchmentExporter : public IExporter
{
	OBJECT_NOCOPY_METHODS( CEntrenchmentExporter );

	CEntrenchmentExporter() {}

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
	void FinishCheck( const string &rszObjectTypeName, bool bExportk ) {}
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
														 const string &rszObjectTypeName,
														 const string &rszObjectName,
														 bool bExport,
														 EXPORT_TYPE exportType ) { return ER_SUCCESS; }
};

#endif // !defined(__ENTRENCHMENT_EXPORTER__)
