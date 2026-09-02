
#pragma once
#include "MapEditorLib/BuildDataBuilder.h"

class CWindowMSButtonSharedBuilder : public CBuildDataBuilder
{
	static const char WINDOW_MSBUTTON_SHARED_TYPE_NAME[];
	static const char BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME[];
	static const char TEXTURE_TYPE_NAME[];
	static const std::string BUILD_DATA_TYPE_NAME;

	std::string szNormalStateTexObjectName;

	OBJECT_NOCOPY_METHODS( CWindowMSButtonSharedBuilder );

	CWindowMSButtonSharedBuilder() {}

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

private:
	bool CreateVisualState( const std::string &rszUniqueObjectName, IManipulator *pBuildDataManipulator, IManipulator * pMSBManipulator, int index );
	bool CreateButtonState( const std::string &rszUniqueObjectName, IManipulator *pBuildDataManipulator, IManipulator * pMSBManipulator, int index, const char * szSuffixName, bool bNormalState );
};


