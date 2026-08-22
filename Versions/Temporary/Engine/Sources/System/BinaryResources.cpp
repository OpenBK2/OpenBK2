#include "stdafx.h"

#include "BinaryResources.h"

#include <boost/uuid/uuid_io.hpp>

#include "VFSOperations.h"
#include "Misc/StrProc.h"

#include <fmt/format.h>

namespace NBinResources
{

std::string GetBinaryFileName( const std::string &rszDirPrefix, const int nRecordID, const boost::uuids::uuid &uid )
{
	std::string szDirPrefix = rszDirPrefix;
	NStr::TrimRight( szDirPrefix, '\\' );
	if (uid.is_nil())
	{
		DebugTrace( "Empty resource GUID: %s\\%d", szDirPrefix.c_str(), nRecordID );
		return fmt::format( "{}\\{}", szDirPrefix, nRecordID );
	}
	return fmt::format( "{}\\{}", szDirPrefix, boost::uuids::to_string(uid) );
}

std::string GetExistentBinaryFileName( const std::string &rszDirPrefix, const int nRecordID, const boost::uuids::uuid &uid )
{
	std::string szName = GetBinaryFileName( rszDirPrefix, nRecordID, uid );

	if ( NVFS::GetMainVFS()->DoesFileExist( szName ) )
		return szName;

	std::string szDirPrefix = rszDirPrefix;
	NStr::TrimRight( szDirPrefix, '\\' );
	return fmt::format( "{}\\{}", szDirPrefix, nRecordID );
}

}

