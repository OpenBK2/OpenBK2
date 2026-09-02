
#pragma once
#include "DefaultBuilder.h"

#include "MapEditorLib_export.h"

class MAPEDITORLIB_EXPORT CBuildDataBuilder : public CBuilderBase, public IBuildDataCallback
{
protected:
	virtual const std::string& GetBuildDataTypeName() = 0;
	virtual bool InternalInsertObject( std::string *pszObjectTypeName,
																		 std::string *pszUniqueObjectName,
																		 bool bFromMainMenu,
																		 bool *pbCanChangeObjectName,
																		 bool *pbNeedExport,
																		 bool *pbNeedEdit,
																		 IManipulator *pBuildDataManipulator ) = 0;
	virtual bool NeedBuildDataDialog() const { return true; }
public:
	// можно поменять значение по умолчанию для следужщего поля: ( *pbCanChangeObjectName ) = false;
	virtual bool InsertObject( std::string *pszObjectTypeName,
														 std::string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit );

	bool IsUniqueObjectName( const std::string &szObjectType, const std::string &szObjectName );
};


