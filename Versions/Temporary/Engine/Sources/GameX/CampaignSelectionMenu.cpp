#include "stdafx.h"
#include "CampaignSelectionMenu.h"
#include "DBGameRoot.h"
#include "GameXClassIDs.h"
#include "UI/SceneClassIDs.h"
#include "SceneB2/Scene.h"
#include "InterfaceState.h"
#include "ScenarioTracker.h"
#include "System/Commands.h"
#include "Misc/StrProc.h"
#include "GetConsts.h"
#include "System/Text.h"
#include "SceneB2/Cursor.h"
#include "UI/DBUserInterface.h"
#include "3Dmotor/DBScene.h"

#include "GameX_export.h"

#include <fmt/format.h>

static bool s_bCampaignAutostartMission = false;

const wchar_t* NO_DIFFICULTY_INFO = L"******";
const int CAMPAIGN_DEFAULT_DIFFICULTY = 1;

class CTextData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTextData )
public:
	ZDATA
	std::wstring szText;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szText); return 0; }
	
	CTextData() {}
	CTextData( const std::wstring &_szText ) { szText = _szText; }
};

class CListItemTextViewer : public IDataViewer
{
	OBJECT_NOCOPY_METHODS(CListItemTextViewer)
public:
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
};

// CListItemTextViewer

void CListItemTextViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const
{
	CDynamicCast<IListControlItem> pItem = pWindow;
	NI_VERIFY( pItem, "Wrong window", return );
	
	CDynamicCast<ITextView> pView = pItem->GetSubItem( 0 );
	NI_VERIFY( pView, "Wrong window", return );
	
	if ( pData )
	{
		CDynamicCast<CTextData> pText = pData;
		NI_VERIFY( pText, "Wrong data", return );
		
		pView->SetText( pView->GetDBText() + pText->szText );
	}
	else
		pView->SetText( L"" );
}

// CInterfaceCampaignSelectionMenu

CInterfaceCampaignSelectionMenu::CInterfaceCampaignSelectionMenu() :
 CInterfaceScreenBase( "CampaignSelectionMenu2", "campaign_selection_menu" )
{
}

bool CInterfaceCampaignSelectionMenu::Init()
{
	if ( !CInterfaceScreenBase::Init() ) 
		return false;

	InterfaceState()->VerifyScenarioTracker( IInterfaceState::ESTT_NONE );
	
	AddScreen( this );
	
	MakeInterior();

	return true;
}

void CInterfaceCampaignSelectionMenu::MakeInterior()
{
	pMain = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	const NDb::SGameRoot *pGameRoot = NGameX::GetGameRoot();
	if ( !pGameRoot )
		return;

	// GameRoot determines the list size; the shipped panels are only templates.
	const int nCampaignCount = pGameRoot->campaigns.size();
	CreateCampaignWindows( nCampaignCount );

	const bool bDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	
	// get components
	// 0 = USA
	// 1 = Germany
	// 2 = USSR
	for ( int i = 0; i < campaigns.size(); ++i )
	{
		SCampaign &campaign = campaigns[i];
		if ( campaign.pWnd )
			campaign.pWnd->ShowWindow( true );

		const NDb::SCampaign *pDBCampaign = pGameRoot->campaigns[campaign.nDBCampaign];
		if ( !pDBCampaign )
		{
			if ( campaign.pBtn )
				campaign.pBtn->Enable( false );
			continue;
		}

		bool bEnableCampaign = !bDemo || NGlobal::GetVar( fmt::format("DEMO_CAMPAIGN_ENABLE_{}", i), 0 );
		if ( campaign.pBtn )
			campaign.pBtn->Enable( bEnableCampaign );

		ICampaignState *pCampaignState = InterfaceState()->GetCampaign( pDBCampaign->GetDBID() );
		if ( pCampaignState && pCampaignState->IsCompleted() )
			campaign.ePlay = PT_OUTRO;
		else 
			campaign.ePlay = PT_CAMPAIGN;

		const NDb::STexture *pPicture = (campaign.ePlay == PT_CAMPAIGN) ? pDBCampaign->pTextureNotStarted : pDBCampaign->pTextureCompleted;
		campaign.szOutro = pDBCampaign->szOutroMovie;

		if ( campaign.pPictureWnd )
			campaign.pPictureWnd->SetTexture( pPicture );

		std::wstring wszName;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBCampaign->,LocalizedName) )
			wszName = GET_TEXT_PRE(pDBCampaign->,LocalizedName);
		if ( campaign.pNameView )
			campaign.pNameView->SetText( campaign.pNameView->GetDBText() + wszName );

		std::wstring wszDesc;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBCampaign->,LocalizedDesc) )
			wszDesc = GET_TEXT_PRE(pDBCampaign->,LocalizedDesc);
		if ( campaign.pDescView )
			campaign.pDescView->SetText( campaign.pDescView->GetDBText() + wszDesc );
		if ( campaign.pDescCont )
			campaign.pDescCont->Update();
	}
	
	if ( nCampaignCount > 3 )
		CreateCampaignList();

	pPlayBtn = GetChildChecked<IButton>( pMain, "PlayBtn", true );
	pPlayOutroBtn = GetChildChecked<IButton>( pMain, "PlayOutroBtn", true );

	if ( pGameRoot )
	{
		if ( pMain )
			SetMainWindowTexture( pMain, pGameRoot->pInterfacesBackground );
	}

	pDifficulty = GetChildChecked<IComboBox>( pMain, "Difficulty", true );
	if ( pDifficulty )
		pDifficulty->SetViewer( new CListItemTextViewer() );

	ePlay = PT_CAMPAIGN;
	nSelected = -1;
	
	SelectCampaign( 0, true );
	if ( pPlayBtn )
		pPlayBtn->Enable( nSelected >= 0 );
	if ( pPlayOutroBtn && nSelected < 0 )
		pPlayOutroBtn->ShowWindow( false );
}

