#include "stdafx.h"
#include "InterfaceSaveLoadMenu.h"
#include "InterfaceMisc.h"
#include "InterfaceState.h"
#include "GameXClassIDs.h"
#include "UI/Window.h"
#include "Misc/StrProc.h"
#include "System/FileUtils.h"
#include "ScenarioTracker.h"
#include "System/Text.h"
#include "3Dmotor/ScreenShot.h"

#include "GameX_export.h"

#include <fmt/format.h>

// CInterfaceSaveLoadMenu::CReactions

bool CInterfaceSaveLoadMenu::CReactions::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "react_on_selection_change" )
	{
		pInterface->OnSelectionChange( szSender );
		return true;
	}
	return false;
}

int CInterfaceSaveLoadMenu::CReactions::Check( const std::string &szCheckName ) const
{
	return 0;
}

// CInterfaceSaveLoadMenu::SSortByName

bool CInterfaceSaveLoadMenu::SSortByName::operator()( const NSaveLoad::SSavegameEntry &save1, const NSaveLoad::SSavegameEntry &save2 )
{
	if ( save1.info.bQuickSave != save2.info.bQuickSave )
		return save1.info.bQuickSave;
	if ( save1.info.bAutoSave != save2.info.bAutoSave )
		return save1.info.bAutoSave;

	std::wstring wszName1 = save1.wszName;
	std::wstring wszName2 = save2.wszName;
	transform( wszName1.begin(), wszName1.end(), wszName1.begin(), towlower ); 
	transform( wszName2.begin(), wszName2.end(), wszName2.begin(), towlower ); 

	return wszName1 < wszName2;
}

// CInterfaceSaveLoadMenu::SSortByDate

bool CInterfaceSaveLoadMenu::SSortByDate::operator()( const NSaveLoad::SSavegameEntry &save1, const NSaveLoad::SSavegameEntry &save2 )
{
	FILETIME fileTime1;
	FILETIME fileTime2;
	SystemTimeToFileTime( &save1.time, &fileTime1 );
	SystemTimeToFileTime( &save2.time, &fileTime2 );
	ULARGE_INTEGER val = *(ULARGE_INTEGER*)(&fileTime1);
	ULARGE_INTEGER val2 = *(ULARGE_INTEGER*)(&fileTime2);
	return val.QuadPart > val2.QuadPart;
}

// CInterfaceSaveLoadMenu

CInterfaceSaveLoadMenu::CInterfaceSaveLoadMenu() : 
	CInterfaceScreenBase( "SaveLoadMenu", "save_load_menu" )
{
	pScreenShotTexture = new CBackgroundMutableTexture;

	AddObserver( "menu_back", &CInterfaceSaveLoadMenu::MsgBack );
	AddObserver( "menu_delete", &CInterfaceSaveLoadMenu::MsgDelete );
	AddObserver( "menu_load", &CInterfaceSaveLoadMenu::MsgLoad );
	AddObserver( "menu_save", &CInterfaceSaveLoadMenu::MsgSave );
	AddObserver( "message_box_ok", &CInterfaceSaveLoadMenu::MsgOk );
	AddObserver( "message_box_cancel", &CInterfaceSaveLoadMenu::MsgCancel );
	AddObserver( "save_list_dbl_click", &CInterfaceSaveLoadMenu::MsgDblClick );
	AddObserver( "select_prev", &CInterfaceSaveLoadMenu::MsgSelectPrev );
	AddObserver( "select_next", &CInterfaceSaveLoadMenu::MsgSelectNext );
}

CInterfaceSaveLoadMenu::~CInterfaceSaveLoadMenu()
{
	pReactions = 0;
}

bool CInterfaceSaveLoadMenu::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;
		
	pReactions = new CReactions( pScreen, this );
	AddScreen( pReactions );

	// Get elements
	pMain = GetChildChecked<IWindow>( pScreen, "SaveLoadMenu", true );

	bConfirmLoadQuestion = true;

	return true;
}

