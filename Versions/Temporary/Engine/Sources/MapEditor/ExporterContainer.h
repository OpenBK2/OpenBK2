#pragma once

#include "MapEditorLib/Interface_Exporter.h"

class CExporterContainer : public IExporterContainer
{
	OBJECT_NOCOPY_METHODS( CExporterContainer );
	//
	typedef std::unordered_map<std::string, CPtr<IExporter> > CExporterMap;
	typedef std::list<CPtr<IExportTool> > CExportToolList;
	//
	CExporterMap exporterMap;
	CExportToolList exportToolList;

	bool StartExport( const std::string &rszExportTypeName,
										const std::string &rszObjectTypeName,
										bool bExport,
										bool bForce,
										bool bStartTools );
	void FinishExport( const std::string &rszExportTypeName,
										 const std::string &rszObjectTypeName,
										 bool bExport,
										 bool bForce,
										 bool bFinishTools );
	EXPORT_RESULT	ExportObject( const std::string &rszExportTypeName,
															IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bExport,
															bool bForce );

public:
	CExporterContainer() {}
	~CExporterContainer() {}

	// IExporterContainer
	bool CanExportObject( const std::string &rszObjectTypeName );
	IExporter* GetExporter( const std::string &rszObjectTypeName );
	//
	void Create( const std::string &rszObjectTypeName );
	void Destroy( const std::string &rszObjectTypeName );
	//
	void RegisterExportTool( IExportTool *pExportTool );
	void UnRegisterExportTool( IExportTool *pExportTool );
	//
	bool StartExport( const std::string &rszObjectTypeName, bool bForce, bool bStartTools, bool bExportReferences );
	void FinishExport( const std::string &rszObjectTypeName, bool bForce, bool bFinishTools, bool bExportReferences );
	EXPORT_RESULT	ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															bool bExportReferences );
	//
	bool StartCheck( const std::string &rszObjectTypeName, bool bStartTools, bool bCheckReferences );
	void FinishCheck( const std::string &rszObjectTypeName, bool bFinishTools, bool bCheckReferences );
	EXPORT_RESULT	CheckObject( IManipulator* pManipulator,
														 const std::string &rszObjectTypeName,
														 const std::string &rszObjectName,
														 bool bCheckReferences );
	//
	EXPORT_RESULT GetExportResult( const std::string &rszObjectRefName );
	EXPORT_RESULT GetExportResult( const std::string &rszObjectTypeName, const std::string &rszObjectName );
};



