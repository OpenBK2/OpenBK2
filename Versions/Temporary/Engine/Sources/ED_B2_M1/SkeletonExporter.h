#pragma once

#include "ED_Common/BasicSceneExporter.h"

#include "ED_B2_M1_export.h"

class CSkeletonExporter : public CBasicSceneExporter
{
	OBJECT_NOCOPY_METHODS( CSkeletonExporter );
	// CRAP{ HASH_SET
	typedef std::unordered_map<CDBID, int> CAnimationRefMap;
	// CRAP} HASH_SET
	CAnimationRefMap animations;
	//
	const char *GetAddPath() const override;
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
	EXPORT_RESULT CustomCheck( const std::string &szTypeName,
	                           const std::string &szObjName,
	                           const std::string &szSrcScenePath,
	                           const std::string &szDestinationPath,
	                           IManipulator *pManipulator ) override;

	CSkeletonExporter() {}

public:
	// Import metadata independently of the export UI (also used by headless checks).
	ED_B2_M1_EXPORT bool ImportGltfInfo( IManipulator *pManipulator ) override;
};
