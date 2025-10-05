#include "stdafx.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "InterfaceMPCustomGame.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "InterfaceMisc.h"
#include "Misc/StrProc.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "GameRoomData.h"
#include "System/Text.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "UI/ListControlSorters.h"

#include <zconf.h>

const int nMAX_PLAYERS = 8;

// CInterfaceMPCustomGame

CInterfaceMPCustomGame::CInterfaceMPCustomGame() : 
CInterfaceMPScreenBase( "MPCustomGame", "custom_game" ), nSelectedID(-1)
{
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_UPDATE_GAME_LIST, SMPUIGameListMessage, &CInterfaceMPCustomGame::OnUpdateGameList );
}

CInterfaceMPCustomGame::~CInterfaceMPCustomGame()
{
}

bool CInterfaceMPCustomGame::Init()
{
	if ( CInterfaceScreenBase::	Init() == false ) 
		return false;

	AddScreen( this );
	pMain = GetChildChecked<IWindow>( pScreen, "Main", true );
	if ( !pMain )
		return false;
	SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );

	SetControls();
	SMPUIGameListMessage *pMsg = new SMPUIGameListMessage( true );
	Singleton<IMPToUIManager>()->AddUIMessage( pMsg );
	return true;
}

void CInterfaceMPCustomGame::RegisterObservers()
{
}

void CInterfaceMPCustomGame::SetFiltersData()
{
	CPtr<CTextData> pData;

	std::wstring wszAnyText = InterfaceState()->GetTextEntry("T_MP_GAME_ROOM_ANY");
	if ( pMapSizeBox )
	{		
		pData = new CTextData( wszAnyText );
		std::wstring wszSmallText = InterfaceState()->GetTextEntry("T_MP_GAME_ROOM_SMALL_MAP");
		pMapSizeBox->AddItem( pData );
		pData = new CTextData( wszSmallText );
		std::wstring wszMediumText = InterfaceState()->GetTextEntry("T_MP_GAME_ROOM_MEDIUM_MAP");
		pMapSizeBox->AddItem( pData );
		pData = new CTextData( wszMediumText );
		std::wstring wszLargeText = InterfaceState()->GetTextEntry("T_MP_GAME_ROOM_LARGE_MAP");
		pMapSizeBox->AddItem( pData );
		pData = new CTextData( wszLargeText );
		pMapSizeBox->AddItem( pData );
		pMapSizeBox->Select( 0 );
	}

	if ( pGameTypeBox )
	{
		pData = new CTextData( wszAnyText );
		pGameTypeBox->AddItem( pData );
		pGameTypeBox->Select( 0 );
	}
	if ( pTechLevel )
	{
		pData = new CTextData( wszAnyText );
		pTechLevel->AddItem( pData );
		for( int i = 0; i < NGameX::GetMPConsts()->techLevels.size(); ++i )
		{
			pData = new CTextData( 	GET_TEXT_PRE( NGameX::GetMPConsts()->techLevels[i]., Name ) );
			pTechLevel->AddItem( pData );
		}
		pTechLevel->Select( 0 );
	}
	if ( pNumPlayersBox )
	{
		pData = new CTextData( wszAnyText );
		pNumPlayersBox->AddItem( pData );
		for ( int i = 1; i <= 4; ++i )
		{
			std::wstring wszPlayers = NStr::ToUnicode( StrFmt( "%d", i * 2 ) );
			pData = new CTextData( wszPlayers );
			pNumPlayersBox->AddItem( pData );
		}	
		pNumPlayersBox->Select( 0 );		
	}
}

