#include "stdafx.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "InterfaceMPCreateCustomGame.h"
#include "GameXClassIDs.h"
#include "UI/SceneClassIDs.h"
#include "SceneB2/Scene.h"
#include "InterfaceState.h"
#include "Misc/StrProc.h"

#include "DBGameRoot.h"
#include "UISpecificB2/DBUIspecificB2.h"
#include "GameRoomData.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "System/Text.h"
#include "CustomMissions.h"

#include <zconf.h>

// CInterfaceMPCreateCustomGame

CInterfaceMPCreateCustomGame::CInterfaceMPCreateCustomGame() : 
CInterfaceMPScreenBase( "MPCreateCustomGame", "create_custom_game" )
{
}

CInterfaceMPCreateCustomGame::~CInterfaceMPCreateCustomGame()
{
	if ( pScreen ) 
		Scene()->RemoveScreen( pScreen );
}

bool CInterfaceMPCreateCustomGame::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	RegisterObservers();

	// load screen
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	if ( AddUIScreen( pScreen, "MPCreateCustomGame", this ) == false )
		return false;

	pMain = GetChildChecked<IWindow>( pScreen, "Main", true );
	NI_VERIFY( pMain, "No main window found", return false );	
	SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );

	pSelected = 0;
	InitControls();
	return true;
}

void CInterfaceMPCreateCustomGame::RegisterObservers()
{	
}

void CInterfaceMPCreateCustomGame::FillMapData()
{
	std::vector<CDBID> mpMapDBIDs;
	NCustom::GetMultiplayerMaps( &mpMapDBIDs );

	for ( int i = 0; i < mpMapDBIDs.size(); ++i )
	{
		SMPMapInfo tmpInfo;
		const NDb::SMultiplayerMap *pMPMap = NDb::Get<NDb::SMultiplayerMap>( mpMapDBIDs[i] );
		if ( !pMPMap )
			continue;

		tmpInfo.wszMapName = GET_TEXT_PRE( pMPMap->, MapName );
		tmpInfo.wszGameType = L""; 
		tmpInfo.pMPMap = pMPMap;

		tmpInfo.nPlayersMax = pMPMap->nPlayers;
		tmpInfo.nMapSizeX = pMPMap->nSizeX;
		tmpInfo.nMapSizeY = pMPMap->nSizeY;

		maps.push_back( tmpInfo );
	}
	UpdateInterior();
}

void CInterfaceMPCreateCustomGame::InitControls()
{
	pButtonCreateGame = GetChildChecked<IButton>( pMain, "ButtonCreateGame", true );		
	pTechLevelBox = GetChildChecked<IComboBox> ( pMain, "TechLevel", true );
	pNumPlayersBox = GetChildChecked<IComboBox> ( pMain, "NumPlayers", true );
	pSessionName = GetChildChecked<IEditLine>( pMain, "SessionName", true );
	pList = GetChildChecked<IListControl> ( pMain, "Maps", false );
	pTimeLimit = GetChildChecked<IEditLine> ( pMain, "TimeLimit", true );
	pCaptureTime = GetChildChecked<IEditLine> ( pMain, "CaptureTime", true );
	pSliderGameSpeed = GetChildChecked<ISlider>( pMain, "GameSpeed", true );
	pGameSpeedText = GetChildChecked<ITextView>( pMain, "GameSpeedText", true );
	pButtonUnitExperience = GetChildChecked<IButton>( pMain, "UnitExperience", true );
	pButtonStartPosition = GetChildChecked<IButton>( pMain, "RandomPlacement", true );
	pMapName = GetChildChecked<ITextView>( pMain,  "MapName", true );

	pAdvancedPopup = GetChildChecked<IWindow>( pMain, "AdvancedPopup", true );
	pAdvancedPassword = GetChildChecked<IEditLine>( pAdvancedPopup, "AdvancedPassword", true );
	pChecksumPWL = GetChildChecked<IWindow>( pMain,  "ChecksumPWL", true );
	if ( pChecksumPWL )
		pChecksumPWL->ShowWindow( false );
	SetControls();
}

