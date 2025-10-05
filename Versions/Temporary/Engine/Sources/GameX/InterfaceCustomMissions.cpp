#include "stdafx.h"
#include "InterfaceCustomMissions.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "DBGameRoot.h"
#include "ScenarioTracker.h"
#include "Misc/StrProc.h"
#include "System/Text.h"
#include "CustomMissions.h"

const int CUSTOM_MISSION_DEFAULT_DIFFICULTY = 1;

namespace // unnamed
{

class CDifficultyData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CDifficultyData )
public:
	ZDATA
	std::wstring wszText;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszText); return 0; }
	
	CDifficultyData() {}
	CDifficultyData( const std::wstring &_wszText ) { wszText = _wszText; }
};

class CDifficultyTextViewer : public IDataViewer
{
	OBJECT_NOCOPY_METHODS(CDifficultyTextViewer)
public:
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
};

// CDifficultyTextViewer

void CDifficultyTextViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const
{
	CDynamicCast<IListControlItem> pItem = pWindow;
	NI_VERIFY( pItem, "Wrong window", return );
	
	CDynamicCast<ITextView> pView = pItem->GetSubItem( 0 );
	NI_VERIFY( pView, "Wrong window", return );
	
	std::wstring wszText;
	if ( pData )
	{
		CDynamicCast<CDifficultyData> pText = pData;
		NI_VERIFY( pText, "Wrong data", return );
		
		wszText = pText->wszText;
	}
	pView->SetText( pView->GetDBText() + wszText );
}

} // namespace - unnamed

// CInterfaceCustomMissions

CInterfaceCustomMissions::CInterfaceCustomMissions() : 
	CInterfaceScreenBase( "CustomMissions", "custom_missions" )
{
}

bool CInterfaceCustomMissions::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );
	
	MakeInterior();

	return true;
}

void CInterfaceCustomMissions::MakeInterior()
{
	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_SINGLE );

	pMainWnd = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	pTopPanel = GetChildChecked<IWindow>( pMainWnd, "TopPanel", true );
	pBottomPanel = GetChildChecked<IWindow>( pMainWnd, "BottomPanel", true );
	pMissionsListPanel = GetChildChecked<IWindow>( pMainWnd, "LeftPanel", true );
	pMinimapPanel = GetChildChecked<IWindow>( pMainWnd, "MinimapPanel", true );
	pMissionDescPanel = GetChildChecked<IWindow>( pMainWnd, "MissionDescPanel", true );

	pMissionsListItemTemplate = GetChildChecked<IWindow>( pMissionsListPanel, "MissionsListItem", true );
	pMissionsListCont = GetChildChecked<IScrollableContainer>( pMissionsListPanel, "MissionsListCont", true );
	
	if ( pMissionsListItemTemplate )
		pMissionsListItemTemplate->ShowWindow( false );
	
	pMissionDescView = GetChildChecked<ITextView>( pMissionDescPanel, "MissionDescView", true );
	pMissionDescCont = GetChildChecked<IScrollableContainer>( pMissionDescPanel, "MissionDescCont", true );
	pMissionDescItemHeader = GetChildChecked<IWindow>( pMissionDescPanel, "MissionDescItemHeader", true );
	pMissionDescItemNameView = GetChildChecked<ITextView>( pMissionDescPanel, "MissionName", true );
	pMissionDescItemSeasonView = GetChildChecked<ITextView>( pMissionDescPanel, "Season", true );

	if ( pMissionDescCont )
	{
		if ( pMissionDescItemHeader )
			pMissionDescCont->PushBack( pMissionDescItemHeader, false );
		if ( pMissionDescView )
			pMissionDescCont->PushBack( pMissionDescView, false );
	}

	pMinimap = GetChildChecked<IMiniMap>( pMinimapPanel, "Minimap", true );
	
	pDifficultyComboBox = GetChildChecked<IComboBox>( pBottomPanel, "Difficulty", true );
	pPlayBtn = GetChildChecked<IButton>( pBottomPanel, "PlayBtn", true );

	if ( pDifficultyComboBox )
		pDifficultyComboBox->SetViewer( new CDifficultyTextViewer() );

	std::vector<CDBID> dbIDs;
	NCustom::GetCustomMissions( &dbIDs );
	for ( int i = 0; i < dbIDs.size(); ++i )
	{
		const NDb::SMapInfo *pMapInfo = NDb::Get<NDb::SMapInfo>( dbIDs[i] );

		AddMission( pMapInfo );
	}
	
	IWindow *pFirstLineWnd = 0;
	if ( !missions.empty() )
		pFirstLineWnd = missions.front().pWnd;
	if ( pMissionsListCont && pFirstLineWnd )
		pMissionsListCont->Select( pFirstLineWnd );

	UpdateSelection( true );

	if ( pMainWnd )
		SetMainWindowTexture( pMainWnd, InterfaceState()->GetMenuBackgroundTexture() );
}