void CInterfaceSaveLoadMenu::MakeInterior()
{
	pLoad = GetChildChecked<IWindow>( pMain, "LoadButton", true );
	pSave = GetChildChecked<IWindow>( pMain, "SaveButton", true );
	pDeleteBtn = GetChildChecked<IButton>( pMain, "DeleteBtn", true );
	pPicture = GetChildChecked<IWindow>( pMain, "Picture", true );
	pEditLine = GetChildChecked<IEditLine>( pMain, "TextEdit", true );
	pHeaderSaveView = GetChildChecked<ITextView>( pMain, "HeaderSaveView", true );
	pHeaderLoadView = GetChildChecked<ITextView>( pMain, "HeaderLoadView", true );
	IWindow *pLoadListPanel = GetChildChecked<IWindow>( pMain, "LoadListPanel", true );
	IWindow *pSaveListPanel = GetChildChecked<IWindow>( pMain, "SaveListPanel", true );
	if ( bLoadMode )
		pListPanel = GetChildChecked<IWindow>( pLoadListPanel, "ListPanel", true );
	else
		pListPanel = GetChildChecked<IWindow>( pSaveListPanel, "ListPanel", true );
	pListTemplate = GetChildChecked<IScrollableContainer>( pListPanel, "SaveList", true );
	pListItemTemplate = GetChildChecked<IWindow>( pListPanel, "ListItem", true );
	if ( pLoadListPanel )
		pLoadListPanel->ShowWindow( bLoadMode );
	if ( pSaveListPanel )
		pSaveListPanel->ShowWindow( !bLoadMode );
	if ( pEditLine )
	{
		pEditLine->ShowWindow( !bLoadMode );
		if ( !bLoadMode )
			pEditLine->SetFocus( true );
	}
	if ( pListItemTemplate )
		pListItemTemplate->ShowWindow( false );

	pDescPanel = GetChildChecked<IWindow>( pMain, "DescMissionPanel", true );
	pDescCampaignView = GetChildChecked<ITextView>( pDescPanel, "CampaignView", true );
	pDescChapterView = GetChildChecked<ITextView>( pDescPanel, "ChapterView", true );
	pDescMissionView = GetChildChecked<ITextView>( pDescPanel, "MissionView", true );
	pDescMissionBriefingCont = GetChildChecked<IScrollableContainer>( pDescPanel, "DescMissionBriefingCont", true );
	pDescMissionBriefingView = GetChildChecked<ITextView>( pDescMissionBriefingCont, "ItemView", true );
	if ( pDescMissionBriefingCont && pDescMissionBriefingView )
		pDescMissionBriefingCont->PushBack( pDescMissionBriefingView, false );
	if ( pDescPanel )
		pDescPanel->ShowWindow( false );

	pDescCustomPanel = GetChildChecked<IWindow>( pMain, "DescCustomPanel", true );
	pDescCustomMissionView = GetChildChecked<ITextView>( pDescCustomPanel, "MissionView", true );
	pDescCustomMissionBriefingCont = GetChildChecked<IScrollableContainer>( pDescCustomPanel, "DescMissionBriefingCont", true );
	pDescCustomMissionBriefingView = GetChildChecked<ITextView>( pDescCustomMissionBriefingCont, "ItemView", true );
	if ( pDescCustomMissionBriefingCont && pDescCustomMissionBriefingView )
		pDescCustomMissionBriefingCont->PushBack( pDescCustomMissionBriefingView, false );
	if ( pDescCustomPanel )
		pDescCustomPanel->ShowWindow( false );

	pDescChapterPanel = GetChildChecked<IWindow>( pMain, "DescChapterPanel", true );
	pDescChapterCampaignView = GetChildChecked<ITextView>( pDescChapterPanel, "CampaignView", true );
	pDescChapterChapterView = GetChildChecked<ITextView>( pDescChapterPanel, "ChapterView", true );
	pDescChapterBriefingCont = GetChildChecked<IScrollableContainer>( pDescChapterPanel, "DescChapterBriefingCont", true );
	pDescChapterBriefingView = GetChildChecked<ITextView>( pDescChapterBriefingCont, "ItemView", true );
	pChapterTexture = GetChildChecked<IWindow>( pMain, "ChapterPicture", true );
	pChapterTextureBorder = GetChildChecked<IWindow>( pMain, "ChapterPictureBorder", true );
	if ( pDescChapterBriefingCont && pDescChapterBriefingView )
		pDescChapterBriefingCont->PushBack( pDescChapterBriefingView, false );
	if ( pDescChapterPanel )
		pDescChapterPanel->ShowWindow( false );
	if ( pChapterTextureBorder )
		pChapterTextureBorder->ShowWindow( false );

	if ( pLoad )
		pLoad->ShowWindow( false );
	if ( pSave )
		pSave->ShowWindow( false );
	if ( pListTemplate )
		pListTemplate->ShowWindow( false );
	if ( pHeaderSaveView )
		pHeaderSaveView->ShowWindow( false );
	if ( pHeaderLoadView )
		pHeaderLoadView->ShowWindow( false );

	pSaveList = 0;
	nSelected = -1;
	nLastID = 0;

	NSaveLoad::GetSaveList( &saves, &nLastID );
	//Fill list for the first time
	FillSaveList( SORT_BY_NAME );

	if ( CWindow *pPic = dynamic_cast_ptr<CWindow *>(pPicture) ) // CRAP - don't use CWindow type out of UI implementation
		pPic->SetBackground( pScreenShotTexture );
	
	UpdateButtons();

	if ( pMain )
		SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );
}

