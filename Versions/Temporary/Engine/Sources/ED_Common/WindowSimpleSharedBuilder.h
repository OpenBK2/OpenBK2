
#pragma once
#include "BaseUIBuilder.h"

class CWindowSimpleSharedBuilder : public CBaseUIBuilder
{
	static const char BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME[];
	static const char TEXTURE_TYPE_NAME[];
	static const std::string BUILD_DATA_TYPE_NAME;

	OBJECT_NOCOPY_METHODS( CWindowSimpleSharedBuilder );

	CWindowSimpleSharedBuilder() {}

protected:
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, std::string *pszDescription, IView *pBuildDataView );

	//CBuildDataBuilder
	const std::string& GetBuildDataTypeName() { return BUILD_DATA_TYPE_NAME; }
	bool InternalInsertObject( std::string *pszObjectTypeName,
														 std::string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit,
														 IManipulator *pBuildDataManipulator );
};


