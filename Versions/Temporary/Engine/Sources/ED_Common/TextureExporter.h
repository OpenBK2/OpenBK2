#if !defined(__TEXTURE_EXPORTER__)
#define __TEXTURE_EXPORTER__
#pragma once

#include "../MapEditorLib/Interface_Exporter.h"

class CTextureExporter : public IExporter
{
	OBJECT_NOCOPY_METHODS( CTextureExporter );

	list<string> invalidObjects;

	bool CheckDestination( const string &szFileName, const string &szObjectName ) const;
	
	CTextureExporter() {}
public:
	// IExporter
	bool StartExport( const string &rszObjectTypeName, bool bForce ) { return true; }
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
														 EXPORT_TYPE exportType );
};

#endif // !defined(__TEXTURE_EXPORTER__)