void CInterfaceSaveLoadMenu::FillSaveList( const ESortModes nSortMode )
{
	if ( pSaveList )
		pSaveList->RemoveItems();
	if ( pListPanel && pSaveList )
	{
		pListPanel->RemoveChild( pSaveList );
		pSaveList = 0;
	}

	pSaveList = dynamic_cast<IScrollableContainer*>( AddWindowCopy( pListPanel, pListTemplate ) );
	if ( pSaveList )
		pSaveList->ShowWindow( true );
	NI_ASSERT( pSaveList, "Unable to create list" );

	sort( saves.begin(), saves.end(), SSortByDate() );
	for ( int i = 0; i < saves.size(); ++i )
	{
		NSaveLoad::SSavegameEntry &save = saves[i];
		save.pWindow = AddWindowCopy( pSaveList, pListItemTemplate );
		if ( save.pWindow )
		{
			save.pWindow->ShowWindow( true );
			save.pWindow->SetName( fmt::format( "Item{}", i ) );
		}

		IWindow *pFlag = GetChildChecked<IWindow>( save.pWindow, "SubItemFlag", true );		//Get subitems
		ITextView *pName = GetChildChecked<ITextView>( save.pWindow, "SubItemName", true );
		ITextView *pDate = GetChildChecked<ITextView>( save.pWindow, "SubItemDate", true );

		//Fill the line
		if ( pFlag )
		{
			pFlag->SetTexture( save.info.pFlagTexture );
			pFlag->ShowWindow( save.info.pFlagTexture != 0 );
		}
		
		if ( pName )
			pName->SetText( pName->GetDBText() + save.wszName );
		
		std::wstring wszDate = NStr::ToUnicode( fmt::format("{:02d}.{:02d}.{:04d} {:02d}:{:02d}",
			save.time.wDay, save.time.wMonth, save.time.wYear,
			save.time.wHour, save.time.wMinute ) );
		if ( pDate )
			pDate->SetText( pDate->GetDBText() + wszDate );

		if ( pSaveList )
			pSaveList->PushBack( save.pWindow, true );
	}
}

void CInterfaceSaveLoadMenu::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
}

void CInterfaceSaveLoadMenu::MsgBack( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	if ( ePrevInterface == EPI_MAIN_MENU )
		NMainLoop::Command( ML_COMMAND_MAIN_MENU, "single_player_submenu" );
	else if ( ePrevInterface == EPI_LOOSE_MENU )
		NMainLoop::Command( ML_COMMAND_WIN_LOOSE, "-1" );

	if ( ePrevInterface == EPI_CHAPTER_MAP )
		NInput::PostEvent( "menu_return_from_subscreen", 0, 0 );
}

bool CInterfaceSaveLoadMenu::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive );
}

void CInterfaceSaveLoadMenu::MsgDelete( const SGameMessage &msg )
{
	if ( nSelected == -1 ) 
	{
		return;
	}
	eConfirmAction = CONFIRM_DELETE;
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
		InterfaceState()->GetTextEntry( "T_SAVELOAD_MENU_DELETE_QUESTION" ) ).c_str() );
}

