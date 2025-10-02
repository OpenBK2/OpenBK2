#pragma once
#include "..\mapeditorlib\interface_commandhandler.h"
#include "..\MapEditorLib\BuildDataBuilder.h"

class CAcksBuilder : public CDefaultBuilderBase, public IBuildDataCallback, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CAcksBuilder );

	CAcksBuilder();
	~CAcksBuilder();

	bool UpdateAckSets( const string &rszAnimationFolder );
protected:
	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView );
	bool IsUniqueObjectName( const string &szObjectType, const string &szObjectName );
};


