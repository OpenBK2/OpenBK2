#if !defined(__BUILDER__CONTAINER__)
#define __BUILDER__CONTAINER__
#pragma once

#include "..\MapEditorLib\Interface_Builder.h"

class CBuilderContainer : public IBuilderContainer
{
	OBJECT_NOCOPY_METHODS( CBuilderContainer );
	//
	typedef hash_map<string, CPtr<IBuilder> > CBuilderMap;
	CBuilderMap builderMap;

	IBuilder *GetBuilder( const string &rszObjectTypeName );

public:
	CBuilderContainer() {}
	~CBuilderContainer() {}

	// IBuilderContainer
	bool CanBuildObject( const string &rszObjectTypeName );
	bool CanDefaultBuildObject( const string &rszObjectTypeName );
	//
	void Create( const string &rszObjectTypeName );
	void Destroy( const string &rszObjectTypeName );
	//
	bool InsertObject( string *pszObjectTypeName,
										 string *pszUniqueObjectName,
										 bool bFromMainMenu,
										 bool *pbCanChangeObjectName,
										 bool *pbNeedExport,
										 bool *pbNeedEdit );
	bool CopyObject( const string &rszObjectTypeName,
									 const string &rszDestination,
									 const string &rszSource );
	bool RenameObject( const string &rszObjectTypeName,
										 const string &rszDestination,
										 const string &rszSource );
	bool RemoveObject( const string &rszObjectTypeName,
											const string &rszObjectName );
	void GetDefaultFolder( const string &rszObjectTypeName, string *pszDefaultFolder );
	bool FillBuildData( string *pszBuildDataTypeName,
											string *pszBuildDataName,
											SBuildDataParams *pBuildDataParams,					
											IBuildDataCallback *pBuildDataCallback );
	bool FillNewObjectName( SBuildDataParams *pBuildDataParams );
};

#endif // !defined(__BUILDER__CONTAINER__)