void CInterfaceMPCreateCustomGame::SetControls()
{
	if ( pButtonCreateGame )
		pButtonCreateGame->Enable( false );
	CPtr<CTextData> pData;
	CPtr<CTextDataViewer> pViewer;
	if ( pTechLevelBox )
	{
		pViewer = new CTextDataViewer();
		pTechLevelBox->SetViewer( pViewer );		
		// Get Tech Levels from MPConsts
		const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();
		for ( int i = 0; i < pMPConsts->techLevels.size(); ++i )
		{
			if ( CHECK_TEXT_NOT_EMPTY_PRE(pMPConsts->techLevels[i].,Name) )
				pData = new CTextData( GET_TEXT_PRE(pMPConsts->techLevels[i].,Name) );
			else
				pData = new CTextData( NStr::ToUnicode( StrFmt( "!Level%d!", i ) ) );
			pTechLevelBox->AddItem( pData );
		}
		pTechLevelBox->Select( 0 );
	}
	if ( pNumPlayersBox )  
	{		
		pViewer = new CTextDataViewer(); 
		pNumPlayersBox->SetViewer( pViewer );
		pNumPlayersBox->Enable( false );
	}
	if ( pList ) 
	{
		CPtr<CItemCustomGameListViewer> pCustomViewer = new CItemCustomGameListViewer();
		pList->SetViewer( pCustomViewer );
	}
	
	if ( pTimeLimit )
		pTimeLimit->SetText( L"60" );

	if ( pCaptureTime )
		pCaptureTime->SetText( L"30" );

	if ( pSliderGameSpeed )
	{
		int nMax = pSliderGameSpeed->GetNSpecialPositions();
		pSliderGameSpeed->SetRange( 0, nMax, 1 );	
		pSliderGameSpeed->SetNotifySink( this );
		pSliderGameSpeed->SetPos( nMax / 2 );
		SliderPosition( nMax / 2, 0 );
	}

	if ( pPicture )
		pPicture->SetTexture( 0 );

	if ( pSessionName )
		pSessionName->SetText( L"" );

	if ( pButtonUnitExperience )
		pButtonUnitExperience->SetState( 1 );

	if ( pButtonStartPosition )
		pButtonStartPosition->SetState( 1 );

	if ( pAdvancedPassword )
		pAdvancedPassword->SetText( L"" );

	FillMapData();
}

void CInterfaceMPCreateCustomGame::SliderPosition( const float fPosition, CWindow *pWho )
{
	if ( !pGameSpeedText )
		return;	
	int nPos = fPosition; // 
	int nShown = nPos - pSliderGameSpeed->GetNSpecialPositions() / 2;
	std::wstring newtext = pGameSpeedText->GetDBText() + NStr::ToUnicode( StrFmt("  %+d", nShown ) );
	pGameSpeedText->SetText( newtext );
}

bool CInterfaceMPCreateCustomGame::OnSessionEnter()
{
/*	if ( pSessionName )
	{
		wszSessionName = pSessionName->GetText();
		pSessionName->SetText(L"");
	}*/
	return true;
}

void CInterfaceMPCreateCustomGame::UpdateInterior()
{	
	NI_VERIFY( pList, "No maps list", return );	

	pList->RemoveAllElements();

	for ( CMPMaps::iterator it = maps.begin(); it!= maps.end(); it++ )
	{		
		SMPMapInfo &info = *it;
		CPtr<CCustomGameListData> pData = new CCustomGameListData( info );
		IListControlItem *pNewItem = pList->AddItem( pData );
		if ( !pSelected )
		{
			pSelected = info.pMPMap;
			pList->SelectItem( pData );
			OnSelectMapReaction();
		}
	}	

	pList->Update();
}

