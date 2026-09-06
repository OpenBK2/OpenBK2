#include "stdafx.h"

#include "CreateModView.h"
#include "CreateMODDialog.h"

#include "Main/MODs.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/Tools_Resources.h"

#include <cstdlib>
#include <vector>

// Create MOD as it has always been: CCreateMODDialog, a CResizeDialog over
// IDD_CREATE_MOD with two edits, a description and two buttons.
//
// The two rules that decide whether a MOD can be created live here rather than
// on the dialog, because both implementations need them and a rule about what
// makes a valid MOD folder should not have two copies. CCreateMODDialog calls
// these now; nothing about what it does changed.

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


	bool RunMfc( IWidget *pParent, SNewMod *pMod )
	{
		if ( pMod == 0 )
		{
			return false;
		}
		// No parent, as the original had none: MFC then owns the dialog with the
		// main window, and passing one now would change which window is disabled.
		CCreateMODDialog createMODDialog;
		if ( createMODDialog.DoModal() != IDOK )
		{
			return false;
		}
		pMod->szFolderPath = createMODDialog.GetFolder();
		// Through the ANSI code page, which is what String2File did with these
		// internally before the boundary carried them wide. Same bytes on disk.
		MBSC2Unicode( &pMod->wszName, createMODDialog.GetName(), ::GetACP() );
		MBSC2Unicode( &pMod->wszDesc, createMODDialog.GetDescription(), ::GetACP() );
		return !pMod->szFolderPath.empty() && !pMod->wszName.empty();
	}


	bool Run( IWidget *pParent, SNewMod *pMod )
	{
#ifdef OBK2_WITH_WX
		// The same flag every migrated dialog follows, so a session runs either
		// the MFC set or the wx set. See SelectTablesViewMfc.cpp.
		const char *pszUseWx = std::getenv( "OBK2_WX_DIALOGS" );
		if ( pszUseWx != 0 && pszUseWx[0] != '0' && pszUseWx[0] != '\0' )
		{
			return RunWx( pParent, pMod );
		}
#endif
		return RunMfc( pParent, pMod );
	}
}
