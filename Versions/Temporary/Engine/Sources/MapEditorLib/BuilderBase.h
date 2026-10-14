#pragma once

#include "Interface_Builder.h"

#include "MapEditorLib_export.h"

class MAPEDITORLIB_EXPORT CBuilderBase : public IBuilder
{
public:
	// IBuilder
	bool InsertObject( std::string *pszObjectTypeName,
	                   std::string *pszUniqueObjectName,
	                   bool bFromMainMenu,
	                   bool *pbCanChangeObjectName,
	                   bool *pbNeedExport,
	                   bool *pbNeedEdit ) override;
	bool CopyObject( const std::string &rszObjectTypeName,
	                 const std::string &rszDestination,
	                 const std::string &rszSource ) override;
	bool RenameObject( const std::string &rszObjectTypeName,
	                   const std::string &rszDestination,
	                   const std::string &rszSource ) override;
	bool RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName ) override;

	void GetDefaultFolder( const std::string &rszObjectTypeName, std::string *pszDefaultFolder ) override
	{
		if ( pszDefaultFolder )
		{
			pszDefaultFolder->clear();
		}
	}
};