void CInterfaceMPCustomGame::SetControls()
{
	IButton *pBtn = pBtn = GetChildChecked<IButton>( pMain, "ButtonJoinGame", true );		
	pBtn->Enable( false );
	pFilters = GetChildChecked<IWindow> ( pMain, "Filters", true );
	if ( pFilters )
		pFilters->ShowWindow( false );
	pMapSizeBox = GetChildChecked<IComboBox> ( pFilters, "MapSize", true );
	if ( pMapSizeBox )
		pMapSizeBox->SetViewer( new CTextDataViewer() );
	pGameTypeBox = GetChildChecked<IComboBox> ( pFilters, "GameType", true );
	if ( pGameTypeBox )
		pGameTypeBox->SetViewer( new CTextDataViewer() );
	pTechLevel = GetChildChecked<IComboBox> ( pFilters, "TechLevel", true );
	if ( pTechLevel )
		pTechLevel->SetViewer( new CTextDataViewer() );
	pNumPlayersBox = GetChildChecked<IComboBox> ( pFilters, "NumPlayers", true );
	if ( pNumPlayersBox )
		pNumPlayersBox->SetViewer( new CTextDataViewer() );

	pList = GetChildChecked<IListControl> ( pMain, "CustomGame", false );
	if ( pList )
	{
		pList->SetViewer( new CItemCustomGameListViewer() );
	}
	SetFiltersData();

	pPasswordPopup = GetChildChecked<IWindow>( pMain, "PasswordPopup", true );
	pPasswordEdit = GetChildChecked<IEditLine>( pPasswordPopup, "PasswordEdit", true );
	pPasswordPopup->ShowWindow( false );

	pGettingListPopup = GetChildChecked<IWindow>( pMain, "GettingListPopup", true );
	pRefresh = GetChildChecked<IButton>( pMain, "ButtonRefresh", true );
	if ( pRefresh )
		pRefresh->Enable( false );
}

void CInterfaceMPCustomGame::CItemCustomGameListViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const
{
//	DebugTrace("Make interior for listviewer");
 	CDynamicCast<IListControlItem> pItem = pWindow;
	NI_VERIFY( pItem, "Wrong window", return );

	CDynamicCast<CCustomGameListData> pInfo = pData;
	SMPGameInfo &info = pInfo->info;
	
	NMPSetData::SetChildText( pItem, "ItemGameName", NStr::ToUnicode( info.szSessionName ) );			

	std::wstring wStr = NStr::ToUnicode( info.szMapName );
	NMPSetData::SetChildText( pItem, "ItemMapName", wStr );

	std::wstring wszTechText;
	if ( info.nTechLevel >= 0 && info.nTechLevel < NGameX::GetMPConsts()->techLevels.size() 
		&& CHECK_TEXT_NOT_EMPTY_PRE( NGameX::GetMPConsts()->techLevels[info.nTechLevel]., Name ) )
		wszTechText = GET_TEXT_PRE( NGameX::GetMPConsts()->techLevels[info.nTechLevel]., Name );
	NMPSetData::SetChildText( pItem, "ItemTechLevel", wszTechText );

	ITextView *pTxt = GetChildChecked<ITextView> ( pItem, "ItemGameType", true );		
	//NMPSetData::SetGameType( pTxt, info.pMPMap->pMap, 0/*info.nGameType*/ );

	NMPSetData::SetChildText( pItem, "ItemMapSize", NStr::ToUnicode( StrFmt( "%d x %d", info.nSizeX, info.nSizeY ) ) );		

	NMPSetData::SetChildText( pItem, "ItemNumPlayers", NStr::ToUnicode( StrFmt( "%d / %d", info.nPlayers, info.nPlayersMax ) ) );		
	//NMPSetData::SetChildText( pItem, "ItemPing", NStr::ToUnicode( StrFmt( "%d", info.nPing ) ) );	

	CPtr<IWindow> pPassword = GetChildChecked<IWindow>( pItem, "ItemPassword", true );
	pPassword->ShowWindow( info.bPwdReq );
}

void CInterfaceMPCustomGame::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
}

bool CInterfaceMPCustomGame::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "react_on_back" )
		return OnBackReaction( szSender );

	if ( szReaction == "react_on_join_game" )
		return OnJoinGameReaction( szSender );

	if ( szReaction == "react_on_filters" )
		return OnFiltersReaction( szSender );

	if ( szReaction == "react_on_load_game" )
		return OnLoadGameReaction( szSender );

	if ( szReaction == "react_on_create_game" )
		return OnCreateGameReaction( szSender );

	if ( szReaction == "react_on_refresh" )
		return OnRefreshReaction( szSender );

	if ( szReaction == "react_on_select_game" )
		return OnSelectGameReaction( szSender );

	if ( szReaction == "reaction_on_cancel" )
		return OnCancelFilters( szSender );
	if ( szReaction == "reaction_on_ok" )
		return OnOkFilters( szSender );

	if ( szReaction == "react_on_password_ok" )
		return OnPasswordOk( szSender );
	if ( szReaction == "react_on_password_cancel" )
		return OnPasswordCancel( szSender );

	return false;
}

bool CInterfaceMPCustomGame::OnCancelFilters( const std::string &szSender )
{
	pFilters->ShowWindow( false );
	return true;
}

