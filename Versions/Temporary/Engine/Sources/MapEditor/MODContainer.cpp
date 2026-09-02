#include "stdafx.h"
#include <fmt/printf.h>

#include "MODContainer.h"
#include "CreateMODDialog.h"
#include "OpenMODDialog.h"
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
	CCreateMODDialog createMODDialog;
	if	( createMODDialog.DoModal() == IDOK )
	{
		std::string szMODFolder = createMODDialog.GetFolder();
		CString strName = createMODDialog.GetName();
		CString strDescriotion = createMODDialog.GetDescription();
		if ( !szMODFolder.empty() && !strName.IsEmpty() )
		{
			NProgress::Create( true );
			CString strPM;
			strPM.LoadString( IDS_PM_CREATE_MOD );
			NProgress::SetMessage( fmt::sprintf( strPM.GetString(), szMODFolder.c_str() ) );
			NProgress::SetRange( 0, 2 );
			//
			// Создать файлы с именем и описанием
			String2File( strName, true, szMODFolder + "name.txt", ::GetACP(), true );
			String2File( strDescriotion, true, szMODFolder + "desc.txt", ::GetACP(), true );
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
	}
	return false;
}


bool CMODContainer::OpenMOD()
{
	if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
	{
		return false;
	}
	COpenMODDialog openMODDialog;
	if	( openMODDialog.DoModal() == IDOK )
	{
		NMOD::SMOD mod;
		if ( openMODDialog.GetMOD( &mod ) )
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


