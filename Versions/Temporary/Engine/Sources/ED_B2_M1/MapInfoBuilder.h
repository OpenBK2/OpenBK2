
#pragma once
#include "../MapEditorLib/BuildDataBuilder.h"

class CMapInfoBuilder : public CBuildDataBuilder
{
	static const int MIN_PLAYER_COUNT;
	static const int MAX_PLAYER_COUNT;
	static const int MIN_TERRAIN_SIZE_X;
	static const int MIN_TERRAIN_SIZE_Y;
	static const int MAX_TERRAIN_SIZE_X;
	static const int MAX_TERRAIN_SIZE_Y;
	static const char TEXTURE_TYPE_NAME[];
	static const char MATERIAL_TYPE_NAME[];
	static const string BUILD_DATA_TYPE_NAME;
	static const string COPY_DATA_TYPE_NAME;
	static const string MAPINFO_TYPE_NAME;
	static const string MAPINFO_DEFAULT_FOLDER;

	OBJECT_NOCOPY_METHODS( CMapInfoBuilder );

	CMapInfoBuilder() {}
protected:
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView );
	
	// CBuildDataBuilder
	const string& GetBuildDataTypeName() { return BUILD_DATA_TYPE_NAME; }
	bool InternalInsertObject( string *pszObjectTypeName,
														 string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit,
														 IManipulator *pBuildDataManipulator );

	// IBuilder
	void GetDefaultFolder( const string &rszObjectTypeName, string *pszDefaultFolder );
	bool InsertObject( string *pszObjectTypeName,
										 string *pszUniqueObjectName,
										 bool bFromMainMenu,
										 bool *pbCanChangeObjectName,
										 bool *pbNeedExport,
										 bool *pbNeedEdit );
	bool CopyObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource );
	bool RemoveObject( const string &rszObjectTypeName, const string &rszObjectName );

	// members
	bool IsValidDataBuilder( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView );
	bool IsValidDataCopier( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView );
	bool InternalCopy( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource, IManipulator *pBuildDataManipulator );


public:
	static bool EnsureMinimapMaterialAndTexture( IManipulator *pObjectManipulator, const CDBID &dbid );
	static bool MakeMinimapMaterialAndTexture( string *pszObjectName, const string &szFolder );
};


