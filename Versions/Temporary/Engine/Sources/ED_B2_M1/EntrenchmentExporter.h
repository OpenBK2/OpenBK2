#pragma once

#include "ED_Common/StoreObjectExporter.h"

class CEntrenchmentExporter : public IExporter
{
	OBJECT_NOCOPY_METHODS( CEntrenchmentExporter );

	CEntrenchmentExporter() {}

public:
	// IExporter
	bool StartExport( const std::string &rszObjectTypeName, bool bForce ) override
	{
		return true;
	}

	void FinishExport( const std::string &rszObjectTypeName, bool bForce ) override {}

	EXPORT_RESULT ExportObject( IManipulator *pManipulator,
	                            const std::string &rszObjectTypeName,
	                            const std::string &rszObjectName,
	                            bool bForce,
	                            EXPORT_TYPE exportType ) override;

	//
	bool StartCheck( const std::string &rszObjectTypeName, bool bExport ) override
	{
		return true;
	}

	void FinishCheck( const std::string &rszObjectTypeName, bool bExportk ) override {}

	EXPORT_RESULT CheckObject( IManipulator *pManipulator,
	                           const std::string &rszObjectTypeName,
	                           const std::string &rszObjectName,
	                           bool bExport,
	                           EXPORT_TYPE exportType ) override
	{
		return ER_SUCCESS;
	}
};
