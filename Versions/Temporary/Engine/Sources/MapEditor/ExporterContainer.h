#pragma once

#include "MapEditorLib/Interface_Exporter.h"

class CExporterContainer : public IExporterContainer
{
	OBJECT_NOCOPY_METHODS( CExporterContainer );
	//
	typedef std::unordered_map<std::string, CPtr<IExporter>> CExporterMap;
	typedef std::list<CPtr<IExportTool>> CExportToolList;
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
	EXPORT_RESULT ExportObject( const std::string &rszExportTypeName,
	                            IManipulator *pManipulator,
	                            const std::string &rszObjectTypeName,
	                            const std::string &rszObjectName,
	                            bool bExport,
	                            bool bForce );

public:
	CExporterContainer() {}

	~CExporterContainer() override {}

	// IExporterContainer
	bool CanExportObject( const std::string &rszObjectTypeName ) override;
	IExporter *GetExporter( const std::string &rszObjectTypeName ) override;
	//
	void Create( const std::string &rszObjectTypeName ) override;
	void Destroy( const std::string &rszObjectTypeName ) override;
	//
	void RegisterExportTool( IExportTool *pExportTool ) override;
	void UnRegisterExportTool( IExportTool *pExportTool ) override;
	//
	bool
	StartExport( const std::string &rszObjectTypeName, bool bForce, bool bStartTools, bool bExportReferences ) override;
	void
	FinishExport( const std::string &rszObjectTypeName, bool bForce, bool bFinishTools, bool bExportReferences ) override;
	EXPORT_RESULT ExportObject( IManipulator *pManipulator,
	                            const std::string &rszObjectTypeName,
	                            const std::string &rszObjectName,
	                            bool bForce,
	                            bool bExportReferences ) override;
	//
	bool StartCheck( const std::string &rszObjectTypeName, bool bStartTools, bool bCheckReferences ) override;
	void FinishCheck( const std::string &rszObjectTypeName, bool bFinishTools, bool bCheckReferences ) override;
	EXPORT_RESULT CheckObject( IManipulator *pManipulator,
	                           const std::string &rszObjectTypeName,
	                           const std::string &rszObjectName,
	                           bool bCheckReferences ) override;
	//
	EXPORT_RESULT GetExportResult( const std::string &rszObjectRefName ) override;
	EXPORT_RESULT GetExportResult( const std::string &rszObjectTypeName, const std::string &rszObjectName ) override;
};
