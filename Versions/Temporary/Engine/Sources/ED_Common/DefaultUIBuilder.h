
#pragma once
#include "BaseUIBuilder.h"


class CDefaultUIBuilder : public CBaseUIBuilder
{
	static const string DEFAULT_BUILD_DATA_TYPE_NAME;

	OBJECT_NOCOPY_METHODS( CDefaultUIBuilder );

	CDefaultUIBuilder() {}

protected:
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
	{
		return false;
	}

	//CBuildDataBuilder
	const string& GetBuildDataTypeName() { return DEFAULT_BUILD_DATA_TYPE_NAME; }
	bool InternalInsertObject( string *pszObjectTypeName,
														 string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit,
														 IManipulator *pBuildDataManipulator ) { return false; }
	bool NeedBuildDataDialog() const { return false; }
};


