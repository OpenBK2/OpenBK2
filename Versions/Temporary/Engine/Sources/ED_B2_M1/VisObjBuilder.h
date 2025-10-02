
#pragma once
#include "mapeditorlib/interface_commandhandler.h"
#include "Stats_B2_M1/Season.h"
#include "MapEditorLib/BuildDataBuilder.h"


class CVisObjBuilder : public CBuildDataBuilder, public ICommandHandler
{
	enum EResourceType
	{
		RT_MODEL			= 0,
		RT_MATERIAL		= 1,
		RT_TEXTURE		= 2,
		RT_GEOMETRY		= 3,
		RT_AIGEOMETRY	= 4,
		RT_SKELETON		= 5,
		RT_COUNT			= 6,
	};
	//
	static const char VISOBJ_TYPE_NAME[];
	static const char MODEL_TYPE_NAME[];
	static const char MATERIAL_TYPE_NAME[];
	static const char TEXTURE_TYPE_NAME[];
	static const char GEOMETRY_TYPE_NAME[];
	static const char AIGEOMETRY_TYPE_NAME[];
	static const char SKELETON_TYPE_NAME[];
	static const string RESOURCE_PREFIX[RT_COUNT];
	static const char MODEL_FILE_NAME_EXTENTION[];
	static const char TEXTURE_FILE_NAME_EXTENTION[];
	static const string BUILD_DATA_TYPE_NAME;
	//
	OBJECT_NOCOPY_METHODS( CVisObjBuilder );
	//
	static void GetSeasonedFolderName( string *pszFileName, NDb::ESeason eSeason );
	static void GetSeasonedFileName( string *pszFileName, NDb::ESeason eSeason );
	static void GetResourceFileName( string *pszResourceFileName, EResourceType eResourceType, const string &rszVisObjFileName );
	//
	bool AddVisObjEntry( const string &rszUniqueObjectName,
											 IManipulator *pBuildDataManipulator,
											 const string &rszMBFullFileName,
											 const string &rszTGAFullFileName,
											 NDb::ESeason eSeason );
	bool CreateVisObj( const string &rszVisObjFolder );
	//
	bool RemoveMaterial( const string &rszObjectTypeName, const string &rszObjectName );	
	bool RemoveTexture( const string &rszObjectTypeName, const string &rszObjectName );	
	bool RemoveGeometry( const string &rszObjectTypeName, const string &rszObjectName );	
	bool RemoveAIGeometry( const string &rszObjectTypeName, const string &rszObjectName );	
	bool RemoveSkeleton( const string &rszObjectTypeName, const string &rszObjectName );	
	bool RemoveAnimation( const string &rszObjectTypeName, const string &rszObjectName );	
	bool RemoveModel( const string &rszObjectTypeName, const string &rszObjectName );	
	//
	CVisObjBuilder();
	~CVisObjBuilder();

protected:
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView );

	//CBuildDataBuilder
	const string& GetBuildDataTypeName() { return BUILD_DATA_TYPE_NAME; }
	bool InternalInsertObject( string *pszObjectTypeName,
														 string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit,
														 IManipulator *pBuildDataManipulator );
	//CBuilderBase
	bool RemoveObject( const string &rszObjectTypeName, const string &rszObjectName );

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
};


