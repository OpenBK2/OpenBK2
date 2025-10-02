#if !defined(__GEOMETRY_EXPORTER__)
#define __GEOMETRY_EXPORTER__
#pragma once

#include "..\ED_Common\BasicSceneExporter.h"

class CGeometryExporter : public CBasicSceneExporter
{
	OBJECT_NOCOPY_METHODS( CGeometryExporter );
	//
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
	const char *GetAddPath() const;
	//
	EXPORT_RESULT CustomCheck( const string &szTypeName,
														 const string &szObjName, 
														 const string &szSrcScenePath,
														 const string &szDestinationPath, 
														 IManipulator *pManipulator );
	//
	CGeometryExporter() {}
};

#endif // !defined(__GEOMETRY_EXPORTER__)

