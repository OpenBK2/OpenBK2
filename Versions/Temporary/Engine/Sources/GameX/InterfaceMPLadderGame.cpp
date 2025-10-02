#include "StdAfx.h"
#include "InterfaceMPLadderGame.h"
#include "GameXClassIDs.h"
#include "UI/SceneClassIDs.h"
#include "SceneB2/Scene.h"
#include "InterfaceState.h"
#include "InterfaceMisc.h"
#include "Misc/StrProc.h"

#include "DBGameRoot.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "UISpecificB2/DBUIspecificB2.h"
#include "GameRoomData.h"
#include "System/Text.h"
#include "CustomMissions.h"
#include "MapSizeSorter.h"

// CInterfaceMPLadderGame

CInterfaceMPLadderGame::CInterfaceMPLadderGame() : 
CInterfaceScreenBase( "MPLadderGame", "ladder_game" )
{
}

CInterfaceMPLadderGame::~CInterfaceMPLadderGame()
{
	if ( pScreen ) 
		Scene()->RemoveScreen( pScreen );
}

bool CInterfaceMPLadderGame::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	RegisterObservers();

	// load screen
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	if ( AddUIScreen( pScreen, "MPLadderGame", this ) == false )
		return false;
	pMain = GetChildChecked<IWindow>( pScreen, "Main", true );
	NI_VERIFY( pMain, "No main window found", return false );	

	InitControls();
	return true;
}

void CInterfaceMPLadderGame::RegisterObservers()
{	
}

void CInterfaceMPLadderGame::FillMapData()
{
	const NDb::SGameRoot *pGR = NGameX::GetGameRoot();
	NI_VERIFY( pGR, "Unable to get GameRoot", return );

	maps.resize( pGR->multiplayerMaps.size() );
	for ( int i = 0; i < pGR->multiplayerMaps.size(); ++i )
		maps[i] = new CLadderMapData( pGR->multiplayerMaps[i] );

	UpdateInterior();
}

void CInterfaceMPLadderGame::InitControls()
{
	pGameTypeBox = GetChildChecked<IComboBox> ( pMain, "Players", true );
	pSide =  GetChildChecked<IComboBox> ( pMain, "Side", true );
	pTechLevel = GetChildChecked<IComboBox> ( pMain, "TechLevel", true );
	pList = GetChildChecked<IListControl> ( pMain, "Maps", true );
	pMapName = GetChildChecked<ITextView>( pMain,  "MapName", true );
	pStartButton = GetChildChecked<IButton>( pMain,  "ButtonStartGame", true );
	if ( pStartButton )
		pStartButton->Enable( false );

	pChecksumPWL = GetChildChecked<IWindow>( pMain,  "ChecksumPWL", true );
	if ( pChecksumPWL )
		pChecksumPWL->ShowWindow( false );

	SetControls();
}

