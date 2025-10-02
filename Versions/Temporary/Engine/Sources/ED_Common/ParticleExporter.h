#if !defined(__PARTICLE_EXPORTER__)
#define __PARTICLE_EXPORTER__
#pragma once

#include "StoreObjectExporter.h"

class CParticleExporter : public CStoreObjectExporter
{
	OBJECT_NOCOPY_METHODS( CParticleExporter );
	//
	string szScriptText;
	bool bNeedExport;
	// dst_name => temp_name
	typedef hash_map<string, string> CExportedFilesMap;
	CExportedFilesMap exportedFilesMap;
	//
	bool CParticleExporter::ImportInfoToDBAfterRefs( const string &szObjName,
																									 const string &szDstFileName,
																									 IManipulator *pManipulator );
	//
	CParticleExporter();
public:
	// IExporter
	bool StartExport( const string &rszObjectTypeName, bool bForce );
	void FinishExport( const string &rszObjectTypeName, bool bForce );
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
};

#endif // !defined(__PARTICLE_EXPORTER__)

