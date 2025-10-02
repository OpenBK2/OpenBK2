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

	hash_map<int, string> materials;

	const bool UpdateVisObj( IManipulator* pManipulator, const string &szRefName, const vector<SAnimationInfo> &frames, const int nStage );
	const bool CopyModel( const string &szOldModelName, const string &szNewName, const string &szRoot );
	const bool CreateVisObj( IManipulator* pManipulator, const string &szObjectName, const string &szRoot );
	const bool ProcessVisObj( IManipulator *pManipulator, const string &szRefName, const string &szNewName, const string &szRoot, const vector<SAnimationInfo> &frames, const int nStage );
	const bool UpdateEntrancesAndSlots( IManipulator *pManipulator, const string &szObjectName );
	
	const bool CreateTexture( const string &szTextureName, const string &szFileName );
	const string GetMaterial( const string &szModelName, const string &szModelPath, const int nMaterial, const bool bTransparent, const bool bReflective );

	const bool UpdateModels( IManipulator *pManipulator, const string &szRefName, const string &szObjectName, const int nMaterial );
	const bool UpdateSectionMaterials( IManipulator *pManipulator, const string &szObjectName );

protected:
	bool NeedCreatePassability() { return true; }
public:
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
};