void CInterfaceMPLadderGame::SetControls()
{
	CPtr<CTextData> pData;
	CPtr<CTextDataViewer> pViewer;
	wstring wszAnyText = InterfaceState()->GetTextEntry( "T_MP_GAME_ROOM_ANY" );
	const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();

	if ( pSide )
	{
		pViewer = new CTextDataViewer(); 
		pSide->SetViewer( pViewer );	
		pData = new CTextData( wszAnyText );
		pSide->AddItem( pData );
		pSide->Select( 0 );

		if ( pMPConsts )
		{
			typedef const vector< NDb::SMultiplayerSide> MPSides;
			MPSides &sides = pMPConsts->sides;
			for ( MPSides::const_iterator it = sides.begin(); it != sides.end(); it++ )
			{
				const NDb::SMultiplayerSide &side = *it;
				if ( CHECK_TEXT_NOT_EMPTY_PRE(side.,Name) )
				{
					pData = new CTextData( GET_TEXT_PRE(side.,Name) );
					pSide->AddItem( pData );
				}
			}		
		}
	}

	if ( pTechLevel )
	{
		pViewer = new CTextDataViewer(); 
		pTechLevel->SetViewer( pViewer );	
		pData = new CTextData( wszAnyText );
		pTechLevel->AddItem( pData );
		pTechLevel->Select( 0 );
		if ( pMPConsts )
		{
			typedef const vector< NDb::SMultiplayerTechLevel> MPTechLevels;
			MPTechLevels &techlevels = pMPConsts->techLevels;
			for ( MPTechLevels::const_iterator it = techlevels.begin(); it != techlevels.end(); it++ )
			{
				const NDb::SMultiplayerTechLevel &techlevel = *it;
				if ( CHECK_TEXT_NOT_EMPTY_PRE(techlevel.,Name) )
				{
					pData = new CTextData( GET_TEXT_PRE(techlevel.,Name) );
					pTechLevel->AddItem( pData );
				}
			}		
		}
	}

	if ( pGameTypeBox )
	{
		pViewer = new CTextDataViewer(); 
		pGameTypeBox->SetViewer( pViewer );		
		pData = new CTextData( wszAnyText );
		pGameTypeBox->AddItem( pData );
		for ( int i = 1; i < 5; ++i )
		{
			pData = new CTextData( NStr::ToUnicode( StrFmt( "%d v %d", i, i ) ) );
			pGameTypeBox->AddItem( pData );
		}
		pGameTypeBox->Select( 0 );		
	}

	if ( pList ) 
	{
		CPtr<CItemLadderMapListViewer> pCustomViewer = new CItemLadderMapListViewer();
		pList->SetViewer( pCustomViewer );
		IWindowSorter *pSizeSorter = new CListControlSorterMapSize;
		pSizeSorter->SetColumn( 3 );
		pList->SetSorter( pSizeSorter, 3 );
	}

	if ( pPicture )
		pPicture->SetTexture( 0 );

	FillMapData();
	CheckEnableStartButton();
}

void CInterfaceMPLadderGame::CheckEnableStartButton()
{
	int nMaps = 0;
	for ( int i = 0; i < maps.size(); ++i )
	{
		CLadderMapData &mapData = *(maps[i]);
		mapData.bSelected = ( mapData.pSwitch->GetState() == 1 );
		if ( mapData.bSelected && mapData.bAllowed )
			++nMaps;
	}

	pStartButton->Enable( nMaps > 0 );
}

void CInterfaceMPLadderGame::UpdateInterior()
{	
	NI_VERIFY( pList, "No maps list", return );	

	pList->RemoveAllElements();
	for ( CMPMaps::iterator it = maps.begin(); it!= maps.end(); it++ )
	{		
		pList->AddItem( *it );
	}
	pList->Update();
}

void CInterfaceMPLadderGame::CItemLadderMapListViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const
{	
	CDynamicCast<IListControlItem> pItem = pWindow;
	NI_VERIFY( pItem, "Wrong window", return );
	CDynamicCast<CLadderMapData> pInfo = pData;	
	NI_VERIFY( pInfo, "Wrong data", return );

	NMPSetData::SetChildText( pItem,  "ItemMapName", GET_TEXT_PRE( pInfo->pMapDesc->, MapName ) );
	NMPSetData::SetChildText( pItem,  "ItemMapSize", 	NStr::ToUnicode( StrFmt( "%d x %d", pInfo->pMapDesc->nSizeX, pInfo->pMapDesc->nSizeY ) ) );
	NMPSetData::SetChildText( pItem,  "ItemMaxPlayers", NStr::ToUnicode( StrFmt( "%d", pInfo->pMapDesc->nPlayers ) ) );
	pInfo->pSwitch = GetChildChecked<IButton>( pItem, "Status", true );
	if ( pInfo->pSwitch )
	{
		pInfo->pSwitch->ShowWindow( pInfo->bAllowed );
		pInfo->pSwitch->SetState( pInfo->bSelected ? 1 : 0 );
	}
}

bool CInterfaceMPLadderGame::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "react_on_back" )
		return OnBackReaction( szSender );

	if ( szReaction == "react_on_start" )
		return OnStartGameReaction( szSender );

	if ( szReaction == "react_on_select_map" )
		return OnSelectMapReaction( szSender );

	if ( szReaction == "react_on_team_size" )
		return OnTeamSizeChanged();

	if ( szReaction == "react_on_map_checked" )
		return OnChangeMapStatusReaction();

	return false;
}