void CInterfaceCampaignSelectionMenu::CreateCampaignWindows( int nCampaignCount )
{
	if ( !pMain )
		return;

	// Reuse the original panel placements for the standard three-campaign menu.
	// Discover the available templates without treating their count as a limit.
	campaignWnds.clear();
	for ( int i = 1; ; ++i )
	{
		IWindow *pWnd = pMain->GetChild( fmt::format( "CampaignPanel{}", i ), true );
		if ( !pWnd )
			break;
		pWnd->ShowWindow( false );
		campaignWnds.push_back( pWnd );
	}
	NI_VERIFY( !campaignWnds.empty(), "Campaign menu: panel template is missing", return );

	campaigns.reserve( nCampaignCount );
	for ( int i = 0; i < nCampaignCount; ++i )
	{
		if ( i >= campaignWnds.size() )
		{
			// Copy the descriptor, so every added campaign gets independent controls.
			const NDb::SWindowSimple *pTemplate =
				checked_cast<const NDb::SWindowSimple*>( campaignWnds[0]->GetDesc() );
			CPtr<NDb::SWindowSimple> pPanel = pTemplate->Duplicate();
			pPanel->szName = fmt::format( "CampaignPanel{}", i + 1 );
			IWindow *pWnd = AddWindowCopy( pMain, pPanel );
			NI_VERIFY( pWnd, "Campaign menu: cannot create campaign panel", return );
			campaignWnds.push_back( pWnd );
		}
		AddCampaignWindow( i, i );
	}
}

