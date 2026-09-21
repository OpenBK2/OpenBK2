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
		// Collected and handed over in one call rather than renamed one at a
		// time. Each rename has to find every object pointing at the one being
		// renamed, which is a pass over the database, so a loop of RenameObject
		// would make a folder cost one pass per file in it.
		std::vector< std::pair<CDBID, CDBID> > renames;
		for ( std::vector<std::string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it )
		{
			if ( it->size() > 4 && NFile::ComparePathEq(it->size() - 4, 4, *it, 0, 4, szXDB) )
			{
				std::string szNewObjFileName = *it;
				szNewObjFileName.replace( 0, szOldName.size(), szNewName );
				renames.push_back( std::make_pair( CDBID(*it), CDBID(szNewObjFileName) ) );
			}
		}
		bRes = NDb::RenameObjects( renames ) && bRes;
	}
	else
		bRes = bRes && NDb::RenameObject( CDBID(szOldName), CDBID(szNewName) );
	//
	return bRes;
}

}
