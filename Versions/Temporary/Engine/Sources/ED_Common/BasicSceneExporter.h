#ifndef __BASICSCENESXPORTER_H__
#define __BASICSCENESXPORTER_H__
#pragma once

#include "BasicExporter.h"

class CBasicSceneExporter : public CBasicExporter
{
	virtual bool Validate( IManipulator *pManipulator );
	bool ExportFromMaya( const string &szTypeName, 
											 const string &szObjName,
											 const string &_szDstPath,
											 const string &_szSrcPath,
											 IManipulator *pManipulator );
	virtual const char *GetAddPath() const = 0;
	//
	virtual bool FormScript( string *pScriptText, 
													 const string &szTypeName,
													 const string &szObjName,
													 const string &szDstPath,
													 const string &szSrcPath,
													 IManipulator *pManipulator ) = 0;
	virtual bool ImportInfoToDBBeforeRefs( const string &szGeomObjName, 
		                                     const string &szSrcScenePath,
																				 const string &szDstFileName,
																				 IManipulator *pManipulator ) { return true; }
	virtual bool ImportInfoToDBAfterRefs( const string &szGeomObjName, 
		                                    const string &szSrcScenePath,
																				const string &szDstFileName,
																				IManipulator *pManipulator ) { return true; }
	virtual EXPORT_RESULT CustomCheck( const string &szTypeName, 
																		 const string &szObjName,
																		 const string &szSrcScenePath,
																		 const string &szDestinationPath,
																		 IManipulator *pManipulator ) { return ER_SUCCESS; }
protected:
	const char *GetScriptTemplate( const char *pszTemplateName ) const { return GetTextTemplate( pszTemplateName ); }
	//
	CBasicSceneExporter() {}
public:
	// IExporter
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	// checker
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bExport,
															EXPORT_TYPE exportType );
};

#endif

