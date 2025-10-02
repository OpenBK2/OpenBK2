#pragma once

#include "Interface_Builder.h"


class CBuilderBase : public IBuilder
{
public:
	// IBuilder
	virtual bool InsertObject( string *pszObjectTypeName, string *pszUniqueObjectName, bool bFromMainMenu, bool *pbCanChangeObjectName, bool *pbNeedExport, bool *pbNeedEdit );
	virtual bool CopyObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource );
	virtual bool RenameObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource );
	virtual bool RemoveObject( const string &rszObjectTypeName, const string &rszObjectName );
	virtual void GetDefaultFolder( const string &rszObjectTypeName, string *pszDefaultFolder ) { if ( pszDefaultFolder ) { pszDefaultFolder->clear(); } }
};



