#pragma once

#include "System_export.h"


#include "VFS.h"

namespace NVFS
{
	SYSTEM_EXPORT IVFS* CreateWinVFS( const string &szBasePath );
	SYSTEM_EXPORT IFileCreator* CreateWinFileCreator( const string &szBasePath );

	SYSTEM_EXPORT bool GetWinFileStats( struct SFileStats *pStats, const string &szPath );
	SYSTEM_EXPORT bool DoesWinFileExist( const string &szPath );
	SYSTEM_EXPORT void VFSSegmentProfiler();
}

