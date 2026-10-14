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
	                 IManipulator *pManipulator ) override;
	bool ImportInfoToDBBeforeRefs( const std::string &szGeomObjName,
	                               const std::string &szSrcScenePath,
	                               const std::string &szDstFileName,
	                               IManipulator *pManipulator ) override;
	const char *GetAddPath() const override;
	//
	EXPORT_RESULT CustomCheck( const std::string &szTypeName,
	                           const std::string &szObjName,
	                           const std::string &szSrcScenePath,
	                           const std::string &szDestinationPath,
	                           IManipulator *pManipulator ) override;

	//
	CGeometryExporter() {}
};