void CInterfaceSaveLoadMenu::MsgOk( const SGameMessage &msg )
{
	//User agreed to do something (delete or overwrite savegame)
	switch ( eConfirmAction ) 
	{
		case CONFIRM_SAVE:
			DoSaveGame();
			break;
			
		case CONFIRM_DELETE:
			{
				//Delete file
				BOOL bResult = DeleteFile( saves[nSelected].szFileName.c_str() );
				NI_ASSERT( bResult, "File removal failed");

				//Remove extra-info file
				DeleteFile( saves[nSelected].szInfoFileName.c_str() );		//Result is irrelevant

				//Delete screen entry
				NSaveLoad::CSaveList::iterator it = std::begin(saves) + nSelected;
				saves.erase( it );
				//nSelected = -1;
				//Rebuild list
				FillSaveList( SORT_BY_NAME );
				if ( nSelected >= saves.size() )
					nSelected = saves.size() - 1;
				SelectItem( true, true );
				
				UpdateButtons();
			}
			break;
			
		case CONFIRM_LOAD:
		{
			IWindow *pWait = GetChildChecked<IWindow>( pMain, "LoadingPopup", true );
			if ( pWait )
			{
				pWait->ShowWindow( true );
				Draw();
			}
			saves[nSelected].info.SetLoadInfo();
			NMainLoop::Command( ML_COMMAND_LOAD_GAME, saves[nSelected].szFileTitle.c_str() );
		}
		break;
	
		case NO_SAVE_NAME:
			break;
	}
}

void CInterfaceSaveLoadMenu::MsgCancel( const SGameMessage &msg )
{
	//User refused to delete game, do nothing
}

void CInterfaceSaveLoadMenu::MsgDblClick( const SGameMessage &msg )
{
	if ( bLoadMode )
		Load();
}

void CInterfaceSaveLoadMenu::MsgSelectPrev( const SGameMessage &msg )
{
	if ( nSelected > 0 )
	{
		--nSelected;
		SelectItem( true, true );
		UpdateButtons();
	}
}

void CInterfaceSaveLoadMenu::MsgSelectNext( const SGameMessage &msg )
{
	if ( nSelected >= 0 && nSelected + 1 < saves.size() )
	{
		++nSelected;
		SelectItem( true, true );
		UpdateButtons();
	}
}

void CInterfaceSaveLoadMenu::Load()
{
	if ( nSelected >= 0 && nSelected < saves.size() )
	{
		if ( bConfirmLoadQuestion )
		{
			eConfirmAction = CONFIRM_LOAD;
			NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
				CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
				InterfaceState()->GetTextEntry( "T_SAVELOAD_MENU_CONFIRM_LOAD" ) ).c_str() );
		}
		else
		{
			saves[nSelected].info.SetLoadInfo();
			NMainLoop::Command( ML_COMMAND_LOAD_GAME, saves[nSelected].szFileTitle.c_str() );
		}
	}
}

void CInterfaceSaveLoadMenu::MsgLoad( const SGameMessage &msg )
{
	Load();
}

void CInterfaceSaveLoadMenu::MsgSave( const SGameMessage &msg )
{
	//Get EditLine contents
	std::wstring wszSaveName = pEditLine ? pEditLine->GetText() : L"";

	if ( wszSaveName.empty() )
	{
		eConfirmAction = NO_SAVE_NAME;
		NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
			CICMessageBox::MakeConfigString( "MessageBoxWindowOk", 
			InterfaceState()->GetTextEntry( "T_SAVELOAD_MENU_NO_NAME" ) ).c_str() );
			
		return;
	}
	//Check if the game name already exists
	for ( int i = 0; i < saves.size(); ++i )
	{
		if ( saves[i].wszName == wszSaveName )			//Already exists, ask
		{
			eConfirmAction = CONFIRM_SAVE;
			NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
				CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
				InterfaceState()->GetTextEntry( "T_SAVELOAD_MENU_OVERWRITE_QUESTION" ) ).c_str() );

			return;
		}
	}

	nSelected = -1;			// indicate that it is a new save
	DoSaveGame();				// not found, fresh save
}

