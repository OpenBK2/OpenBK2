
#pragma once
#include "MapEditorLib/BuildDataBuilder.h"

class CBaseUIBuilder : public CBuildDataBuilder
{
protected:
	// hidden constuctor
	CBaseUIBuilder() {}

protected:
	// IBuilder
	bool CopyObject( const std::string &rszObjectTypeName,
	                 const std::string &rszDestination,
	                 const std::string &rszSource ) override;
	bool RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName ) override;
};
