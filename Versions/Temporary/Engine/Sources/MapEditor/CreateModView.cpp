#include "stdafx.h"

#include "CreateModView.h"

#include "Main/MODs.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/StringManager.h"

#include <vector>

// The two rules that decide whether a MOD can be created, kept apart from the
// dialog (CreateModViewWx.cpp): a rule about what makes a valid MOD folder is
// not about drawing.

namespace NCreateMod
{
	std::string MakeFolderPath( const std::string &rszFolderName )
	{
		std::string szFolder;
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		if ( pUserData == 0 )
		{
			return szFolder;
		}
		// The data storage folder with its trailing separator dropped, then its
		// parent: Mods sits beside Data, not inside it.
		szFolder = pUserData->constUserData.szDataStorageFolder;
		if ( !szFolder.empty() )
		{
			szFolder = szFolder.substr( 0, szFolder.size() - 1 );
		}
		const size_t nPos = szFolder.rfind( '\\' );
		if ( nPos != std::string::npos && nPos > 0 )
		{
			szFolder = szFolder.substr( 0, nPos );
		}
		szFolder += "\\Mods\\" + rszFolderName + "\\";
		return szFolder;
	}


	bool IsFolderNameFree( const std::string &rszFolderName )
	{
		if ( rszFolderName.empty() ||
				 rszFolderName.find_first_of( "/\\" ) != std::string::npos )
		{
			return false;
		}
		const std::string szFolder = MakeFolderPath( rszFolderName );
		std::vector<NMOD::SMOD> modList;
		NMOD::GetAllMODs( &modList );
		for ( size_t nIndex = 0; nIndex < modList.size(); ++nIndex )
		{
			if ( CStringManager::Compare( modList[nIndex].szFullFolderPath, szFolder,
																		true, true, false ) == 0 )
			{
				return false;
			}
		}
		return true;
	}
}
