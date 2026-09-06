#include "stdafx.h"
#include "MapEditorLib/MfcWidget.h"
#include <fmt/printf.h>

#include "MODContainer.h"
#include "CreateModView.h"
#include "OpenMODDialog.h"
#include "OpenModView.h"
#include "Main/MODs.h"
#include "libdb/Db.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_Progress.h"

bool CMODContainer::CanNewMOD()
{
	return true;
}


bool CMODContainer::CanOpenMOD()
{
	std::vector<NMOD::SMOD> modList;
	NMOD::GetAllMODs( &modList );
	return ( !modList.empty() );
}


bool CMODContainer::CanCloseMOD()
{
	return ( NMOD::DoesAnyMODAttached() );
}


bool CMODContainer::NewMOD()
{
	if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
	{
		return false;
	}
	// Which dialog answers is NCreateMod's business, not this container's. The
	// two questions the MFC pair asked -- accepted? and is what came back
	// usable? -- are one question, and the dialog already refused to accept an
	// unusable answer.
	CWndWidget ownerWidget( AfxGetMainWnd() );
	NCreateMod::SNewMod newMod;
	if ( NCreateMod::Run( &ownerWidget, &newMod ) )
	{
		const std::string &szMODFolder = newMod.szFolderPath;
		NProgress::Create( true );
		CString strPM;
		strPM.LoadString( IDS_PM_CREATE_MOD );
		NProgress::SetMessage( fmt::sprintf( strPM.GetString(), szMODFolder.c_str() ) );
		NProgress::SetRange( 0, 2 );
		//
		// Создать файлы с именем и описанием
		//
		// The wide overload: these files are UTF-16 with a BOM either way, and
		// the narrow one only reached that by converting through ::GetACP()
		// first. The text is already wide by the time it gets here.
		String2File( newMod.wszName, szMODFolder + "name.txt", true );
		String2File( newMod.wszDesc, szMODFolder + "desc.txt", true );
		// Открыть новый мод
		NMOD::InstantAttachMOD( szMODFolder, NDb::DATABASE_MODE_EDITOR );
		NProgress::IteratePosition(); // 1
		Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_RELOAD, true );
		//
		SSWTParams swtParams;
		swtParams.dwFlags = SWT_MOD;
		swtParams.bFillMODFromBase = true;
		Singleton<IMainFrameContainer>()->Get()->SetWindowTitle( swtParams );
		//
		if ( Singleton<IUserDataContainer>() && Singleton<IUserDataContainer>()->Get() )
		{
			Singleton<IUserDataContainer>()->Get()->szOpenedMODFolder = szMODFolder;
		}
		NProgress::IteratePosition(); //2
		NProgress::Destroy();
		return true;
	}
	return false;
}


bool CMODContainer::OpenMOD()
{
	if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
	{
		return false;
	}
	// Which dialog answers is NOpenMod's business, not this container's.
	// The two questions the MFC pair asked -- accepted? and was anything
	// chosen? -- are one question, so they are one call.
	NMOD::SMOD mod;
	{
		CWndWidget ownerWidget( AfxGetMainWnd() );
		if ( NOpenMod::Run( &ownerWidget, &mod ) )
		{
			NProgress::Create( true );
			CString strPM;
			strPM.LoadString( IDS_PM_OPEN_MOD );
			NProgress::SetMessage( fmt::sprintf( strPM.GetString(), mod.szFullFolderPath.c_str() ) );
			NProgress::SetRange( 0, 2 );
			//
			NMOD::InstantAttachMOD( mod.szFullFolderPath, NDb::DATABASE_MODE_EDITOR );
			NProgress::IteratePosition(); // 1
			Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_RELOAD, true );
			//
			SSWTParams swtParams;
			swtParams.dwFlags = SWT_MOD;
			swtParams.bFillMODFromBase = true;
			Singleton<IMainFrameContainer>()->Get()->SetWindowTitle( swtParams );
			//
			if ( Singleton<IUserDataContainer>() && Singleton<IUserDataContainer>()->Get() )
			{
				Singleton<IUserDataContainer>()->Get()->szOpenedMODFolder = mod.szFullFolderPath;
			}
			NProgress::IteratePosition(); //2
			NProgress::Destroy();
			return true;
		}
	}
	return false;
}


void CMODContainer::CloseMOD()
{
	if ( NMOD::DoesAnyMODAttached() )
	{
		if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
		{
			return;
		}
		NProgress::Create( true );
		CString strPM;
		strPM.LoadString( IDS_PM_CLOSE_MOD );
		NProgress::SetMessage( fmt::sprintf( strPM.GetString(), Singleton<IUserDataContainer>()->Get()->szOpenedMODFolder.c_str() ) );
		NProgress::SetRange( 0, 2 );
		//
		NMOD::InstantAttachMOD( "", NDb::DATABASE_MODE_EDITOR );
		NProgress::IteratePosition(); //1
		Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_RELOAD, true );
		//
		SSWTParams swtParams;
		swtParams.dwFlags = SWT_MOD;
		swtParams.bFillMODFromBase = true;
		Singleton<IMainFrameContainer>()->Get()->SetWindowTitle( swtParams );

		if ( Singleton<IUserDataContainer>() && Singleton<IUserDataContainer>()->Get() )
		{
			Singleton<IUserDataContainer>()->Get()->szOpenedMODFolder.clear();
		}
		NProgress::IteratePosition(); //2
		NProgress::Destroy();
	}
}

/**

bool CMODContainer::IsValidFolder( const std::string &rszFolder )
{
	if ( NMOD::DoesAnyMODAttached() )
	{
		return false;
	}
	else
	{
		return false;
	}
}


bool CMODContainer::IsValidPath( const std::string &rszPath )
{
	std::string szFolder;
	CStringManager::SplitFileName( &szFolder, 0, 0, rszPath );
	return IsValidFolder( szFolder );
}
/**/

std::string CMODContainer::GetDataFolder( SUserData::ENormalizePathType eNormalizePathType )
{
	if ( NMOD::DoesAnyMODAttached() )
	{
		switch ( eNormalizePathType )
		{
			default:
			case SUserData::NPT_EXPORT_SOURCE:
			case SUserData::NPT_START:
				return Singleton<IUserDataContainer>()->Get()->GetPath( eNormalizePathType );
			case SUserData::NPT_DATA_STORAGE:
			case SUserData::NPT_EXPORT_DESTINATION:
			{
				NMOD::SMOD mod;
				NMOD::GetAttachedMOD( &mod );
				return mod.szFullFolderPath;
			}
		}
		return std::string();
	}
	else
	{
		return Singleton<IUserDataContainer>()->Get()->GetPath( eNormalizePathType );
	}
}

// basement storage  


