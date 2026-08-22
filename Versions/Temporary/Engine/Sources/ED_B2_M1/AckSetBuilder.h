#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/BuildDataBuilder.h"

#include <cstdint>

class CAcksBuilder : public CDefaultBuilderBase, public IBuildDataCallback, public ICommandHandler
{
	OBJECT_NOCOPY_METHODS( CAcksBuilder );

	CAcksBuilder();
	~CAcksBuilder();

	bool UpdateAckSets( const string &rszAnimationFolder );
protected:
	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
	// IBuildDataCallback
	bool IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView );
	bool IsUniqueObjectName( const string &szObjectType, const string &szObjectName );
};


