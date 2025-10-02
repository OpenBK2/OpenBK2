#pragma once

#include "../MapEditorLib/Interface_Exporter.h"
#include "../MapEditorLib/InteractiveMaya.h"
#include "../MapEditorLib/TextMapSettings.h"

class CBasicExporter : public IExporter
{
	CPtr<CInteractiveMaya> pMayaProcess;
	string szObjectTypeName;
	mutable CTextMapSettings textMapSettings; // to load on-demand from const functions
	//
	bool LoadExporterSettings() const;
protected:
	void Log( ELogOutputType eLogOutputType, const string &szText ) const;
	const char *GetTextTemplate( const char *pszTemplateName ) const;
	bool ExecuteMayaScript( const string &szScript );
public:
	CBasicExporter() {}
	// IExporter
	bool StartExport( const string &rszObjectTypeName, bool bForce );
	void FinishExport( const string &rszObjectTypeName, bool bForce );
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType ) { return ER_SUCCESS; }
	// checker
	bool StartCheck( const string &rszObjectTypeName, bool bExport ) { return true; }
	void FinishCheck( const string &rszObjectTypeName, bool bExport ) {}
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bExport,
															EXPORT_TYPE exportType ) { return ER_SUCCESS; }
};


