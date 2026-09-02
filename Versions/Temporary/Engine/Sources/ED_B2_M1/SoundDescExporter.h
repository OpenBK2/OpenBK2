#pragma once

#include "MapEditorLib/Interface_Exporter.h"

class CSoundDescExporter : public IExporter
{
	OBJECT_NOCOPY_METHODS( CSoundDescExporter );

	CSoundDescExporter() {}
public:
		
	// IExporter
	virtual bool StartExport( const std::string &rszObjectTypeName, bool bForce ) { return true; }
	virtual void FinishExport( const std::string &rszObjectTypeName, bool bForce ) {}
	virtual EXPORT_RESULT ExportObject( IManipulator* pManipulator,
																			const std::string &rszObjectTypeName,
																			const std::string &rszObjectName,
																			bool bForce,
																			EXPORT_TYPE exportType );
	virtual bool StartCheck( const std::string &rszObjectTypeName, bool bExport ) { return true; }
	virtual void FinishCheck( const std::string &rszObjectTypeName, bool bExport ) {}
	virtual EXPORT_RESULT CheckObject( IManipulator* pManipulator,
																			const std::string &rszObjectTypeName,
																			const std::string &rszObjectName,
																			bool bExport,
																			EXPORT_TYPE exportType ) { return ER_SUCCESS; }
};


