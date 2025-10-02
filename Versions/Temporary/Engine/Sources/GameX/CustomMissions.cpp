#include "StdAfx.h"
#include "../System/FilePath.h"
#include "../System/VFS.h"
#include "../System/VFSOperations.h"
#include "../libdb/Db.h"
#include "CustomMissions.h"

namespace NCustom
{

void GetObjectsFromFolderByType( vector<CDBID> *pDBIDs, const NFile::CFilePath &path, const string &szType )
{
	vector<string> filenames;
	NVFS::GetMainVFS()->GetAllFileNames( &filenames, path );
	//
	pDBIDs->reserve( filenames.size() );
	const string szXDB = ".xdb";
	for ( vector<string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it )
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

void GetCustomMissions( vector<CDBID> *pDBIDs )
{
	GetObjectsFromFolderByType( pDBIDs, CUSTOM_MISSIONS_FOLDER, "MapInfo" );
}

void GetCustomCampaigns( vector<CDBID> *pDBIDs )
{
	GetObjectsFromFolderByType( pDBIDs, CUSTOM_CAMPAIGNS_FOLDER, "Campaign" );
}

void GetMultiplayerMaps( vector<CDBID> *pDBIDs )
{
	GetObjectsFromFolderByType( pDBIDs, MULTIPLAYER_MAPS_FOLDER, "MultiplayerMap" );
}

}
