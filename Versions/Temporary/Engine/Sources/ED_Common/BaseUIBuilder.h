#if !defined(__BASE_UI_BUILDER__)
#define __BASE_UI_BUILDER__

#pragma once
#include "../MapEditorLib/BuildDataBuilder.h"


class CBaseUIBuilder : public CBuildDataBuilder
{
protected:
	// hidden constuctor
	CBaseUIBuilder() {}

protected:
	// IBuilder
	virtual bool CopyObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource );
	virtual bool RemoveObject( const string &rszObjectTypeName, const string &rszObjectName );
};

#endif // !defined(__BASE_UI_BUILDER__)

