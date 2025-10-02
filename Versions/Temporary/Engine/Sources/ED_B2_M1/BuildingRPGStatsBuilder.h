#pragma once

#include "MapEditorLib/BuildDataBuilder.h"

class CBuildingRPGStatsBuilder : public CBuildDataBuilder
{
	static const string BUILD_DATA_TYPE_NAME;

	OBJECT_NOCOPY_METHODS( CBuildingRPGStatsBuilder );

	string szPreviousDBType;
	CBuildingRPGStatsBuilder() {}

protected:
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView );

	//CBuildDataBuilder
	const string& GetBuildDataTypeName() { return BUILD_DATA_TYPE_NAME; }
	bool InternalInsertObject( string *pszObjectTypeName,
														 string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit,
														 IManipulator *pBuildDataManipulator );
};


