#if !defined(__STATICDEBRISSET_BUILDER__)
#define __STATICDEBRISSET_BUILDER__

#pragma once

#include "../MapEditorLib/BuildDataBuilder.h"

class CStaticDebrisSetBuilder : public CBuildDataBuilder
{
	static const string BUILD_DATA_TYPE_NAME;

	OBJECT_NOCOPY_METHODS( CStaticDebrisSetBuilder );

	CStaticDebrisSetBuilder() {}
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

#endif // !defined(__STATICDEBRISSET_BUILDER__)

