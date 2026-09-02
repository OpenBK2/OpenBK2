#pragma once

#include "StoreObjectExporter.h"

class CParticleExporter : public CStoreObjectExporter
{
	OBJECT_NOCOPY_METHODS( CParticleExporter );
	//
	std::string szScriptText;
	bool bNeedExport;
	// dst_name => temp_name
	typedef std::unordered_map<std::string, std::string> CExportedFilesMap;
	CExportedFilesMap exportedFilesMap;
	//
	bool ImportInfoToDBAfterRefs( const std::string &szObjName,
																									 const std::string &szDstFileName,
																									 IManipulator *pManipulator );
	//
	CParticleExporter();
public:
	// IExporter
	bool StartExport( const std::string &rszObjectTypeName, bool bForce );
	void FinishExport( const std::string &rszObjectTypeName, bool bForce );
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
};



