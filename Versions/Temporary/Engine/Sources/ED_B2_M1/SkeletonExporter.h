#pragma once

#include "ED_Common/BasicSceneExporter.h"

class CSkeletonExporter : public CBasicSceneExporter
{
	OBJECT_NOCOPY_METHODS( CSkeletonExporter );
	// CRAP{ HASH_SET
	typedef std::unordered_map<CDBID, int> CAnimationRefMap;
	// CRAP} HASH_SET
	CAnimationRefMap animations;
	//
	const char *GetAddPath() const;
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
	EXPORT_RESULT CustomCheck( const std::string &szTypeName,
														 const std::string &szObjName, 
														 const std::string &szSrcScenePath,
														 const std::string &szDestinationPath, 
														 IManipulator *pManipulator );
	CSkeletonExporter() {}
};



