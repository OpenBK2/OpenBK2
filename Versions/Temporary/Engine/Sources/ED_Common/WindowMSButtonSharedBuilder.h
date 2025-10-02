#if !defined(__WINDOW_MSBUTTON_SHARED_BUILDER__)
#define __WINDOW_MSBUTTON_SHARED_BUILDER__

#pragma once
#include "..\MapEditorLib\BuildDataBuilder.h"

class CWindowMSButtonSharedBuilder : public CBuildDataBuilder
{
	static const char WINDOW_MSBUTTON_SHARED_TYPE_NAME[];
	static const char BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME[];
	static const char TEXTURE_TYPE_NAME[];
	static const string BUILD_DATA_TYPE_NAME;

	string szNormalStateTexObjectName;

	OBJECT_NOCOPY_METHODS( CWindowMSButtonSharedBuilder );

	CWindowMSButtonSharedBuilder() {}

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

private:
	bool CreateVisualState( const string &rszUniqueObjectName, IManipulator *pBuildDataManipulator, IManipulator * pMSBManipulator, int index );
	bool CreateButtonState( const string &rszUniqueObjectName, IManipulator *pBuildDataManipulator, IManipulator * pMSBManipulator, int index, const char * szSuffixName, bool bNormalState );
};

#endif // !defined(__WINDOW_MSBUTTON_SHARED_BUILDER__)