int CInterfaceMPLadderGame::Check( const string &szCheckName ) const
{
	return 0;
}
///////////////////////////////////	///////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceMPLadderGame::OnBackReaction( const string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	return true;
}

bool CInterfaceMPLadderGame::OnStartGameReaction( const string &szSender )
{
	if ( pChecksumPWL )
	{
		pChecksumPWL->ShowWindow( true );
		Draw();
	}

	const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();
	CPtr<SMPUILadderGameMessage> pMsg = new SMPUILadderGameMessage();
	CPtr<IButton> pHistoricalOpponents = GetChildChecked<IButton>( pMain, "HistoricalOpponents", true );
	if ( pHistoricalOpponents )
		pMsg->bHistoricity = ( pHistoricalOpponents->GetState() == 1 );
	if ( pGameTypeBox )
		pMsg->nTeamSize = pGameTypeBox->GetSelectedIndex();
	if ( pSide )
		pMsg->nCountry = pSide->GetSelectedIndex();
	// Tech Levels
	int nTechLevel = 0;
	if ( pTechLevel )
		nTechLevel = pTechLevel->GetSelectedIndex();
	for ( int i = 0; i < pMPConsts->techLevels.size(); ++i )
	{
		if ( nTechLevel <= 0 || nTechLevel - 1 == i )
			pMsg->techLevels.push_back( i );
	}
	// Maps
	for ( int i = 0; i < maps.size(); ++i )
	{
		CLadderMapData &mapData = *(maps[i]);
		if ( mapData.pSwitch->GetState() == 1 && mapData.bAllowed )
			pMsg->maps.push_back( i );
	}
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	Singleton<IMPToUIManager>()->AddUIMessage( pMsg );
	return true;
}

bool CInterfaceMPLadderGame::OnSelectMapReaction( const string &szSender )
{	
	IListControlItem *pItem = pList->GetSelectedListItem();
	CDynamicCast<CLadderMapData> pInfo = pItem->GetUserData();
	CDBPtr<NDb::SMultiplayerMap> pSelected = pInfo->pMapDesc;
	wstring wszName = L"";
	CDBPtr<NDb::STexture> pMiniMapTexture = 0;
	CPtr<IMiniMap> pWindowMiniMap = GetChildChecked<IMiniMap>( pMain, "Minimap", true );

	if ( !pSelected || !pSelected->pMap )
	{
		NI_ASSERT( 0, StrFmt( "DATA: Invalid MP map descriptor: %s", NDb::GetResName( pSelected )  ) );
		return true;
	}
	wszName = GET_TEXT_PRE( pSelected->, MapName );
	CDBPtr<NDb::SMaterial> pMiniMap = pSelected->pMap->pMiniMap;
	if ( pMiniMap )
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
	NMPSetData::SetText( pMapName, wszName);
	if ( pPicture )
		pPicture->SetTexture( pMiniMapTexture );	

	return true;
}

void CInterfaceMPLadderGame::AfterLoad()
{
	CInterfaceScreenBase::AfterLoad();

	RegisterObservers();
}

bool CInterfaceMPLadderGame::OnTeamSizeChanged()
{
	int nMinPlayers = pGameTypeBox->GetSelectedIndex() * 2;
	for ( int i = 0; i < maps.size(); ++i )
	{
		CLadderMapData &mapData = *(maps[i]);
		mapData.bAllowed = ( mapData.pMapDesc->nPlayers >= nMinPlayers );
		mapData.pSwitch->ShowWindow( mapData.bAllowed );
	}
	pList->Update();

	CheckEnableStartButton();
	return true;
}

bool CInterfaceMPLadderGame::OnChangeMapStatusReaction()
{
	CheckEnableStartButton();
	return true;
}

bool CInterfaceMPLadderGame::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive );
}

// CICMPLadderGame

void CICMPLadderGame::PreCreate()
{
}

void CICMPLadderGame::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMPLadderGame::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x321B6380, CInterfaceMPLadderGame );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MP_LADDER_GAME, CICMPLadderGame );


