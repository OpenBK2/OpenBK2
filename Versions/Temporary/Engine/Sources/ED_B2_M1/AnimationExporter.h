#pragma once

#include "ED_Common/BasicSceneExporter.h"

class CAnimationExporter : public CBasicSceneExporter
{
	OBJECT_NOCOPY_METHODS( CAnimationExporter );

	const char *GetAddPath() const;
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

	CAnimationExporter() {}
};