void CInterfaceCampaignSelectionMenu::CreateCampaignList()
{
	if ( !pMain || campaigns.empty() || !campaigns[0].pDescCont )
		return;

	// Clone the existing menu widgets so this also works with packed retail UI.
	// Keep descriptors in CPtr: dropping the last CObj clears a resource even
	// while widgets still hold CDBPtr references, resetting the slider to vertical.
	// These copies must remain intact until the widgets release their references.
	const NDb::SWindowScrollableContainer *pTemplate =
		checked_cast<const NDb::SWindowScrollableContainer*>( campaigns[0].pDescCont->GetDesc() );
	CPtr<NDb::SWindowScrollableContainer> pListDesc = pTemplate->Duplicate();
	CPtr<NDb::SWindowScrollableContainerShared> pListShared =
		checked_cast_ptr<const NDb::SWindowScrollableContainerShared*>( pTemplate->pShared )->Duplicate();
	CPtr<NDb::SWindowSimple> pBorder =
		checked_cast_ptr<const NDb::SWindowSimple*>( pListShared->pBorder )->Duplicate();
	CPtr<NDb::SWindowScrollBar> pBar = pListShared->pScrollBar->Duplicate();
	CPtr<NDb::SWindowScrollBarShared> pBarShared =
		checked_cast_ptr<const NDb::SWindowScrollBarShared*>( pBar->pShared )->Duplicate();
	CPtr<NDb::SWindowSlider> pSlider = pBarShared->pSlider->Duplicate();

	// Reuse the game's horizontal track and thumb rather than vertical arrow art.
	CDBPtr<NDb::SWindowSliderShared> pHorizontal = NDb::Get<NDb::SWindowSliderShared>(
		CDBID( "UI/Game/Menu/MPCreateCustomGame/CommonSlider_WindowSliderShared.xdb" ) );
	NI_VERIFY( pHorizontal, "Campaign menu: horizontal slider template is missing", return );
	CPtr<NDb::SWindowSliderShared> pSliderShared = pHorizontal->Duplicate();
	pSliderShared->fMaxLeverSize = 0; // Size the thumb to the visible fraction of the row.

	// The options thumb uses a fixed-size texture, which only paints 40 pixels
	// even when the draggable thumb is wider. Tile its center and keep the end
	// caps intact so the visible artwork fills the thumb at every resolution.
	CPtr<NDb::SWindowMSButton> pLever = pSliderShared->pLever->Duplicate();
	CPtr<NDb::SWindowMSButtonShared> pLeverShared =
		checked_cast_ptr<const NDb::SWindowMSButtonShared*>( pLever->pShared )->Duplicate();
	const NDb::SBackground *pLeverBackground = pLeverShared->visualStates[0].normal.pBackground;
	CPtr<NDb::SBackgroundTiledTexture> pLeverTiles =
		checked_cast_ptr<const NDb::SBackgroundTiledTexture*>( pSliderShared->pBackground )->Duplicate();
	pLeverTiles->pTexture = pLeverBackground->pTexture;
	pLeverTiles->nColor = pLeverBackground->nColor;
	const float fWidth = pLeverTiles->pTexture->nWidth;
	const float fHeight = pLeverTiles->pTexture->nHeight;
	const float fCapWidth = (std::min)( 8.0f, fWidth / 3 );
	pLeverTiles->rL.ptSize = CTPoint<float>( fCapWidth, fHeight );
	pLeverTiles->rL.rcMaps = CTRect<float>( 0, 0, fCapWidth, fHeight );
	pLeverTiles->rR.ptSize = CTPoint<float>( fCapWidth, fHeight );
	pLeverTiles->rR.rcMaps = CTRect<float>( fWidth - fCapWidth, 0, fWidth, fHeight );
	pLeverTiles->rF.ptSize = CTPoint<float>( fWidth - 2 * fCapWidth, fHeight );
	pLeverTiles->rF.rcMaps = CTRect<float>( fCapWidth, 0, fWidth - fCapWidth, fHeight );
	pLeverShared->visualStates[0].normal.pBackground = pLeverTiles;
	pLever->pShared = pLeverShared;
	pSliderShared->pLever = pLever;

	pSlider->pShared = pSliderShared;
	pSlider->nSpecialPositions = 0;
	pSlider->placement.position = CVec2( 0, 0 );
	pSlider->placement.size = CVec2( 0, 0 );
	pSlider->placement.horAllign = NDb::EPA_MARGIN;
	pSlider->placement.verAllign = NDb::EPA_MARGIN;
	pSlider->placement.lowerMargin = CVec2( 0, 0 );
	pSlider->placement.upperMargin = CVec2( 0, 0 );
	pBarShared->pSlider = pSlider;
	pBarShared->pButtonLower = 0;
	pBarShared->pButtonGreater = 0;
	pBar->pShared = pBarShared;
	pBar->placement = pSlider->placement;
	pBar->placement.verAllign = NDb::EPA_HIGH_END;
	pBar->placement.size = CVec2( 0, 18 );

	// Reserve the bottom 20 pixels for the scrollbar, outside the clipped panels.
	pBorder->placement = pSlider->placement;
	pBorder->placement.upperMargin = CVec2( 0, 20 );
	pListShared->children.clear();
	pListShared->pScrollBar = pBar;
	pListShared->pBorder = pBorder;
	pListShared->pSelection = 0;
	pListShared->pPreSelection = 0;
	pListShared->pNegativeSelection = 0;
	pListShared->nInterval = 9;
	pListDesc->pShared = pListShared;
	pListDesc->szName = "CampaignList";
	pListDesc->placement = pSlider->placement;
	pListDesc->placement.verAllign = NDb::ERA_CENTER;
	pListDesc->placement.lowerMargin = CVec2( 11, 0 );
	pListDesc->placement.upperMargin = CVec2( 11, 0 );
	int nPanelHeight = 0;
	campaigns[0].pWnd->GetPlacement( 0, 0, 0, &nPanelHeight );
	pListDesc->placement.size = CVec2( 0, nPanelHeight + 20 );

	pCampaignList = dynamic_cast<IScrollableContainer*>( AddWindowCopy( pMain, pListDesc ) );
	NI_VERIFY( pCampaignList, "Campaign menu: cannot create scroll container", return );
	for ( SCampaign &campaign : campaigns )
	{
		if ( campaign.pWnd )
			pCampaignList->PushBack( campaign.pWnd, false );
	}
	pCampaignList->Update();
}

