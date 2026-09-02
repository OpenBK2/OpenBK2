#pragma once

#include "MapEditorLib/Interface_Builder.h"

class CBuilderContainer : public IBuilderContainer
{
	OBJECT_NOCOPY_METHODS( CBuilderContainer );
	//
	typedef std::unordered_map<std::string, CPtr<IBuilder> > CBuilderMap;
	CBuilderMap builderMap;

	IBuilder *GetBuilder( const std::string &rszObjectTypeName );

public:
	CBuilderContainer() {}
	~CBuilderContainer() {}

	// IBuilderContainer
	bool CanBuildObject( const std::string &rszObjectTypeName );
	bool CanDefaultBuildObject( const std::string &rszObjectTypeName );
	//
	void Create( const std::string &rszObjectTypeName );
	void Destroy( const std::string &rszObjectTypeName );
	//
	bool InsertObject( std::string *pszObjectTypeName,
										 std::string *pszUniqueObjectName,
										 bool bFromMainMenu,
										 bool *pbCanChangeObjectName,
										 bool *pbNeedExport,
										 bool *pbNeedEdit );
	bool CopyObject( const std::string &rszObjectTypeName,
									 const std::string &rszDestination,
									 const std::string &rszSource );
	bool RenameObject( const std::string &rszObjectTypeName,
										 const std::string &rszDestination,
										 const std::string &rszSource );
	bool RemoveObject( const std::string &rszObjectTypeName,
											const std::string &rszObjectName );
	void GetDefaultFolder( const std::string &rszObjectTypeName, std::string *pszDefaultFolder );
	bool FillBuildData( std::string *pszBuildDataTypeName,
											std::string *pszBuildDataName,
											SBuildDataParams *pBuildDataParams,					
											IBuildDataCallback *pBuildDataCallback );
	bool FillNewObjectName( SBuildDataParams *pBuildDataParams );
};



