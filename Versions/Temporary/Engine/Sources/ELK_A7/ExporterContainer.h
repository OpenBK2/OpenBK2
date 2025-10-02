#pragma once

#include "../MapEditorLib/Interface_Exporter.h"

class CExporterContainer : public IExporterContainer
{
	OBJECT_NOCOPY_METHODS( CExporterContainer );
	//
	typedef hash_map<string, CPtr<IExporter> > CExporterMap;
	typedef list<CPtr<IExportTool> > CExportToolList;
	//
	CExporterMap exporterMap;
	CExportToolList exportToolList;

	bool StartExport( const string &rszExportTypeName,
										const string &rszObjectTypeName,
										bool bExport,
										bool bForce,
										bool bStartTools );
	void FinishExport( const string &rszExportTypeName,
										 const string &rszObjectTypeName,
										 bool bExport,
										 bool bForce,
										 bool bFinishTools );
	EXPORT_RESULT	ExportObject( const string &rszExportTypeName,
															IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bExport,
															bool bForce );

public:
	CExporterContainer() {}
	~CExporterContainer() {}

	// IExporterContainer
	bool CanExportObject( const string &rszObjectTypeName );
	IExporter* GetExporter( const string &rszObjectTypeName );
	//
	void Create( const string &rszObjectTypeName );
	void Destroy( const string &rszObjectTypeName );
	//
	void RegisterExportTool( IExportTool *pExportTool );
	void UnRegisterExportTool( IExportTool *pExportTool );
	//
	bool StartExport( const string &rszObjectTypeName, bool bForce, bool bStartTools, bool bExportReferences );
	void FinishExport( const string &rszObjectTypeName, bool bForce, bool bFinishTools, bool bExportReferences );
	EXPORT_RESULT	ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															bool bExportReferences );
	//
	bool StartCheck( const string &rszObjectTypeName, bool bStartTools, bool bCheckReferences );
	void FinishCheck( const string &rszObjectTypeName, bool bFinishTools, bool bCheckReferences );
	EXPORT_RESULT	CheckObject( IManipulator* pManipulator,
														 const string &rszObjectTypeName,
														 const string &rszObjectName,
														 bool bCheckReferences );
	//
	EXPORT_RESULT GetExportResult( const string &rszObjectRefName );
	EXPORT_RESULT GetExportResult( const string &rszObjectTypeName, const string &rszObjectName );
};



