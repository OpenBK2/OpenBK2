#pragma once

#include "System_export.h"


namespace NBinResources
{
	SYSTEM_EXPORT bool IsEmptyGUID( const GUID &uid );
	SYSTEM_EXPORT std::string GUIDToString( const GUID &uid );
	std::string GetBinaryFileName( const std::string &rszDirPrefix, const int nRecordID, const GUID &uid );
	std::string GetExistentBinaryFileName( const std::string &rszDirPrefix, const int nRecordID, const GUID &uid );
}


