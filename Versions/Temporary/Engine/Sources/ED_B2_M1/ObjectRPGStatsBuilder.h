#if !defined(__OBJECTRPGSTATS_BUILDER__)
#define __OBJECTRPGSTATS_BUILDER__

#pragma once
#include "../MapEditorLib/BuildDataBuilder.h"

class CObjectRPGStatsBuilder : public CBuildDataBuilder
{
	static const string BUILD_DATA_TYPE_NAME;

	OBJECT_NOCOPY_METHODS( CObjectRPGStatsBuilder );

	CObjectRPGStatsBuilder() {}

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

#endif // !defined(__OBJECTRPGSTATS_BUILDER__)

