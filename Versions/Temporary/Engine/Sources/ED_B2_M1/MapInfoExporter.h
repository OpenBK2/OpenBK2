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
	~CMapInfoExporter();
public:

	// IExporter
	bool StartExport( const std::string &rszObjectTypeName, bool bForce );
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

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
};


