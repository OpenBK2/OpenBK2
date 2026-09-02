#pragma once

#include "ED_Common/BasicSceneExporter.h"

class CGeometryExporter : public CBasicSceneExporter
{
	OBJECT_NOCOPY_METHODS( CGeometryExporter );
	//
	bool FormScript( std::string *pScriptText,
									 const std::string &szTypeName,
									 const std::string &szObjName, 
		               const std::string &szDstPath,
									 const std::string &szSrcPath,
		               IManipulator *pManipulator );
	bool ImportInfoToDBBeforeRefs( const std::string &szGeomObjName, 
		                             const std::string &szSrcScenePath,
																 const std::string &szDstFileName,
																 IManipulator *pManipulator );
	const char *GetAddPath() const;
	//
	EXPORT_RESULT CustomCheck( const std::string &szTypeName,
														 const std::string &szObjName, 
														 const std::string &szSrcScenePath,
														 const std::string &szDestinationPath, 
														 IManipulator *pManipulator );
	//
	CGeometryExporter() {}
};