void CInterfaceMPCreateCustomGame::CItemCustomGameListViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const
{	
	CDynamicCast<IListControlItem> pItem = pWindow;
	NI_VERIFY( pItem, "Wrong window", return );

	CDynamicCast<CCustomGameListData> pInfo = pData;	
	NI_VERIFY( pInfo, "Wrong data", return );
	SMPMapInfo &info = pInfo->info;

	NMPSetData::SetChildText( pItem,  "ItemMapName", info.wszMapName );
	NMPSetData::SetChildText( pItem,  "ItemGameType", info.wszGameType );
	NMPSetData::SetChildText( pItem,  "ItemMapSize", 
		NStr::ToUnicode( StrFmt( "%d x %d", info.nMapSizeX, info.nMapSizeY ) ) );
	NMPSetData::SetChildText( pItem,  "ItemMaxPlayers",
		NStr::ToUnicode( StrFmt( "%d",info.nPlayersMax ) ) );
}

void CInterfaceMPCreateCustomGame::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
}

bool CInterfaceMPCreateCustomGame::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "react_session_name_change" )
		return CheckEnableCreateButton();

	if ( szReaction == "react_on_back" )
		return OnBackReaction( szSender );

	if ( szReaction == "react_on_create" )
		return OnCreateGameReaction( szSender );

	if ( szReaction == "react_on_select_map" )
		return OnSelectMapReaction();	

	if ( szReaction == "session_enter" )
		return OnSessionEnter();

	if ( szReaction == "react_on_open_advanced" )
		return OnShowAdvanced( true );
	
	if ( szReaction == "react_on_close_advanced" )
		return OnShowAdvanced( false );

	return false;
}

int CInterfaceMPCreateCustomGame::Check( const std::string &szCheckName ) const
{
	return 0;
}
///////////////////////////////////	///////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceMPCreateCustomGame::OnBackReaction( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MP_CUSTOM_GAME_MENU, "" );

	// Resume updating games list
	SMPUIGameListMessage *pMsg = new SMPUIGameListMessage( true );
	Singleton<IMPToUIManager>()->AddUIMessage( pMsg );

	return true;
}

bool CInterfaceMPCreateCustomGame::OnCreateGameReaction( const std::string &szSender )
{
	if ( !pSelected )
		return true;

	if ( pChecksumPWL )
	{
		pChecksumPWL->ShowWindow( true );
		Draw();
	}

	SMPUICreateGameMessage *pCreateMsg = new SMPUICreateGameMessage;

	if ( pSessionName )
		pCreateMsg->info.szSessionName = NStr::ToMBCS( pSessionName->GetText() );

	pCreateMsg->info.szMapName = NStr::ToMBCS( GET_TEXT_PRE( pSelected->, MapName ) );
	pCreateMsg->specificInfo.pMPMap = pSelected;
	pCreateMsg->info.nPlayersMax = pSelected->nPlayers - pNumPlayersBox->GetSelectedIndex();

	std::wstring wszTime = L"???";
	if ( pTimeLimit )
		wszTime = pTimeLimit->GetText();
	int nTime = NStr::ToInt( NStr::ToMBCS ( wszTime ) );
  pCreateMsg->specificInfo.nTimeLimit = (std::max)( nTime, 5 );

	if ( pSliderGameSpeed )
	{
		int nShown = pSliderGameSpeed->GetPos() - pSliderGameSpeed->GetNSpecialPositions() / 2;
		pCreateMsg->specificInfo.nGameSpeed = nShown;
	}	
	
	pCreateMsg->specificInfo.nTechLevel = pTechLevelBox->GetSelectedIndex();
	bool bUnitExp = false;
	CPtr<IButton> pUnitExperience = GetChildChecked<IButton>( pMain, "UnitExperience", true );
	if ( pUnitExperience )
		pCreateMsg->specificInfo.bUnitExp = ( pUnitExperience->GetState() == 1 );

	if ( pButtonStartPosition )
		pCreateMsg->specificInfo.bRandomPlacement = ( pButtonStartPosition->GetState() == 1 );

	wszTime = pCaptureTime->GetText();
	int nCaptureTime = NStr::ToInt( NStr::ToMBCS ( wszTime ) );
	pCreateMsg->specificInfo.nCaptureTime = nCaptureTime;

	std::string szPassword = NStr::ToMBCS( pAdvancedPassword->GetText() );
	pCreateMsg->info.bPwdReq = ( !szPassword.empty() );
	pCreateMsg->szPassword = szPassword;

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MP_GAME_ROOM, "" );

	Singleton<IMPToUIManager>()->AddUIMessage( pCreateMsg );

	return true;
}

