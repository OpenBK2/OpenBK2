#pragma once

#include "System_export.h"


#include "VFS.h"

namespace NVFS
{
	SYSTEM_EXPORT void SetMainVFS( IVFS *pVFS );
	SYSTEM_EXPORT void SetMainFileCreator( IFileCreator *pFileCreator );

	SYSTEM_EXPORT IVFS* GetMainVFS();
	SYSTEM_EXPORT IFileCreator* GetMainFileCreator();

	// Resolve the physical save destination for diagnostics. Native saves replace
	// a completed temporary file, so failed writes leave the previous file intact.
	SYSTEM_EXPORT std::string GetWritePath( IFileCreator *pCreator, const std::string &szPath );
	SYSTEM_EXPORT bool WriteFile( IFileCreator *pCreator, const std::string &szPath,
		const CDataStream &data, std::string *pError = nullptr );
}


