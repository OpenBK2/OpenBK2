#pragma once

#include "BuilderBase.h"

#include "MapEditorLib_export.h"

class MAPEDITORLIB_EXPORT CDefaultBuilderBase : public CBuilderBase
{
public:
	// CBuilderBase
	virtual bool InsertObject( std::string *pszObjectTypeName, std::string *pszUniqueObjectName, bool bFromMainMenu, bool *pbCanChangeObjectName, bool *pbNeedExport, bool *pbNeedEdit );
};