bool CInterfaceMPCustomGame::OnOkFilters( const std::string &szSender )
{
	pFilters->ShowWindow( false );
	if ( pMapSizeBox )
		filter.nMapSize = pMapSizeBox->GetSelectedIndex();
	if ( pGameTypeBox )
		filter.nGameType = pGameTypeBox->GetSelectedIndex();
	if ( pTechLevel )
		filter.nTechLevel = pTechLevel->GetSelectedIndex();
	if ( pNumPlayersBox )
		filter.nPlayers = pNumPlayersBox->GetSelectedIndex();

	RebuildGameList();
	return true;
}

int CInterfaceMPCustomGame::Check( const std::string &szCheckName ) const
{
	return 0;
}

bool CInterfaceMPCustomGame::OnUpdateGameList( SMPUIGameListMessage *pMsg )
{	
	if ( pGettingListPopup )
		pGettingListPopup->ShowWindow( false );
	if ( pRefresh )
		pRefresh->Enable( true );

	for ( std::list<int>::iterator it = pMsg->gamesRemoved.begin(); it != pMsg->gamesRemoved.end(); ++it )
	{
		CMPGameListItems::iterator itGame = items.find( *it );
		if ( itGame == items.end() )
			continue;

		SGameEntry &game = itGame->second;
		if ( game.pItem )
			pList->RemoveItem( game.pItem );
		items.erase( itGame );
	}

	for ( std::list<SUIGameInfo>::iterator it = pMsg->gamesAddChange.begin(); it != pMsg->gamesAddChange.end(); ++it )
	{
		SUIGameInfo &game = *it;
		SGameEntry &entry = items[game.nGameID];
		entry.info.nServerID = game.nGameID;
		entry.info.bPwdReq = game.bPwdReq;
		entry.info.nPing = 0;
		entry.info.nPlayers = game.nPlayers;
		entry.info.nPlayersMax = game.nPlayersMax;
		entry.info.szSessionName = game.szSessionName;
		entry.info.szMapName = game.szMapName;
		entry.info.nSizeX = game.nSizeX;
		entry.info.nSizeY = game.nSizeY;
		entry.info.nTechLevel = game.nTechLevel;

		CPtr<CCustomGameListData> pData = new CCustomGameListData;
		pData->info = entry.info;

		bool bShouldShow = IsAllowedByFilter( entry.info );
		if ( entry.pItem && bShouldShow )
			entry.pItem->SetUserData( pData );
		else if ( !entry.pItem && bShouldShow )
			entry.pItem = pList->AddItem( pData );
		else if ( entry.pItem && !bShouldShow )
		{
			pList->RemoveItem( entry.pItem );
			entry.pItem = 0;
		}
	}	
	pList->Update();
	return true;
}

bool CInterfaceMPCustomGame::OnBackReaction( const std::string &szSender )
{
	// Stop updating games list
	SMPUIGameListMessage *pStopUpdateMsg = new SMPUIGameListMessage( false );
	Singleton<IMPToUIManager>()->AddUIMessage( pStopUpdateMsg );

	// Let the MPManager handle it
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_BACK_FROM_GAME_LIST );

	return true;
}

bool CInterfaceMPCustomGame::OnJoinGameReaction( const std::string &szSender )
{
	if ( nSelectedID != -1 )
	{
		CMPGameListItems::iterator it = items.find( nSelectedID );
		if ( it == items.end() )
			return true;

		CCustomGameListData *pData = dynamic_cast<CCustomGameListData*>( it->second.pItem->GetUserData() );
		SMPGameInfo &game = pData->info;
		if ( game.bPwdReq )
		{
			pPasswordPopup->ShowWindow( true );
			pPasswordEdit->SetText( L"" );
			return true;
		}
		wszPassword = L"";
		TryToJoinGame();
	}
	return true;
}

bool CInterfaceMPCustomGame::OnFiltersReaction( const std::string &szSender )
{	
	pFilters->ShowWindow( true );	

	if ( pMapSizeBox )
		 pMapSizeBox->Select( filter.nMapSize );
	if ( pGameTypeBox )
		 pGameTypeBox->Select( filter.nGameType );
	if ( pTechLevel )
		 pTechLevel->Select( filter.nTechLevel );
	if ( pNumPlayersBox )
		 pNumPlayersBox->Select( filter.nPlayers );

	return true;
}

