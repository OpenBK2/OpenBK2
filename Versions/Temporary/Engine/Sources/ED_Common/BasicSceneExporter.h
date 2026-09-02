#pragma once

#include "BasicExporter.h"

class CBasicSceneExporter : public CBasicExporter
{
	virtual bool Validate( IManipulator *pManipulator );
	bool ExportFromMaya( const std::string &szTypeName, 
											 const std::string &szObjName,
											 const std::string &_szDstPath,
											 const std::string &_szSrcPath,
											 IManipulator *pManipulator );
	virtual const char *GetAddPath() const = 0;
	//
	virtual bool FormScript( std::string *pScriptText, 
													 const std::string &szTypeName,
													 const std::string &szObjName,
													 const std::string &szDstPath,
													 const std::string &szSrcPath,
													 IManipulator *pManipulator ) = 0;
	virtual bool ImportInfoToDBBeforeRefs( const std::string &szGeomObjName, 
		                                     const std::string &szSrcScenePath,
																				 const std::string &szDstFileName,
																				 IManipulator *pManipulator ) { return true; }
	virtual bool ImportInfoToDBAfterRefs( const std::string &szGeomObjName, 
		                                    const std::string &szSrcScenePath,
																				const std::string &szDstFileName,
																				IManipulator *pManipulator ) { return true; }
	virtual EXPORT_RESULT CustomCheck( const std::string &szTypeName, 
																		 const std::string &szObjName,
																		 const std::string &szSrcScenePath,
																		 const std::string &szDestinationPath,
																		 IManipulator *pManipulator ) { return ER_SUCCESS; }
protected:
	const char *GetScriptTemplate( const char *pszTemplateName ) const { return GetTextTemplate( pszTemplateName ); }
	//
	CBasicSceneExporter() {}
public:
	// IExporter
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	// checker
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bExport,
															EXPORT_TYPE exportType );
};


