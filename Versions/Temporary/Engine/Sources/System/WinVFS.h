#pragma once

#include "System_export.h"


#include "VFS.h"

namespace NVFS
{
	SYSTEM_EXPORT IVFS* CreateWinVFS( const std::string &szBasePath );
	SYSTEM_EXPORT IFileCreator* CreateWinFileCreator( const std::string &szBasePath );

	SYSTEM_EXPORT bool GetWinFileStats( struct SFileStats *pStats, const std::string &szPath );
	SYSTEM_EXPORT bool DoesWinFileExist( const std::string &szPath );
	SYSTEM_EXPORT void VFSSegmentProfiler();
}


