#pragma once

#include "MapEditorLib/Interface_Builder.h"

class CBuilderContainer : public IBuilderContainer
{
	OBJECT_NOCOPY_METHODS( CBuilderContainer );
	//
	typedef std::unordered_map<std::string, CPtr<IBuilder>> CBuilderMap;
	CBuilderMap builderMap;

	IBuilder *GetBuilder( const std::string &rszObjectTypeName );

public:
	CBuilderContainer() {}

	~CBuilderContainer() override {}

	// IBuilderContainer
	bool CanBuildObject( const std::string &rszObjectTypeName ) override;
	bool CanDefaultBuildObject( const std::string &rszObjectTypeName ) override;
	//
	void Create( const std::string &rszObjectTypeName ) override;
	void Destroy( const std::string &rszObjectTypeName ) override;
	//
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
	void GetDefaultFolder( const std::string &rszObjectTypeName, std::string *pszDefaultFolder ) override;
	bool FillBuildData( std::string *pszBuildDataTypeName,
	                    std::string *pszBuildDataName,
	                    SBuildDataParams *pBuildDataParams,
	                    IBuildDataCallback *pBuildDataCallback ) override;
	bool FillNewObjectName( SBuildDataParams *pBuildDataParams ) override;
};
