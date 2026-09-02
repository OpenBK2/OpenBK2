#pragma once

#include "System_export.h"

#include <boost/uuid/uuid.hpp>

namespace NBinResources
{
	SYSTEM_EXPORT std::string GetBinaryFileName( const std::string &rszDirPrefix, int nRecordID, const boost::uuids::uuid &uid );
	SYSTEM_EXPORT std::string GetExistentBinaryFileName( const std::string &rszDirPrefix, int nRecordID, const boost::uuids::uuid &uid );
}
