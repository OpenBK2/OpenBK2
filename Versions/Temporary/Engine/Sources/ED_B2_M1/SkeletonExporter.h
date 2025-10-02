#if !defined(__SKELETON_EXPORTER__)
#define __SKELETON_EXPORTER__
#pragma once

#include "../ED_Common/BasicSceneExporter.h"

class CSkeletonExporter : public CBasicSceneExporter
{
	OBJECT_NOCOPY_METHODS( CSkeletonExporter );
	// CRAP{ HASH_SET
	typedef hash_map<CDBID, int> CAnimationRefMap;
	// CRAP} HASH_SET
	CAnimationRefMap animations;
	//
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
	CSkeletonExporter() {}
};

#endif // !defined(__SKELETON_EXPORTER__)