void CInterfaceCampaignSelectionMenu::AddCampaignWindow( int nWndIndex, int nCampaignIndex )
{
	SCampaign campaign;
	campaign.ePlay = PT_CAMPAIGN;
	campaign.pWnd = campaignWnds[nWndIndex];
	campaign.pBtn = GetChildChecked<IButton>( campaign.pWnd, "SelectCampaignBtn", true );
	campaign.pPictureWnd = GetChildChecked<IWindow>( campaign.pWnd, "Flag", true );
	campaign.pNameView = GetChildChecked<ITextView>( campaign.pWnd, "CampaignNameView", true );
	campaign.pDescCont = GetChildChecked<IScrollableContainer>( campaign.pWnd, "DescCont", true );
	campaign.pDescView = GetChildChecked<ITextView>( campaign.pDescCont, "DescView", true );
	campaign.pBackgroundWnd = GetChildChecked<IWindow>( campaign.pWnd, "Background", true );
	if ( campaign.pDescCont && campaign.pDescView )
		campaign.pDescCont->PushBack( campaign.pDescView, false );
	campaign.szBtnName = fmt::format( "SelectCampaignBtn{}", nWndIndex );
	if ( campaign.pBtn )
		campaign.pBtn->SetName( campaign.szBtnName );
	campaign.nDBCampaign = nCampaignIndex;

	campaigns.push_back( campaign );
}

bool CInterfaceCampaignSelectionMenu::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "menu_select_campaign" )
		return OnSelectCampaign( szSender );
	if ( szReaction == "menu_back" )
		return OnBack();
	if ( szReaction == "menu_play" )
		return OnPlay();
	if ( szReaction == "menu_play_outro" )
		return OnPlayOutro();

	return false;
}

int CInterfaceCampaignSelectionMenu::Check( const std::string &szCheckName ) const
{
	return 0;
}

void CInterfaceCampaignSelectionMenu::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );

	PauseIntermission( !bFocus );

	if ( bFocus )
	{
		Cursor()->Show( true );
		Cursor()->SetMode( NDb::USER_ACTION_UNKNOWN );
	}
}

bool CInterfaceCampaignSelectionMenu::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );
	
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return bResult;
}

bool CInterfaceCampaignSelectionMenu::IsModal()
{
	return false;
}

bool CInterfaceCampaignSelectionMenu::OnSelectCampaign( const std::string &szSender )
{
	for ( int i = 0; i < campaigns.size(); ++i )
	{
		SCampaign &campaign = campaigns[i];
		if ( campaign.szBtnName == szSender )
		{
			SelectCampaign( i, false );
			break;
		}
	}

	return true;
}

bool CInterfaceCampaignSelectionMenu::OnBack()
{
	InterfaceState()->VerifyScenarioTracker( IInterfaceState::ESTT_NONE );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "single_player_submenu" );

	return true;
}

bool CInterfaceCampaignSelectionMenu::OnPlay()
{
	// An empty campaign list has no valid selection to launch.
	if ( nSelected < 0 || nSelected >= campaigns.size() )
		return true;

	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bKRIDemo )
	{
		const int nDifficulty = pDifficulty ? pDifficulty->GetSelectedIndex() : 0;

		// 0 = USA
		// 1 = Germany
		// 2 = USSR
		const std::string szMissionGlobalVarName = fmt::format( "DEMO_MISSION_{}_{}", nSelected, nDifficulty );
		std::wstring wszCommand = NGlobal::GetVar( szMissionGlobalVarName );
		std::string szCommand = NStr::ToMBCS( wszCommand );
		NStr::TrimBoth( szCommand, '\"' );
		wszCommand = NStr::ToUnicode( szCommand );

		NGlobal::ProcessCommand( wszCommand );
		
		return true;
	}
	//
