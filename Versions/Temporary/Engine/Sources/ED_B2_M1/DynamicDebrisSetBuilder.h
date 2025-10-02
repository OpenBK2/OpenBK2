#if !defined(__DYNAMICDEBRISSET_BUILDER__)
#define __DYNAMICDEBRISSET_BUILDER__

#pragma once

#include "..\MapEditorLib\BuildDataBuilder.h"

class CDynamicDebrisSetBuilder : public CBuildDataBuilder
{
	static const string BUILD_DATA_TYPE_NAME;

	OBJECT_NOCOPY_METHODS( CDynamicDebrisSetBuilder );

	CDynamicDebrisSetBuilder() {}
protected:
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView );
	
	// CBuildDataBuilder
	const string& GetBuildDataTypeName() { return BUILD_DATA_TYPE_NAME; }
	bool InternalInsertObject( string *pszObjectTypeName,
														 string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit,
														 IManipulator *pBuildDataManipulator );
};

#endif // !defined(__DYNAMICDEBRISSET_BUILDER__)