void CInterfaceCustomMissions::AddMission( const NDb::SMapInfo *pMapInfo )
{
	if ( !pMapInfo )
		return;

	SMission mission;
	mission.pMapInfo = pMapInfo;
	mission.pWnd = AddWindowCopy( pMissionsListCont, pMissionsListItemTemplate );
	mission.pFlagWnd = GetChildChecked<IWindow>( mission.pWnd, "Flag", true );
	mission.pNameView = GetChildChecked<ITextView>( mission.pWnd, "MissionNameView", true );
	mission.pSizeView = GetChildChecked<ITextView>( mission.pWnd, "MissionMapSizeView", true );
	
	const NDb::STexture *pFlag = 0;
	if ( !pMapInfo->players.empty() )
	{
		const NDb::SMapPlayerInfo &playerInfo = pMapInfo->players.front();
		if ( playerInfo.pPartyInfo )
		{
			pFlag = playerInfo.pPartyInfo->pStatisticsIcon;
		}
	}

	if ( CHECK_TEXT_NOT_EMPTY_PRE(pMapInfo->,LocalizedName) )
		mission.wszName = GET_TEXT_PRE(pMapInfo->,LocalizedName);
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pMapInfo->,LocalizedDescription) )
		mission.wszDesc = GET_TEXT_PRE(pMapInfo->,LocalizedDescription);

	mission.wszSize = InterfaceState()->GetMapSizeName( pMapInfo );
	mission.wszSeason = InterfaceState()->GetSeasonName( pMapInfo->eSeason );
	
	if ( mission.pFlagWnd )
		mission.pFlagWnd->SetTexture( pFlag );
	if ( mission.pNameView )
		mission.pNameView->SetText( mission.pNameView->GetDBText() + mission.wszName );
	if ( mission.pSizeView )
		mission.pSizeView->SetText( mission.pSizeView->GetDBText() + mission.wszSize );
	
	if ( mission.pWnd )
		mission.pWnd->ShowWindow( true );
	if ( pMissionsListCont )
		pMissionsListCont->PushBack( mission.pWnd, true );

	missions.push_back( mission );
}

bool CInterfaceCustomMissions::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive );
}

const CInterfaceCustomMissions::SMission* CInterfaceCustomMissions::FindSelected() const
{
	IWindow *pSelectedWnd = 0;
	if ( pMissionsListCont )
		pSelectedWnd = pMissionsListCont->GetSelectedItem();
	if ( !pSelectedWnd )
		return 0;
		
	for ( int i = 0; i < missions.size(); ++i )
	{
		const SMission &mission = missions[i];
		if ( mission.pWnd == pSelectedWnd )
			return &mission;
	}
	
	return 0;
}

bool CInterfaceCustomMissions::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "menu_back" )
		return OnBack();
	if ( szReaction == "menu_play" )
		return OnPlay();
	if ( szReaction == "menu_select" )
		return OnSelect();
	if ( szReaction == "menu_dbl_click" )
		return OnDblClick();

	return false;
}

int CInterfaceCustomMissions::Check( const std::string &szCheckName ) const
{
	return 0;
}

bool CInterfaceCustomMissions::OnBack()
{
	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "single_player_submenu" );

	return true;
}

bool CInterfaceCustomMissions::OnPlay()
{
	const SMission *pMission = FindSelected();
	if ( pMission )
		MissionStart( pMission->pMapInfo );

	return true;
}

bool CInterfaceCustomMissions::OnSelect()
{
	UpdateSelection( false );
	
	return true;
}

bool CInterfaceCustomMissions::OnDblClick()
{
	const SMission *pMission = FindSelected();
	if ( pMission )
		MissionStart( pMission->pMapInfo );

	return true;
}

