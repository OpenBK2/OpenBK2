#include "stdafx.h"
#include "InterfaceCustomCampaign.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "DBGameRoot.h"
#include "ScenarioTracker.h"
#include "Misc/StrProc.h"
#include "System/Text.h"
#include "CustomMissions.h"
#include "CampaignSelectionMenu.h"

#include "GameX_export.h"

const int CUSTOM_CAMPAIGN_DEFAULT_DIFFICULTY = 1;

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

// CInterfaceCustomCampaign

CInterfaceCustomCampaign::CInterfaceCustomCampaign() : 
	CInterfaceScreenBase( "CustomCampaign", "custom_campaign" )
{
}

bool CInterfaceCustomCampaign::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );
	
	MakeInterior();

	return true;
}

void CInterfaceCustomCampaign::MakeInterior()
{
	pMainWnd = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	pTopPanel = GetChildChecked<IWindow>( pMainWnd, "TopPanel", true );
	pBottomPanel = GetChildChecked<IWindow>( pMainWnd, "BottomPanel", true );
	pCampaignsListPanel = GetChildChecked<IWindow>( pMainWnd, "LeftPanel", true );
	pCampaignDescPanel = GetChildChecked<IWindow>( pMainWnd, "RightPanel", true );

	pCampaignsListCont = GetChildChecked<IScrollableContainer>( pCampaignsListPanel, "CampaignsListCont", true );
	pCampaignsListItemTemplate = GetChildChecked<IWindow>( pCampaignsListPanel, "CampaignsListItem", true );
	
	if ( pCampaignsListItemTemplate )
		pCampaignsListItemTemplate->ShowWindow( false );

	pCampaignDescCont = GetChildChecked<IScrollableContainer>( pCampaignDescPanel, "CampaignDescCont", true );
	pCampaignDescView = GetChildChecked<ITextView>( pCampaignDescPanel, "CampaignDescItemView", true );
	
	if ( pCampaignDescCont )
	{
		if ( pCampaignDescView )
			pCampaignDescCont->PushBack( pCampaignDescView, false );
	}
	
	pDifficultyComboBox = GetChildChecked<IComboBox>( pBottomPanel, "Difficulty", true );
	pPlayBtn = GetChildChecked<IButton>( pBottomPanel, "PlayBtn", true );

	if ( pDifficultyComboBox )
		pDifficultyComboBox->SetViewer( new CDifficultyTextViewer() );

	std::vector<CDBID> dbIDs;
	NCustom::GetCustomCampaigns( &dbIDs );
	for ( int i = 0; i < dbIDs.size(); ++i )
	{
		const NDb::SCampaign *pCampaign = NDb::Get<NDb::SCampaign>( dbIDs[i] );

		AddCampaign( pCampaign );
	}
	
	IWindow *pFirstLineWnd = 0;
	if ( !campaigns.empty() )
		pFirstLineWnd = campaigns.front().pWnd;
	if ( pCampaignsListCont && pFirstLineWnd )
		pCampaignsListCont->Select( pFirstLineWnd );

	UpdateSelection( true );

	if ( pMainWnd )
		SetMainWindowTexture( pMainWnd, InterfaceState()->GetMenuBackgroundTexture() );
}

void CInterfaceCustomCampaign::AddCampaign( const NDb::SCampaign *pCampaignDB )
{
	if ( !pCampaignDB )
		return;

	SCampaign campaign;
	campaign.pCampaignDB = pCampaignDB;
	campaign.pWnd = AddWindowCopy( pCampaignsListCont, pCampaignsListItemTemplate );
	campaign.pFlagWnd = GetChildChecked<IWindow>( campaign.pWnd, "Flag", true );
	campaign.pNameView = GetChildChecked<ITextView>( campaign.pWnd, "Name", true );
	
	const NDb::STexture *pFlag = pCampaignDB->pTextureMenuIcon;

	if ( CHECK_TEXT_NOT_EMPTY_PRE(pCampaignDB->,LocalizedName) )
		campaign.wszName = GET_TEXT_PRE(pCampaignDB->,LocalizedName);
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pCampaignDB->,LocalizedDesc) )
		campaign.wszDesc = GET_TEXT_PRE(pCampaignDB->,LocalizedDesc);

	if ( campaign.pFlagWnd )
		campaign.pFlagWnd->SetTexture( pFlag );
	if ( campaign.pNameView )
		campaign.pNameView->SetText( campaign.pNameView->GetDBText() + campaign.wszName );
	
	if ( campaign.pWnd )
		campaign.pWnd->ShowWindow( true );
	if ( pCampaignsListCont )
		pCampaignsListCont->PushBack( campaign.pWnd, true );

	campaigns.push_back( campaign );
}

