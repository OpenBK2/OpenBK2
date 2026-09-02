
#pragma once

#include "MapEditorLib/BuildDataBuilder.h"

class CStaticDebrisSetBuilder : public CBuildDataBuilder
{
	static const std::string BUILD_DATA_TYPE_NAME;

	OBJECT_NOCOPY_METHODS( CStaticDebrisSetBuilder );

	CStaticDebrisSetBuilder() {}
protected:
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, std::string *pszDescription, IView *pBuildDataView );
	
	// CBuildDataBuilder
	const std::string& GetBuildDataTypeName() { return BUILD_DATA_TYPE_NAME; }
	bool InternalInsertObject( std::string *pszObjectTypeName,
														 std::string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit,
														 IManipulator *pBuildDataManipulator );
};