void CInterfaceSaveLoadMenu::DoSaveGame()
{
	NSaveLoad::SSavegameEntry sg;
	
	//Get EditLine contents
	std::wstring wszSaveName = pEditLine ? pEditLine->GetText() : L"";
	nSelected = -1;
	for ( int i = 0; i < saves.size(); ++i )
	{
		NSaveLoad::SSavegameEntry &save = saves[i];
		
		if ( wszSaveName == save.wszName )
		{
			nSelected = i;
			break;
		}
	}

	sg.wszName = wszSaveName;

	GetLocalTime( &sg.time );

	if ( nSelected >= 0 && nSelected < saves.size() ) 				//if replacing a save
	{																													//delete old files and entry
		sg.szFileTitle = saves[nSelected].szFileTitle;

		DeleteFile( saves[nSelected].szFileName.c_str() );
		DeleteFile( saves[nSelected].szInfoFileName.c_str() );		//Result is irrelevant

		//Delete screen entry
		NSaveLoad::CSaveList::iterator it = std::begin(saves) + nSelected;
		saves.erase( it );
	}
	else
		sg.szFileTitle = NSaveLoad::GetUniqueFileName( saves );
	NI_VERIFY( NFile::IsValidDirName( sg.szFileTitle ), "Wrong file name", return );
	sg.szFileName = NSaveLoad::GetSavePath() + sg.szFileTitle + NSaveLoad::SAVE_FILE_EXTENSION;
	sg.szInfoFileName = NSaveLoad::GetSavePath() + sg.szFileTitle + NSaveLoad::INFO_FILE_EXTENSION;

	if ( ePrevInterface != EPI_CHAPTER_MAP )
	{
		pMain->ShowWindow( false );
		IInterfaceBase *pPrevInterface = NMainLoop::GetPrevInterface( this );
		if ( pPrevInterface )
		{
			CInterfaceScreenBase *pPrevScreen = dynamic_cast<CInterfaceScreenBase*>( pPrevInterface );
			if ( pPrevScreen )
				pPrevScreen->Draw( 0 );
		}
		InterfaceState()->GetScreenShotTexture()->Generate( true );
	}
	sg.info.Write( sg.szInfoFileName, sg.wszName, false, false, ePrevInterface == EPI_CHAPTER_MAP );

	saves.push_back( sg );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_SAVE_GAME, sg.szFileTitle.c_str() );

	if ( ePrevInterface == EPI_CHAPTER_MAP )
		NInput::PostEvent( "menu_return_from_subscreen", 0, 0 );
}

void CInterfaceSaveLoadMenu::SetMode( const std::string &szMode )
{
	ePrevInterface = EPI_ANY;
	bLoadMode = true;
	if ( szMode == "save" )
		bLoadMode = false;
	else if ( szMode == "load" )
	{
		bLoadMode = true;
		bConfirmLoadQuestion = true;
	}
	else if ( szMode == "load_from_main_menu" )
	{
		bLoadMode = true;
		bConfirmLoadQuestion = false;
		ePrevInterface = EPI_MAIN_MENU;
	}
	else if ( szMode == "load_from_loose_menu" )
	{
		bLoadMode = true;
		bConfirmLoadQuestion = false;
		ePrevInterface = EPI_LOOSE_MENU;
	}
	else if ( szMode == "save_from_main_menu" )
	{
		bLoadMode = false;
		ePrevInterface = EPI_CHAPTER_MAP;
	}
	else if ( szMode == "load_from_single_statistics" )
	{
		bLoadMode = true;
	}
	else
		NI_ASSERT( 0, "Unknown save/load mode" );

	MakeInterior();

	if ( pLoad )
		pLoad->ShowWindow( bLoadMode );
	if ( pSave )
		pSave->ShowWindow( !bLoadMode );
	if ( pEditLine )
		pEditLine->ShowWindow( !bLoadMode );
	if ( pHeaderSaveView )
		pHeaderSaveView->ShowWindow( !bLoadMode );
	if ( pHeaderLoadView )
		pHeaderLoadView->ShowWindow( bLoadMode );

	if ( bLoadMode && !saves.empty() )
		nSelected = 0;

	SelectItem( true, bLoadMode );

	if ( !bLoadMode )
	{
		// fill initial save info
		startSaveInfo.bQuickSave = false;
		startSaveInfo.bAutoSave = false;
		if ( ePrevInterface != EPI_CHAPTER_MAP && Singleton<IScenarioTracker>()->IsMissionActive() )
		{
			if ( const NDb::SMapInfo *pMapInfoDB = Singleton<IScenarioTracker>()->GetCurrentMission() )
			{
				if ( CHECK_TEXT_NOT_EMPTY_PRE(pMapInfoDB->,LocalizedName) )
				{
					startSaveInfo.wszSaveName = GET_TEXT_PRE(pMapInfoDB->,LocalizedName);
				}
			}
		}
		else if ( Singleton<IScenarioTracker>()->IsChapterActive() )
		{
			if ( const NDb::SChapter *pChapterDB = Singleton<IScenarioTracker>()->GetCurrentChapter() )
			{
				if ( CHECK_TEXT_NOT_EMPTY_PRE(pChapterDB->,LocalizedName) )
				{
					startSaveInfo.wszSaveName = GET_TEXT_PRE(pChapterDB->,LocalizedName);
				}
			}
		}
		startSaveInfo.bFromChapter = false;
		startSaveInfo.GenerateInfo();

		// show default name
	 	if ( pEditLine )
	 	{
			pEditLine->SetText( startSaveInfo.wszSaveName.c_str() );
			pEditLine->SetSelection( 0, startSaveInfo.wszSaveName.size() );
		}

		// show initial save info
		ShowSaveInfo( startSaveInfo );
	}
}

