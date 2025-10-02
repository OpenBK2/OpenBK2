#pragma once

#include "System_export.h"


#include "VFS.h"

namespace NVFS
{
	SYSTEM_EXPORT void SetMainVFS( IVFS *pVFS );
	SYSTEM_EXPORT void SetMainFileCreator( IFileCreator *pFileCreator );

	SYSTEM_EXPORT IVFS* GetMainVFS();
	SYSTEM_EXPORT IFileCreator* GetMainFileCreator();
}