const CInterfaceCustomCampaign::SCampaign* CInterfaceCustomCampaign::FindSelected() const
{
	IWindow *pSelectedWnd = 0;
	if ( pCampaignsListCont )
		pSelectedWnd = pCampaignsListCont->GetSelectedItem();
	if ( !pSelectedWnd )
		return 0;
		
	for ( int i = 0; i < campaigns.size(); ++i )
	{
		const SCampaign &campaign = campaigns[i];
		if ( campaign.pWnd == pSelectedWnd )
			return &campaign;
	}
	
	return 0;
}

bool CInterfaceCustomCampaign::Execute( const std::string &szSender, const std::string &szReaction )
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

int CInterfaceCustomCampaign::Check( const std::string &szCheckName ) const
{
	return 0;
}

bool CInterfaceCustomCampaign::OnBack()
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "single_player_submenu" );

	return true;
}

bool CInterfaceCustomCampaign::OnPlay()
{
	const SCampaign *pCampaign = FindSelected();
	if ( pCampaign )
		CampaignStart( pCampaign->pCampaignDB );

	return true;
}

bool CInterfaceCustomCampaign::OnSelect()
{
	UpdateSelection( false );
	
	return true;
}

bool CInterfaceCustomCampaign::OnDblClick()
{
	const SCampaign *pCampaign = FindSelected();
	if ( pCampaign )
		CampaignStart( pCampaign->pCampaignDB );

	return true;
}

void CInterfaceCustomCampaign::CampaignStart( const NDb::SCampaign *pCampaignDB )
{
	if ( !pCampaignDB )
		return;

	int nDifficulty = 0;
	if ( pDifficultyComboBox )
		nDifficulty = pDifficultyComboBox->GetSelectedIndex();
#if !defined(_SINGLE_DEMO) && !defined(_MP_DEMO)
	StartCampaign( pCampaignDB, nDifficulty, true );
#endif // !defined(_SINGLE_DEMO) && !defined(_MP_DEMO)
}

void CInterfaceCustomCampaign::UpdateSelection( bool bFirstTime )
{
	const SCampaign *pCampaign = FindSelected();
		
	if ( !pCampaign || !pCampaign->pWnd )
	{
		if ( pCampaignDescCont )
			pCampaignDescCont->ShowWindow( false );

		if ( pPlayBtn )
			pPlayBtn->Enable( false );
		if ( pDifficultyComboBox )
			pDifficultyComboBox->ShowWindow( false );
			
		return;
	}
	
	if ( pCampaignDescView )
		pCampaignDescView->SetText( pCampaignDescView->GetDBText() + pCampaign->wszDesc );

	if ( pCampaignDescCont )
	{
		pCampaignDescCont->ShowWindow( true );
		pCampaignDescCont->Update();
	}

	if ( pPlayBtn )
		pPlayBtn->Enable( true );
		
	UpdateDifficulty( pCampaign, bFirstTime );
}

void CInterfaceCustomCampaign::UpdateDifficulty( const SCampaign *pCampaign, bool bFirstTime )
{
	if ( !pDifficultyComboBox )
		return;
		
	const int nOldDifficulty = bFirstTime ? CUSTOM_CAMPAIGN_DEFAULT_DIFFICULTY : pDifficultyComboBox->GetSelectedIndex();
	pDifficultyComboBox->RemoveAllItems();

	if ( !pCampaign || !pCampaign->pCampaignDB || pCampaign->pCampaignDB->difficultyLevels.empty() )
	{
		pDifficultyComboBox->ShowWindow( false );
		return;
	}
	
	const std::vector< CDBPtr< NDb::SDifficultyLevel > > &difficultyLevels = pCampaign->pCampaignDB->difficultyLevels;

	const int nNewDifficulty = (nOldDifficulty >= 0 && nOldDifficulty < difficultyLevels.size() ) ? nOldDifficulty : 
		(std::max)( 0, (std::min)( CUSTOM_CAMPAIGN_DEFAULT_DIFFICULTY, (int)( difficultyLevels.size() ) - 1 ) );

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

bool CInterfaceCustomCampaign::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive );
}

// CICCustomCampaign

void CICCustomCampaign::PreCreate()
{
}

void CICCustomCampaign::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICCustomCampaign::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x17263381, CInterfaceCustomCampaign )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_CUSTOM_CAMPAIGN, CICCustomCampaign )
REGISTER_SAVELOAD_CLASS( GAMEX, 0x17263382, CDifficultyTextViewer )
REGISTER_SAVELOAD_CLASS( GAMEX, 0x17263383, CDifficultyData )


