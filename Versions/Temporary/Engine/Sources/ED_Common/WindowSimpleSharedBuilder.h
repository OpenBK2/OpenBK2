#if !defined(__WINDOW_SIMPLE_SHARED_BUILDER__)
#define __WINDOW_SIMPLE_SHARED_BUILDER__

#pragma once
#include "BaseUIBuilder.h"

class CWindowSimpleSharedBuilder : public CBaseUIBuilder
{
	static const char BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME[];
	static const char TEXTURE_TYPE_NAME[];
	static const string BUILD_DATA_TYPE_NAME;

	OBJECT_NOCOPY_METHODS( CWindowSimpleSharedBuilder );

	CWindowSimpleSharedBuilder() {}

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

#endif // !defined(__WINDOW_SIMPLE_SHARED_BUILDER__)