bool CInterfaceMPCreateCustomGame::OnSelectMapReaction()
{	
	IListControlItem *pItem = pList->GetSelectedListItem();
	CDynamicCast<CCustomGameListData> pInfo = pItem->GetUserData();
	if ( pInfo )
		pSelected = pInfo->info.pMPMap;	

	bool bEnableCreate = false;
	bool bEnableGameType = false;
	bool bEnableNumPlayers = false;	
	std::wstring wszName = L"";
	CDBPtr <NDb::STexture> pMiniMapTexture = 0;

	CPtr<IMiniMap> pWindowMiniMap = GetChildChecked<IMiniMap>( pMain, "Minimap", true );
	
	if ( !pSelected || !pSelected->pMap )
	{
		NI_ASSERT( 0, "DATA: Invalid MP map descriptor" );
		pSelected = 0;
		return true;
	}

	bEnableCreate = true;				
	wszName = GET_TEXT_PRE( pSelected->, MapName );

	/*if ( pMapInfo->pLocalizedDescription ) //Map info
		wszDesc = pMapInfo->pLocalizedDescription->wszText;*/

	const CDBPtr<NDb::SMaterial> pMiniMap = pSelected->pMap->pMiniMap;
	if ( pMiniMap ) //MiniMap
	{
		if ( pMiniMap->pTexture )
		{		
			if ( pWindowMiniMap )
			{
				pWindowMiniMap->SetLoadingMapParams( pSelected->pMap->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE,
					pSelected->pMap->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE );
				pWindowMiniMap->SetMaterial( pMiniMap );
				pWindowMiniMap->ShowWindow( true );
			}
		}		
	}

	CPtr<CTextData> pData;

	NMPSetData::SetText( pMapName, wszName);

	if ( pPicture )
		pPicture->SetTexture( pMiniMapTexture );	

	CheckEnableCreateButton();

	/*if ( pGameTypeBox )
		pGameTypeBox->Enable( bEnableCreate );
	if ( pGameTypeBox )
	{
	}*/

	if ( pNumPlayersBox )
		pNumPlayersBox->Enable( bEnableNumPlayers );
	if ( pNumPlayersBox )
	{	
		pNumPlayersBox->RemoveAllItems();						
		for ( int i = pSelected->nPlayers; i > 1; i-- )		
		{
			CPtr<CTextData> pData = new CTextData( i );
			pNumPlayersBox->AddItem( pData );	
			pNumPlayersBox->Enable( true );			
		}			
		pNumPlayersBox->Select( 0 );
	}
	return true;
}

bool CInterfaceMPCreateCustomGame::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

void CInterfaceMPCreateCustomGame::AfterLoad()
{
	CInterfaceScreenBase::AfterLoad();

	RegisterObservers();
}

bool CInterfaceMPCreateCustomGame::CheckEnableCreateButton()
{
	std::wstring wszSessionName = pSessionName->GetText();
	pButtonCreateGame->Enable( pSelected != 0 && !wszSessionName.empty() );
	return true;
}

bool CInterfaceMPCreateCustomGame::OnShowAdvanced( const bool bShow )
{
	pAdvancedPopup->ShowWindow( bShow );
	return true;
}

// CICMPCreateCustomGame

void CICMPCreateCustomGame::PreCreate()
{
}

void CICMPCreateCustomGame::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMPCreateCustomGame::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x32197340, CInterfaceMPCreateCustomGame );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MP_CREATE_CUSTOM_GAME_MENU, CICMPCreateCustomGame );


