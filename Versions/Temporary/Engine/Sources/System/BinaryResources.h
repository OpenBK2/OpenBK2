#pragma once

#include "System_export.h"


namespace NBinResources
{
	SYSTEM_EXPORT bool IsEmptyGUID( const GUID &uid );
	SYSTEM_EXPORT string GUIDToString( const GUID &uid );
	string GetBinaryFileName( const string &rszDirPrefix, const int nRecordID, const GUID &uid );
	string GetExistentBinaryFileName( const string &rszDirPrefix, const int nRecordID, const GUID &uid );
}


