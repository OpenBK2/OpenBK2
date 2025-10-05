#include "stdafx.h"
#include "System/FilePath.h"
#include "System/VFS.h"
#include "System/VFSOperations.h"
#include "libdb/Db.h"
#include "CustomMissions.h"

namespace NCustom
{

void GetObjectsFromFolderByType( std::vector<CDBID> *pDBIDs, const NFile::CFilePath &path, const std::string &szType )
{
	std::vector<std::string> filenames;
	NVFS::GetMainVFS()->GetAllFileNames( &filenames, path );
	//
	pDBIDs->reserve( filenames.size() );
	const std::string szXDB = ".xdb";
	for ( std::vector<std::string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it )
	{
		NFile::CFilePath filePath = *it;
		if ( filePath.size() > 4 && NFile::ComparePathEq(filePath.size() - 4, 4, filePath, 0, 4, szXDB) )
		{
			const CDBID dbidObject = CDBID( *it );
			if ( NDb::GetClassTypeName(dbidObject) == szType )
				pDBIDs->push_back( dbidObject );
		}
	}
}

void GetCustomMissions( std::vector<CDBID> *pDBIDs )
{
	GetObjectsFromFolderByType( pDBIDs, CUSTOM_MISSIONS_FOLDER, "MapInfo" );
}

void GetCustomCampaigns( std::vector<CDBID> *pDBIDs )
{
	GetObjectsFromFolderByType( pDBIDs, CUSTOM_CAMPAIGNS_FOLDER, "Campaign" );
}

void GetMultiplayerMaps( std::vector<CDBID> *pDBIDs )
{
	GetObjectsFromFolderByType( pDBIDs, MULTIPLAYER_MAPS_FOLDER, "MultiplayerMap" );
}

}
