#pragma once

#include "System_export.h"

#include <boost/uuid/uuid.hpp>

namespace NBinResources
{
	std::string GetBinaryFileName( const std::string &rszDirPrefix, int nRecordID, const boost::uuids::uuid &uid );
	std::string GetExistentBinaryFileName( const std::string &rszDirPrefix, int nRecordID, const boost::uuids::uuid &uid );
}
