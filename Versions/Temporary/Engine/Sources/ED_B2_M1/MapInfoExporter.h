#if !defined(__MAPINFO_EXPORTER__)
#define __MAPINFO_EXPORTER__

#pragma once
#include "..\mapeditorlib\interface_commandhandler.h"
#include "..\MapEditorLib\Interface_Exporter.h"

class CMapInfoExporter : public IExporter, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CMapInfoExporter );
	//
	struct SReGenerateGeometry
	{
		bool operator()( const string &rszObjectTypeName, const CDBID &rDBID );
	};

	struct SCheck
	{
		bool operator()( const string &rszObjectTypeName, const CDBID &rDBID );
	};

	CMapInfoExporter();
	~CMapInfoExporter();
public:

	// IExporter
	bool StartExport( const string &rszObjectTypeName, bool bForce );
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

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
};

#endif // !defined(__MAPINFO_EXPORTER__)

