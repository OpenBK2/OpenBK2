#pragma once

#include "ObjectBaseRPGStatsExporter.h"

class CBuildingRPGStatsExporter : public CObjectBaseRPGStatsExporter
{
	OBJECT_NOCOPY_METHODS( CBuildingRPGStatsExporter );
	
	CBuildingRPGStatsExporter() {}

	struct SAnimationInfo
	{
		int nStartTime;
		int nEndTime;

		SAnimationInfo() : nStartTime( -1 ), nEndTime( -1 ) {}
		SAnimationInfo( const int _nStartTime, const int _nEndTime ) : nStartTime( _nStartTime ), nEndTime( _nEndTime ) {}
	};

	std::unordered_map<int, std::string> materials;

	const bool UpdateVisObj( IManipulator* pManipulator, const std::string &szRefName, const std::vector<SAnimationInfo> &frames, const int nStage );
	const bool CopyModel( const std::string &szOldModelName, const std::string &szNewName, const std::string &szRoot );
	const bool CreateVisObj( IManipulator* pManipulator, const std::string &szObjectName, const std::string &szRoot );
	const bool ProcessVisObj( IManipulator *pManipulator, const std::string &szRefName, const std::string &szNewName, const std::string &szRoot, const std::vector<SAnimationInfo> &frames, const int nStage );
	const bool UpdateEntrancesAndSlots( IManipulator *pManipulator, const std::string &szObjectName );
	
	const bool CreateTexture( const std::string &szTextureName, const std::string &szFileName );
	const std::string GetMaterial( const std::string &szModelName, const std::string &szModelPath, const int nMaterial, const bool bTransparent, const bool bReflective );

	const bool UpdateModels( IManipulator *pManipulator, const std::string &szRefName, const std::string &szObjectName, const int nMaterial );
	const bool UpdateSectionMaterials( IManipulator *pManipulator, const std::string &szObjectName );

protected:
	bool NeedCreatePassability() { return true; }
public:
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
};