#if !defined(_SINGLE_DEMO) && !defined(_MP_DEMO)
	const NDb::SGameRoot *pGameRoot = InterfaceState()->GetGameRoot();
	int nDifficulty = pDifficulty ? pDifficulty->GetSelectedIndex() : 0;
	const NDb::SCampaign *pCampaign = pGameRoot->campaigns[nSelected];
	NI_VERIFY( pCampaign, "Designers: no campaign", return true );
	
	StartCampaign( pCampaign, nDifficulty, false );
#endif // !defined(_SINGLE_DEMO) && !defined(_MP_DEMO)

	return true;
}

bool CInterfaceCampaignSelectionMenu::OnPlayOutro()
{
	if ( nSelected >= 0 )
	{
		SCampaign &campaign = campaigns[nSelected];

		const auto command = fmt::format( "{};campaign_selection_nothing", campaign.szOutro );
		NMainLoop::Command( ML_COMMAND_PLAY_MOVIE, command.c_str() );
	}

	return true;
}

void CInterfaceCampaignSelectionMenu::SelectCampaign( int _nIndex, bool bFirstTime )
{
	int nIndex = -1;
	for ( int i = campaigns.size() - 1; i >= 0; --i )
	{
		SCampaign &campaign = campaigns[i];
		if ( !campaign.pBtn || !campaign.pBtn->IsEnabled() )
			continue;
		nIndex = i;
		if ( nIndex == _nIndex )
			break;
	}
	if ( nIndex < 0 )
		return;

	const NDb::SGameRoot *pGameRoot = NGameX::GetGameRoot();
	NI_VERIFY( 0 <= nIndex && nIndex < pGameRoot->campaigns.size(), "index out of range", return );
	
	const NDb::SCampaign *pCampaign = pGameRoot->campaigns[nIndex];
	NI_VERIFY( pCampaign, "No campaign info", return );
	
	if ( pDifficulty )
	{
		const int nOldDifficulty = bFirstTime ? CAMPAIGN_DEFAULT_DIFFICULTY : pDifficulty->GetSelectedIndex();
		const int nNewDifficulty = (nOldDifficulty >= 0 && nOldDifficulty < pCampaign->difficultyLevels.size() ) ? nOldDifficulty : 
			(std::max)( 0, (std::min)( CAMPAIGN_DEFAULT_DIFFICULTY, (int)( pCampaign->difficultyLevels.size() ) - 1 ) );

		pDifficulty->RemoveAllItems();
		if ( pCampaign->difficultyLevels.empty() )
		{
			pDifficulty->ShowWindow( false );
		}
		else
		{
			pDifficulty->ShowWindow( true );

			// CRAP - специальная затычка для дизайнеров, которые не используют константы в качестве уровня сложности
			// ставим "very easy", идущий последним, в начало
			if ( pCampaign->difficultyLevels.size() == 4 )
			{
				const NDb::SDifficultyLevel *pDifficultyLevel = pCampaign->difficultyLevels[3];
				AddDifficultyLevel( pDifficultyLevel );

				for ( int i = 0; i < 3; ++i )
				{
					const NDb::SDifficultyLevel *pDifficultyLevel = pCampaign->difficultyLevels[i];
					AddDifficultyLevel( pDifficultyLevel );
				}
			}
			else
			{
				for ( int i = 0; i < pCampaign->difficultyLevels.size(); ++i )
				{
					const NDb::SDifficultyLevel *pDifficultyLevel = pCampaign->difficultyLevels[i];
					AddDifficultyLevel( pDifficultyLevel );
				}
			}
			
			pDifficulty->Select( nNewDifficulty );
		}
	}

	nSelected = nIndex;
	if ( pCampaignList )
	{
		// Reveal a clipped selection without moving panels that are already visible.
		const CTRect<float> panelRect = campaigns[nSelected].pWnd->GetWindowRect();
		const CTRect<float> listRect = pCampaignList->GetWindowRect();
		if ( panelRect.left < listRect.left || panelRect.right > listRect.right )
			pCampaignList->EnsureElementVisible( campaigns[nSelected].pWnd );
	}
	
	ePlay = PT_CAMPAIGN;
	for ( int i = 0; i < campaigns.size(); ++i )
	{
		SCampaign &campaign = campaigns[i];
		IButton *pButton = campaign.pBtn;
		bool bSelected = (i == nSelected);
		if ( bSelected )
			ePlay = campaign.ePlay;
		if ( campaign.pBtn )
			campaign.pBtn->ShowWindow( !bSelected );
	}
	if ( pPlayOutroBtn )
		pPlayOutroBtn->ShowWindow( ePlay == PT_OUTRO );
}

