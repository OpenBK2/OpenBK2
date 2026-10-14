#pragma once

#include "VFS.h"

#include "System_export.h"

namespace NVFS
{
SYSTEM_EXPORT IVFS *CreateWinVFS( const std::string &szBasePath );
SYSTEM_EXPORT IFileCreator *CreateWinFileCreator( const std::string &szBasePath );

SYSTEM_EXPORT bool GetWinFileStats( struct SFileStats *pStats, const std::string &szPath );
SYSTEM_EXPORT bool DoesWinFileExist( const std::string &szPath );
SYSTEM_EXPORT void VFSSegmentProfiler();
} // namespace NVFS
