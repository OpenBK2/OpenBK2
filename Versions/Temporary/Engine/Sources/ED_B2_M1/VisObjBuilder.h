#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "Stats_B2_M1/Season.h"
#include "MapEditorLib/BuildDataBuilder.h"

#include <cstdint>

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
	static const std::string RESOURCE_PREFIX[RT_COUNT];
	static const char MODEL_FILE_NAME_EXTENTION[];
	static const char TEXTURE_FILE_NAME_EXTENTION[];
	static const std::string BUILD_DATA_TYPE_NAME;
	//
	OBJECT_NOCOPY_METHODS( CVisObjBuilder );
	//
	static void GetSeasonedFolderName( std::string *pszFileName, NDb::ESeason eSeason );
	static void GetSeasonedFileName( std::string *pszFileName, NDb::ESeason eSeason );
	static void GetResourceFileName( std::string *pszResourceFileName, EResourceType eResourceType, const std::string &rszVisObjFileName );
	//
	bool AddVisObjEntry( const std::string &rszUniqueObjectName,
											 IManipulator *pBuildDataManipulator,
											 const std::string &rszMBFullFileName,
											 const std::string &rszTGAFullFileName,
											 NDb::ESeason eSeason );
	bool CreateVisObj( const std::string &rszVisObjFolder );
	//
	bool RemoveMaterial( const std::string &rszObjectTypeName, const std::string &rszObjectName );	
	bool RemoveTexture( const std::string &rszObjectTypeName, const std::string &rszObjectName );	
	bool RemoveGeometry( const std::string &rszObjectTypeName, const std::string &rszObjectName );	
	bool RemoveAIGeometry( const std::string &rszObjectTypeName, const std::string &rszObjectName );	
	bool RemoveSkeleton( const std::string &rszObjectTypeName, const std::string &rszObjectName );	
	bool RemoveAnimation( const std::string &rszObjectTypeName, const std::string &rszObjectName );	
	bool RemoveModel( const std::string &rszObjectTypeName, const std::string &rszObjectName );	
	//
	CVisObjBuilder();
	~CVisObjBuilder();

protected:
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, std::string *pszDescription, IView *pBuildDataView );

	//CBuildDataBuilder
	const std::string& GetBuildDataTypeName() { return BUILD_DATA_TYPE_NAME; }
	bool InternalInsertObject( std::string *pszObjectTypeName,
														 std::string *pszUniqueObjectName,
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,
														 bool *pbNeedExport,
														 bool *pbNeedEdit,
														 IManipulator *pBuildDataManipulator );
	//CBuilderBase
	bool RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName );

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
};


