#pragma once

#include "VFS.h"

#include "System_export.h"

namespace NVFS
{
// bRememberMissingDirectories: answer "is there a loose copy" for every file under a
// directory found missing on disk from that one answer, for the life of the VFS. Saves
// a failed lookup per packed file opened, but a file written into such a directory later
// is never seen, so only the game, which writes no data while it runs, asks for it.
SYSTEM_EXPORT IVFS *CreateWinVFS( const std::string &szBasePath, bool bRememberMissingDirectories = false );
SYSTEM_EXPORT IFileCreator *CreateWinFileCreator( const std::string &szBasePath );

SYSTEM_EXPORT bool GetWinFileStats( struct SFileStats *pStats, const std::string &szPath );
SYSTEM_EXPORT bool DoesWinFileExist( const std::string &szPath );
SYSTEM_EXPORT void VFSSegmentProfiler();
} // namespace NVFS
