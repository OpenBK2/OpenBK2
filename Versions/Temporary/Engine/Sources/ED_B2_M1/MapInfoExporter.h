#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_Exporter.h"

#include <cstdint>

class CMapInfoExporter : public IExporter, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CMapInfoExporter );

	//
	struct SReGenerateGeometry
	{
		bool operator()( const std::string &rszObjectTypeName, const CDBID &rDBID );
	};

	struct SCheck
	{
		bool operator()( const std::string &rszObjectTypeName, const CDBID &rDBID );
	};

	CMapInfoExporter();
	~CMapInfoExporter() override;

public:
	// IExporter
	bool StartExport( const std::string &rszObjectTypeName, bool bForce ) override;

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

	void FinishCheck( const std::string &rszObjectTypeName, bool bExport ) override {}

	EXPORT_RESULT CheckObject( IManipulator *pManipulator,
	                           const std::string &rszObjectTypeName,
	                           const std::string &rszObjectName,
	                           bool bExport,
	                           EXPORT_TYPE exportType ) override
	{
		return ER_SUCCESS;
	}

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;
};
