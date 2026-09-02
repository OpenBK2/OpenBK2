#pragma once

#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/InteractiveMaya.h"
#include "MapEditorLib/TextMapSettings.h"

#include "ED_Common_export.h"

class ED_COMMON_EXPORT CBasicExporter : public IExporter
{
	CPtr<CInteractiveMaya> pMayaProcess;
	std::string szObjectTypeName;
	mutable CTextMapSettings textMapSettings; // to load on-demand from const functions
	//
	bool LoadExporterSettings() const;
protected:
	void Log( ELogOutputType eLogOutputType, const std::string &szText ) const;
	const char *GetTextTemplate( const char *pszTemplateName ) const;
	bool ExecuteMayaScript( const std::string &szScript );
public:
	CBasicExporter() {}
	// IExporter
	bool StartExport( const std::string &rszObjectTypeName, bool bForce );
	void FinishExport( const std::string &rszObjectTypeName, bool bForce );
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType ) { return ER_SUCCESS; }
	// checker
	bool StartCheck( const std::string &rszObjectTypeName, bool bExport ) { return true; }
	void FinishCheck( const std::string &rszObjectTypeName, bool bExport ) {}
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bExport,
															EXPORT_TYPE exportType ) { return ER_SUCCESS; }
};


