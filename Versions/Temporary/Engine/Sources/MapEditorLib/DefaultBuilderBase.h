#pragma once

#include "BuilderBase.h"


class CDefaultBuilderBase : public CBuilderBase
{
public:
	// CBuilderBase
	virtual bool InsertObject( std::string *pszObjectTypeName, std::string *pszUniqueObjectName, bool bFromMainMenu, bool *pbCanChangeObjectName, bool *pbNeedExport, bool *pbNeedEdit );
};