void CInterfaceCampaignSelectionMenu::AddDifficultyLevel( const NDb::SDifficultyLevel *pDifficultyLevel )
{
	std::wstring wszText;
	if ( pDifficultyLevel && CHECK_TEXT_NOT_EMPTY_PRE(pDifficultyLevel->,LocalizedName) )
		wszText = GET_TEXT_PRE(pDifficultyLevel->,LocalizedName);
	else
		wszText = NO_DIFFICULTY_INFO;
	if ( pDifficulty )
		pDifficulty->AddItem( new CTextData( wszText ) );
}

#ifndef _MP_DEMO
// CICCampaignSelectionMenu

void CICCampaignSelectionMenu::PreCreate()
{
}

void CICCampaignSelectionMenu::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICCampaignSelectionMenu::Configure( const char *pszConfig )
{
}


void CampaignSelectionNothing( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	// do nothing
}

void StartCampaign( const NDb::SCampaign *pCampaignDB, int _nDifficulty, bool bCustom )
{
	int nDifficulty = _nDifficulty;
	// CRAP - возвращаем обратно обмененные местами уровни сложности
	if ( pCampaignDB->difficultyLevels.size() == 4 )
	{
		if ( nDifficulty == 0 )
			nDifficulty = 3;
		else
			nDifficulty--;
	}
	
	InterfaceState()->SetFirstTimeInChapter( true );

	InterfaceState()->VerifyScenarioTracker( IInterfaceState::ESTT_NONE );
	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_SINGLE );

	Singleton<IScenarioTracker>()->CampaignStart( pCampaignDB, nDifficulty, false, bCustom );
	
//	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
//	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" ); // уберем подложку с миссией

	std::string szIntro = pCampaignDB->szIntroMovie;
	if ( szIntro.empty() )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		if ( s_bCampaignAutostartMission )
		{
			std::wstring wszCommand = NStr::ToUnicode( "chapter_map_autostart_mission" );
			NGlobal::ProcessCommand( wszCommand );
		}
		else
			NMainLoop::Command( ML_COMMAND_CHAPTER_MAP_MENU, "" );
	}
	else
	{
		if ( s_bCampaignAutostartMission )
			szIntro += ";back_and_chapter_map_autostart_mission";
		else
			szIntro += ";back_and_chapter_map";
		NMainLoop::Command( ML_COMMAND_PLAY_MOVIE, szIntro.c_str() );
	}
}

void BackAndChapterMapAutostartMission( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NGlobal::ProcessCommand( L"chapter_map_autostart_mission" );
}

void BackAndChapterMap( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NGlobal::ProcessCommand( L"chapter_map" );
}

void DemoCampaignSelectionMenu( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	const bool bContinue = NGlobal::GetVar( "DEMO_MODE_CONTINUE_MOVIE", 0 ) != 0;
	if ( bContinue )
	{
		const std::string szParam = "Movies\\demo_outro.xml;demo_campaign_selection_menu";

		NMainLoop::Command( ML_COMMAND_CLEAR_INTERFACES, "" );
		NMainLoop::Command( ML_COMMAND_PLAY_MOVIE, szParam.c_str() );
	}
	else
	{
		NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
		NMainLoop::Command( ML_COMMAND_CAMPAIGN_SELECTION_MENU, "" );
	}
}

START_REGISTER(CampaignCommands)

REGISTER_VAR_EX( "campaign_autostart_mission", NGlobal::VarBoolHandler, &s_bCampaignAutostartMission, false, STORAGE_NONE );
REGISTER_CMD( "campaign_selection_nothing", CampaignSelectionNothing );
REGISTER_CMD( "back_and_chapter_map_autostart_mission", BackAndChapterMapAutostartMission );
REGISTER_CMD( "back_and_chapter_map", BackAndChapterMap );
REGISTER_CMD( "demo_campaign_selection_menu", DemoCampaignSelectionMenu );

FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( GAMEX, 0x170C0B41, CInterfaceCampaignSelectionMenu )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_CAMPAIGN_SELECTION_MENU, CICCampaignSelectionMenu )

#endif // _MP_DEMO