void CInterfaceSaveLoadMenu::OnSelectionChange( const std::string &szSender )
{
	nSelected = -1;
	IWindow *pSelection = pSaveList ? pSaveList->GetSelectedItem() : 0;
	if ( !pSelection ) 
		return;

	for ( int i = 0; i < saves.size(); ++i )
	{
		if ( pSelection == saves[i].pWindow ) 
		{
			nSelected = i;
			SelectItem( false, true );
			UpdateButtons();
		}
	}
}

void CInterfaceSaveLoadMenu::ComposeDescription( const NSaveLoad::SSaveInfo &info )
{
	std::wstring wszCampaign;
	std::wstring wszChapter;
	std::wstring wszMission;
	std::wstring wszBriefing;
	std::wstring wszChapterBriefing;

  //Campaign
	wszCampaign = info.GetCampaignName();
	
	//Chapter
	wszChapter = info.GetChapterName();
	wszChapterBriefing = info.GetChapterDesc();
	
	//Mission
	bool bIsMission = false;
	wszMission = info.GetMissionName();
	wszBriefing = info.GetMissionDesc();

	if ( pDescPanel )
		pDescPanel->ShowWindow( !info.bFromChapter && !info.bCustomMission );
	if ( pDescCustomPanel )
		pDescCustomPanel->ShowWindow( !info.bFromChapter && info.bCustomMission );
	if ( pDescChapterPanel )
		pDescChapterPanel->ShowWindow( info.bFromChapter );
		
	if ( pDescCampaignView )
		pDescCampaignView->SetText( pDescCampaignView->GetDBText() + wszCampaign );
	if ( pDescChapterView )
		pDescChapterView->SetText( pDescChapterView->GetDBText() + wszChapter );
	if ( pDescMissionView )
		pDescMissionView->SetText( pDescMissionView->GetDBText() + wszMission );
	if ( pDescMissionBriefingView )
		pDescMissionBriefingView->SetText( pDescMissionBriefingView->GetDBText() + wszBriefing );

	if ( pDescCustomMissionView )
		pDescCustomMissionView->SetText( pDescCustomMissionView->GetDBText() + wszMission );
	if ( pDescCustomMissionBriefingView )
		pDescCustomMissionBriefingView->SetText( pDescCustomMissionBriefingView->GetDBText() + wszBriefing );

	if ( pDescChapterCampaignView )
		pDescChapterCampaignView->SetText( pDescChapterCampaignView->GetDBText() + wszCampaign );
	if ( pDescChapterChapterView )
		pDescChapterChapterView->SetText( pDescChapterChapterView->GetDBText() + wszChapter );
	if ( pDescChapterBriefingView )
		pDescChapterBriefingView->SetText( pDescChapterBriefingView->GetDBText() + wszChapterBriefing );

	if ( pDescMissionBriefingCont )
		pDescMissionBriefingCont->Update();
	if ( pDescCustomMissionBriefingCont )
		pDescCustomMissionBriefingCont->Update();
	if ( pDescChapterBriefingCont )
		pDescChapterBriefingCont->Update();

	if ( pPicture )
		pPicture->ShowWindow( true /*!info.bFromChapter*/ );
	if ( pChapterTextureBorder )
		pChapterTextureBorder->ShowWindow( false /*info.bFromChapter*/ );
	if ( pChapterTexture )
	{
		if ( info.bFromChapter )
			pChapterTexture->SetTexture( info.pPicture );
		pChapterTexture->ShowWindow( false );
	}
}

