
#pragma once
#include "MapEditorLib/BuildDataBuilder.h"

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
	static const std::string BUILD_DATA_TYPE_NAME;
	static const std::string COPY_DATA_TYPE_NAME;
	static const std::string MAPINFO_TYPE_NAME;
	static const std::string MAPINFO_DEFAULT_FOLDER;

	OBJECT_NOCOPY_METHODS( CMapInfoBuilder );

	CMapInfoBuilder() {}
protected:
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, std::string *pszDescription, IView *pBuildDataView );
	
	// CBuildDataBuilder
	const std::string& GetBuildDataTypeName() { return BUILD_DATA_TYPE_NAME; }
	bool InternalInsertObject( std::string *pszObjectTypeName,
														 std::string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit,
														 IManipulator *pBuildDataManipulator );

	// IBuilder
	void GetDefaultFolder( const std::string &rszObjectTypeName, std::string *pszDefaultFolder );
	bool InsertObject( std::string *pszObjectTypeName,
										 std::string *pszUniqueObjectName,
										 bool bFromMainMenu,
										 bool *pbCanChangeObjectName,
										 bool *pbNeedExport,
										 bool *pbNeedEdit );
	bool CopyObject( const std::string &rszObjectTypeName, const std::string &rszDestination, const std::string &rszSource );
	bool RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName );

	// members
	bool IsValidDataBuilder( IManipulator *pBuildDataManipulator, std::string *pszDescription, IView *pBuildDataView );
	bool IsValidDataCopier( IManipulator *pBuildDataManipulator, std::string *pszDescription, IView *pBuildDataView );
	bool InternalCopy( const std::string &rszObjectTypeName, const std::string &rszDestination, const std::string &rszSource, IManipulator *pBuildDataManipulator );


public:
	static bool EnsureMinimapMaterialAndTexture( IManipulator *pObjectManipulator, const CDBID &dbid );
	static bool MakeMinimapMaterialAndTexture( std::string *pszObjectName, const std::string &szFolder );
};