bool CInterfaceMPCustomGame::OnRefreshReaction( const std::string &szSender )
{
	if ( pRefresh )
		pRefresh->Enable( false );
	return true;
}

bool CInterfaceMPCustomGame::OnCreateGameReaction( const std::string &szSender )
{
	// Stop updating games list
	SMPUIGameListMessage *pStopUpdateMsg = new SMPUIGameListMessage( false );
	Singleton<IMPToUIManager>()->AddUIMessage( pStopUpdateMsg );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MP_CREATE_CUSTOM_GAME_MENU, "" );

	return true;
}

bool CInterfaceMPCustomGame::OnLoadGameReaction( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_SAVE_LOAD_MENU, "load_from_main_menu" );
	
	return true;
}

bool CInterfaceMPCustomGame::OnSelectGameReaction( const std::string &szSender )
{	
	IButton *pBtn = GetChildChecked<IButton>( pMain, "ButtonJoinGame", true );
	IListControlItem *pItem = pList->GetSelectedListItem();
	CDynamicCast<CCustomGameListData> pInfo = pItem->GetUserData();
	if ( pInfo && pBtn )
	{
		nSelectedID = pInfo->info.nServerID;	
		pBtn->Enable( true );
	}
	//DebugTrace( "+++Selected Server %d", nSelectedID );
	return true;
}

bool CInterfaceMPCustomGame::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

void CInterfaceMPCustomGame::AfterLoad()
{
	CInterfaceScreenBase::AfterLoad();

	RegisterObservers();
}

bool CInterfaceMPCustomGame::OnPasswordOk( const std::string &szSender )
{
	wszPassword = pPasswordEdit->GetText();
	pPasswordPopup->ShowWindow( false );
	TryToJoinGame();
	return true;
}

bool CInterfaceMPCustomGame::OnPasswordCancel( const std::string &szSender )\
{
	pPasswordPopup->ShowWindow( false );
	return true;
}

void CInterfaceMPCustomGame::TryToJoinGame()
{
	// Send join message
	SMPUIJoinGameMessage *pMsg = new SMPUIJoinGameMessage( nSelectedID, NStr::ToMBCS( wszPassword ) );
	Singleton<IMPToUIManager>()->AddUIMessage( pMsg );				

	// Stop updating games list
	SMPUIGameListMessage *pStopUpdateMsg = new SMPUIGameListMessage( false );
	Singleton<IMPToUIManager>()->AddUIMessage( pStopUpdateMsg );

	// Go to Game Room
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MP_GAME_ROOM, "" );
}

void CInterfaceMPCustomGame::RebuildGameList()
{
	pList->RemoveAllElements();
	for ( CMPGameListItems::iterator it = items.begin(); it != items.end(); ++it )
	{
		SGameEntry &entry = it->second;
		entry.pItem = 0;
		CPtr<CCustomGameListData> pData = new CCustomGameListData;
		pData->info = entry.info;

		if ( IsAllowedByFilter( entry.info ) )
			entry.pItem = pList->AddItem( pData );
	}	
	pList->Update();
}

bool CInterfaceMPCustomGame::IsAllowedByFilter( const SMPGameInfo &game )
{
	if ( filter.nGameType )
	{
	}

	if ( filter.nTechLevel )
	{
		if ( game.nTechLevel != filter.nTechLevel - 1 )
			return false;
	}

	if ( filter.nPlayers )
	{
		if ( game.nPlayersMax != filter.nPlayers * 2 )
			return false;
	}

	if ( filter.nMapSize )
	{
		const int nSMBound = 9 * 9;
		const int nMLBound = 15 * 15;
		switch ( filter.nMapSize )
		{
			case 1:			// SMALL
				if ( game.nSizeX * game.nSizeY > nSMBound )
					return false;
				break;
			case 2:			// MEDIUM
				if ( game.nSizeX * game.nSizeY < nSMBound || game.nSizeX * game.nSizeY > nMLBound )
					return false;
				break;
			case 3:			// LARGE
				if ( game.nSizeX * game.nSizeY < nMLBound )
					return false;
				break;
		}
	}

	return true;
}

// CICMPCustomGame

void CICMPCustomGame::PreCreate()
{
}

void CICMPCustomGame::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMPCustomGame::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x32194C00, CInterfaceMPCustomGame );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MP_CUSTOM_GAME_MENU, CICMPCustomGame );


