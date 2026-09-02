#pragma once

#include "Interface_Builder.h"


class CBuilderBase : public IBuilder
{
public:
	// IBuilder
	virtual bool InsertObject( std::string *pszObjectTypeName, std::string *pszUniqueObjectName, bool bFromMainMenu, bool *pbCanChangeObjectName, bool *pbNeedExport, bool *pbNeedEdit );
	virtual bool CopyObject( const std::string &rszObjectTypeName, const std::string &rszDestination, const std::string &rszSource );
	virtual bool RenameObject( const std::string &rszObjectTypeName, const std::string &rszDestination, const std::string &rszSource );
	virtual bool RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName );
	virtual void GetDefaultFolder( const std::string &rszObjectTypeName, std::string *pszDefaultFolder ) { if ( pszDefaultFolder ) { pszDefaultFolder->clear(); } }
};



