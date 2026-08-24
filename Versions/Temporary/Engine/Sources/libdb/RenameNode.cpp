#include "stdafx.h"

#include "libdb/EditorDb.h"

#include "System/FilePath.h"
#include "System/VFSOperations.h"

namespace NFolderManipulator
{

static bool IsFolderName( const std::string &szName )
{
	return !szName.empty() && ( szName[szName.size() - 1] == '\\' || szName[szName.size() - 1] == '/' );
}

bool RenameNode( const std::string &szOldName, const std::string &szNewName )
{
	bool bRes = true;
	if ( IsFolderName(szOldName) )
	{
		std::vector<std::string> filenames;
		NVFS::GetMainVFS()->GetAllFileNames( &filenames, szOldName );
		const std::string szXDB = ".xdb";
		for ( std::vector<std::string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it )
		{
			if ( it->size() > 4 && NFile::ComparePathEq(it->size() - 4, 4, *it, 0, 4, szXDB) )
			{
				std::string szNewObjFileName = *it;
				szNewObjFileName.replace( 0, szOldName.size(), szNewName );
				bRes = bRes && NDb::RenameObject( CDBID(*it), CDBID(szNewObjFileName) );
			}
		}
	}
	else
		bRes = bRes && NDb::RenameObject( CDBID(szOldName), CDBID(szNewName) );
	//
	return bRes;
}

}
