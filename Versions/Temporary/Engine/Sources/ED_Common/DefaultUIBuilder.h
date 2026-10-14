
#pragma once
#include "BaseUIBuilder.h"

class CDefaultUIBuilder : public CBaseUIBuilder
{
	static const std::string DEFAULT_BUILD_DATA_TYPE_NAME;

	OBJECT_NOCOPY_METHODS( CDefaultUIBuilder );

	CDefaultUIBuilder() {}

protected:
	// IBuildDataCallback
	bool
	IsValidBuildData( IManipulator *pBuildDataManipulator, std::string *pszDescription, IView *pBuildDataView ) override
	{
		return false;
	}

	//CBuildDataBuilder
	const std::string &GetBuildDataTypeName() override
	{
		return DEFAULT_BUILD_DATA_TYPE_NAME;
	}

	bool InternalInsertObject( std::string *pszObjectTypeName,
	                           std::string *pszUniqueObjectName,
	                           bool bFromMainMenu,
	                           bool *pbCanChangeObjectName,
	                           bool *pbNeedExport,
	                           bool *pbNeedEdit,
	                           IManipulator *pBuildDataManipulator ) override
	{
		return false;
	}

	bool NeedBuildDataDialog() const override
	{
		return false;
	}
};