void CInterfaceSaveLoadMenu::SelectItem( bool bUpdateSelection, bool bClearEditLine ) 
{
	if ( pDescPanel )
		pDescPanel->ShowWindow( false );
	if ( pDescCustomPanel )
		pDescCustomPanel->ShowWindow( false );
	if ( pDescChapterPanel )
		pDescChapterPanel->ShowWindow( false );

	if ( bClearEditLine && pEditLine )
		pEditLine->SetText( L"" );
	if ( pPicture )
		pPicture->SetTexture( 0 );
	if ( pChapterTexture )
		pChapterTexture->SetTexture( 0 );
	if ( pPicture )
		pPicture->ShowWindow( false );
	if ( pChapterTextureBorder )
		pChapterTextureBorder->ShowWindow( false );

	if ( pDescCampaignView )
		pDescCampaignView->SetText( L"" );
	if ( pDescChapterView )
		pDescChapterView->SetText( L"" );
	if ( pDescMissionView )
		pDescMissionView->SetText( L"" );

	if ( pDescMissionBriefingView )
		pDescMissionBriefingView->SetText( L"" );
	if ( pDescMissionBriefingCont )
		pDescMissionBriefingCont->Update();

	if ( pDescCustomMissionBriefingView )
		pDescCustomMissionBriefingView->SetText( L"" );
	if ( pDescCustomMissionBriefingCont )
		pDescCustomMissionBriefingCont->Update();
	if ( pChapterTextureBorder )
		pChapterTextureBorder->ShowWindow( false );

	if ( pDescMissionBriefingView )
		pDescMissionBriefingView->SetText( L"" );
	if ( pDescMissionBriefingCont )
		pDescMissionBriefingCont->Update();

	if ( 0 <= nSelected && nSelected < saves.size() )
	{
		NSaveLoad::SSavegameEntry &save = saves[nSelected];
		IWindow *pSelection = save.pWindow;
		
		if ( pSaveList )
		{
			if ( bUpdateSelection )
			{
				pSaveList->Select( pSelection );
				pSaveList->EnsureElementVisible( pSelection );
			}
		}

		//Fill associated controls
		if ( pEditLine )
			pEditLine->SetText( save.wszName.c_str() );		//EditLine

		ShowSaveInfo( save.info );
	}
	else
	{
		if ( pSaveList )
			pSaveList->Select( 0 );
	}
}

void CInterfaceSaveLoadMenu::ShowSaveInfo( NSaveLoad::SSaveInfo &info )
{
	//Screenshot
	CWindow *pPic = dynamic_cast_ptr<CWindow *>(pPicture);

	CArray2D<NGfx::SPixel8888> &screenShot = info.screenShot;
	if ( screenShot.IsEmpty() || ( screenShot.GetSizeX() * screenShot.GetSizeY() == 0 ) )
	{
		screenShot.SetSizes( 1, 1 );
		screenShot.FillZero();
	}
	if ( pScreenShotTexture )
		pScreenShotTexture->Set( screenShot );

	if ( pPic )
		pPic->SetBackground( pScreenShotTexture );

	//Description
	ComposeDescription( info );
}

void CInterfaceSaveLoadMenu::UpdateButtons()
{
	if ( pLoad )
		pLoad->Enable( nSelected != -1 );
	if ( pDeleteBtn )
		pDeleteBtn->Enable( nSelected != -1 );
}

bool CInterfaceSaveLoadMenu::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

// CICInterfaceSaveLoad

void CICInterfaceSaveLoad::PreCreate()
{
}

void CICInterfaceSaveLoad::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
	pInterface->SetMode( szMode );
}

void CICInterfaceSaveLoad::Configure( const char *pszConfig )
{
	szMode = pszConfig;
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x170CC400, CInterfaceSaveLoadMenu );
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_SAVE_LOAD_MENU, CICInterfaceSaveLoad );
REGISTER_SAVELOAD_CLASS_NM( GAMEX, 0x170CC401, CReactions, CInterfaceSaveLoadMenu );


