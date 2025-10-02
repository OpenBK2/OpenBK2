#if !defined(__DEFAULT_BUILDER_BASE__)
#define __DEFAULT_BUILDER_BASE__
#pragma once

#include "BuilderBase.h"


class CDefaultBuilderBase : public CBuilderBase
{
public:
	// CBuilderBase
	virtual bool InsertObject( string *pszObjectTypeName, string *pszUniqueObjectName, bool bFromMainMenu, bool *pbCanChangeObjectName, bool *pbNeedExport, bool *pbNeedEdit );
};

#endif // !defined(__DEFAULT_BUILDER_BASE__)


