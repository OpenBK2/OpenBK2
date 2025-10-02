#pragma once

#include "BasicSceneExporter.h"


class CAIGeometryExporter : public CBasicSceneExporter
{
	OBJECT_NOCOPY_METHODS( CAIGeometryExporter );

	bool FormScript( string *pScriptText,
									 const string &szTypeName,
									 const string &szObjName, 
		               const string &szDstPath,
									 const string &szSrcPath,
		               IManipulator *pManipulator );
	bool ImportInfoToDBBeforeRefs( const string &szGeomObjName, 
		                             const string &szSrcScenePath,
																 const string &szDstFileName,
																 IManipulator *pManipulator );
	EXPORT_RESULT CustomCheck( const string &szTypeName,
														 const string &szObjName, 
														 const string &szSrcScenePath,
														 const string &szDestinationPath, 
														 IManipulator *pManipulator );
	const char *GetAddPath() const;

	CAIGeometryExporter() {  }
};


