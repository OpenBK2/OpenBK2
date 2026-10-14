#pragma once

#include "MapEditorLib/Interface_Exporter.h"

class CSquadExporter : public IExporter
{
	OBJECT_NOCOPY_METHODS( CSquadExporter );

	//
	CSquadExporter() {}

public:
	bool StartExport( const std::string &rszObjectTypeName, bool bForce ) override
	{
		return true;
	}

	void FinishExport( const std::string &rszObjectTypeName, bool bForce ) override {}

	EXPORT_RESULT ExportObject( IManipulator *pManipulator,
	                            const std::string &rszObjectTypeName,
	                            const std::string &rszObjectName,
	                            bool bForce,
	                            EXPORT_TYPE exportType ) override
	{
		return ER_SUCCESS;
	}

	//
	bool StartCheck( const std::string &rszObjectTypeName, bool bExport ) override
	{
		return true;
	}

	void FinishCheck( const std::string &rszObjectTypeName, bool bExport ) override {}

	EXPORT_RESULT CheckObject( IManipulator *pManipulator,
	                           const std::string &rszObjectTypeName,
	                           const std::string &rszObjectName,
	                           bool bExport,
	                           EXPORT_TYPE exportType ) override;
};
