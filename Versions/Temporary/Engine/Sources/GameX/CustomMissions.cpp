#include "stdafx.h"

#include "CustomMissions.h"
#include "libdb/Db.h"
#include "SceneB2/TerraGen.h" // NScene::DoesTerrainFileExist
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/FilePath.h"
#include "System/VFS.h"
#include "System/VFSOperations.h"

#include <fmt/format.h>

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
		if ( filePath.size() > 4 && NFile::ComparePathEq( filePath.size() - 4, 4, filePath, 0, 4, szXDB ) )
		{
			const CDBID dbidObject = CDBID( *it );
			if ( NDb::GetClassTypeName( dbidObject ) == szType )
			{
				pDBIDs->push_back( dbidObject );
			}
		}
	}
}

bool IsMapPlayable( const NDb::SMapInfo *pMapInfo )
{
	return pMapInfo != 0 && NScene::DoesTerrainFileExist( pMapInfo );
}

bool VerifyMapPlayable( const NDb::SMapInfo *pMapInfo )
{
	if ( IsMapPlayable( pMapInfo ) )
	{
		return true;
	}
	// Both outputs: the console line is what a player typing "map" sees, and the
	// log is the only one a run started from the command line has.
	const std::string szMap = pMapInfo != 0 ? NDb::GetFileName( pMapInfo->GetDBID() ) : std::string( "<none>" );
	const auto message =
			fmt::format( "map \"{}\" cannot be started: no {} beside it", szMap, NScene::GetTerrainFileName() );
	csSystem << CC_ORANGE << message.c_str() << endl;
	if ( IConsoleBuffer *pConsole = Singleton<IConsoleBuffer>() )
	{
		pConsole->WriteASCII( CONSOLE_STREAM_CONSOLE, message.c_str() );
	}
	return false;
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

} // namespace NCustom
