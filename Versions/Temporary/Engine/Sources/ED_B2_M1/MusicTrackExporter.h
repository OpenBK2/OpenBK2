#pragma once


#include "MapEditorLib/Interface_Exporter.h"

class CMusicTrackExporter : public IExporter
{
	OBJECT_NOCOPY_METHODS( CMusicTrackExporter );

	CMusicTrackExporter() {}
public:

	// IExporter
	virtual bool StartExport( const string &rszObjectTypeName, bool bForce ) { return true; }
	virtual void FinishExport( const string &rszObjectTypeName, bool bForce ) {}
	virtual EXPORT_RESULT ExportObject( IManipulator* pManipulator,
																			const string &rszObjectTypeName,
																			const string &rszObjectName,
																			bool bForce,
																			EXPORT_TYPE exportType );
	virtual bool StartCheck( const string &rszObjectTypeName, bool bExport ) { return true; }
	virtual void FinishCheck( const string &rszObjectTypeName, bool bExport ) {}
	virtual EXPORT_RESULT CheckObject( IManipulator* pManipulator,
																		 const string &rszObjectTypeName,
																		 const string &rszObjectName,
																		 bool bExport,
																		 EXPORT_TYPE exportType ) { return ER_SUCCESS; }
};