void CInterfaceCustomMissions::MissionStart( const NDb::SMapInfo *pMapInfo )
{
	if ( !pMapInfo )
		return;

	int nDifficulty = 0;
	if ( pDifficultyComboBox )
		nDifficulty = pDifficultyComboBox->GetSelectedIndex();

	Singleton<IScenarioTracker>()->CustomMissionStart( pMapInfo, nDifficulty, false );
	
//	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
//	NMainLoop::Command( ML_COMMAND_MISSION, StrFmt( "%s;normal", pMapInfo->GetDBID().ToString().c_str() ) );
	NMainLoop::Command( ML_COMMAND_MISSION_BRIEFING, "" );
}

void CInterfaceCustomMissions::UpdateSelection( bool bFirstTime )
{
	const SMission *pMission = FindSelected();
		
	if ( !pMission || !pMission->pWnd )
	{
		if ( pMinimap )
			pMinimap->ShowWindow( false );

		if ( pMissionDescCont )
			pMissionDescCont->ShowWindow( false );

		if ( pPlayBtn )
			pPlayBtn->Enable( false );
		if ( pDifficultyComboBox )
			pDifficultyComboBox->ShowWindow( false );
			
		return;
	}
	
	if ( pMinimap && pMission->pMapInfo )
	{
		pMinimap->ShowWindow( true );

		float x = pMission->pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE;
		float y = pMission->pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE;

		const NDb::STexture *pTexture = 0;
		if ( pMission->pMapInfo->pMiniMap )
			pTexture = pMission->pMapInfo->pMiniMap->pTexture;

		pMinimap->SetLoadingMapParams( x, y );
		pMinimap->SetTexture( pTexture );
	}

	if ( pMissionDescItemNameView )
		pMissionDescItemNameView->SetText( pMissionDescItemNameView->GetDBText() + pMission->wszName );
	if ( pMissionDescItemSeasonView )
		pMissionDescItemSeasonView->SetText( pMissionDescItemSeasonView->GetDBText() + pMission->wszSeason );
	if ( pMissionDescView )
		pMissionDescView->SetText( pMissionDescView->GetDBText() + pMission->wszDesc );

	if ( pMissionDescCont )
	{
		pMissionDescCont->ShowWindow( true );
		pMissionDescCont->Update();
	}

	if ( pPlayBtn )
		pPlayBtn->Enable( true );
		
	UpdateDifficulty( pMission, bFirstTime );
}

void CInterfaceCustomMissions::UpdateDifficulty( const SMission *pMission, bool bFirstTime )
{
	if ( !pDifficultyComboBox )
		return;
		
	const int nOldDifficulty = bFirstTime ? CUSTOM_MISSION_DEFAULT_DIFFICULTY : pDifficultyComboBox->GetSelectedIndex();
	pDifficultyComboBox->RemoveAllItems();

	if ( !pMission || !pMission->pMapInfo || pMission->pMapInfo->customDifficultyLevels.empty() )
	{
		pDifficultyComboBox->ShowWindow( false );
		return;
	}
	
	const std::vector< CDBPtr< NDb::SDifficultyLevel > > &difficultyLevels = pMission->pMapInfo->customDifficultyLevels;

	const int nNewDifficulty = (nOldDifficulty >= 0 && nOldDifficulty < difficultyLevels.size() ) ? nOldDifficulty : 
		Max( 0, Min( CUSTOM_MISSION_DEFAULT_DIFFICULTY, (int)( difficultyLevels.size() ) - 1 ) );

	pDifficultyComboBox->ShowWindow( true );

	for ( int i = 0; i < difficultyLevels.size(); ++i )
	{
		const NDb::SDifficultyLevel *pDifficultyLevel = difficultyLevels[i];
		if ( pDifficultyLevel && CHECK_TEXT_NOT_EMPTY_PRE(pDifficultyLevel->,LocalizedName) )
			pDifficultyComboBox->AddItem( new CDifficultyData( GET_TEXT_PRE(pDifficultyLevel->,LocalizedName) ) );
		else
			pDifficultyComboBox->AddItem( new CDifficultyData( L"******" ) );
	}
	
	pDifficultyComboBox->Select( nNewDifficulty );
}

#if !defined(_SINGLE_DEMO) || defined(_MP_DEMO)
// CICCustomMissions

void CICCustomMissions::PreCreate()
{
}

void CICCustomMissions::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICCustomMissions::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x1716B381, CInterfaceCustomMissions )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_CUSTOM_MISSIONS, CICCustomMissions )
REGISTER_SAVELOAD_CLASS( 0x172623C0, CDifficultyTextViewer )
REGISTER_SAVELOAD_CLASS( 0x172623C1, CDifficultyData )

#endif // !defined(_SINGLE_DEMO) || defined(_MP_DEMO)
