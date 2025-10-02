#if !defined(__BUILD_DATA_BUILDER__)
#define __BUILD_DATA_BUILDER__

#pragma once
#include "DefaultBuilder.h"

class CBuildDataBuilder : public CBuilderBase, public IBuildDataCallback
{
protected:
	virtual const string& GetBuildDataTypeName() = 0;
	virtual bool InternalInsertObject( string *pszObjectTypeName,
																		 string *pszUniqueObjectName,
																		 bool bFromMainMenu,
																		 bool *pbCanChangeObjectName,
																		 bool *pbNeedExport,
																		 bool *pbNeedEdit,
																		 IManipulator *pBuildDataManipulator ) = 0;
	virtual bool NeedBuildDataDialog() const { return true; }
public:
	// можно поменять значение по умолчанию для следужщего поля: ( *pbCanChangeObjectName ) = false;
	virtual bool InsertObject( string *pszObjectTypeName,
														 string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit );

	bool IsUniqueObjectName( const string &szObjectType, const string &szObjectName );
};

#endif // !defined(__BUILD_DATA_BUILDER__)

