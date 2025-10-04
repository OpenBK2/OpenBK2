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

static bool s_bCampaignAutostartMission = false;

const int MAX_CAMPAIGN_COUNT = 3;
const int CAMPAIGN_WINDOW_COUNT = 5;
const int CAMPAIGN_WINDOW_1_1 = 0;//1;
const int CAMPAIGN_WINDOW_2_1 = 0;//3;
const int CAMPAIGN_WINDOW_2_2 = 1;//4;
const int CAMPAIGN_WINDOW_3_1 = 0;
const int CAMPAIGN_WINDOW_3_2 = 1;
const int CAMPAIGN_WINDOW_3_3 = 2;

const wchar_t* NO_DIFFICULTY_INFO = L"******";
const int CAMPAIGN_DEFAULT_DIFFICULTY = 1;

class CTextData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTextData )
public:
	ZDATA
	wstring szText;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szText); return 0; }
	
	CTextData() {}
	CTextData( const wstring &_szText ) { szText = _szText; }
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

	campaignWnds.resize( CAMPAIGN_WINDOW_COUNT );
	for ( int i = 0; i < campaignWnds.size(); ++i )
	{
		IWindow *pWnd = GetChildChecked<IWindow>( pMain, StrFmt( "CampaignPanel%d", i + 1 ), true );
		if ( pWnd )
			pWnd->ShowWindow( false );
		campaignWnds[i] = pWnd;
	}

	const NDb::SGameRoot *pGameRoot = NGameX::GetGameRoot();
	if ( !pGameRoot )
		return;

	int nCampaignCount = Min( MAX_CAMPAIGN_COUNT, pGameRoot->campaigns.size() );
	switch ( nCampaignCount )
	{
		case 1:
		{
			AddCampaignWindow( CAMPAIGN_WINDOW_1_1, 0 );
			break;
		}

		case 2:
		{
			AddCampaignWindow( CAMPAIGN_WINDOW_2_1, 0 );
			AddCampaignWindow( CAMPAIGN_WINDOW_2_2, 1 );
			break;
		}

		case 3:
		{
			AddCampaignWindow( CAMPAIGN_WINDOW_3_1, 0 );
			AddCampaignWindow( CAMPAIGN_WINDOW_3_2, 1 );
			AddCampaignWindow( CAMPAIGN_WINDOW_3_3, 2 );
			break;
		}
	}

#ifndef _FINALRELEASE
	// additional test campaigns
	bool bAllowTestCampaigns = NGlobal::GetVar( "allow_test_campaigns", 0 ) != 0;
	if ( bAllowTestCampaigns )
		nCampaignCount = Min( 5, pGameRoot->campaigns.size() );
	if ( nCampaignCount >= 4 )
		AddCampaignWindow( 3, 3 );
	if ( nCampaignCount >= 5 )
		AddCampaignWindow( 4, 4 );
#endif //_FINALRELEASE

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

		bool bEnableCampaign = !bDemo || NGlobal::GetVar( StrFmt("DEMO_CAMPAIGN_ENABLE_%d", i), 0 );
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

		wstring wszName;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBCampaign->,LocalizedName) )
			wszName = GET_TEXT_PRE(pDBCampaign->,LocalizedName);
		if ( campaign.pNameView )
			campaign.pNameView->SetText( campaign.pNameView->GetDBText() + wszName );

		wstring wszDesc;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBCampaign->,LocalizedDesc) )
			wszDesc = GET_TEXT_PRE(pDBCampaign->,LocalizedDesc);
		if ( campaign.pDescView )
			campaign.pDescView->SetText( campaign.pDescView->GetDBText() + wszDesc );
		if ( campaign.pDescCont )
			campaign.pDescCont->Update();
	}
	
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
}

void CInterfaceCampaignSelectionMenu::AddCampaignWindow( int nWndIndex, int nCampaignIndex )
{
	SCampaign campaign;
	campaign.pWnd = campaignWnds[nWndIndex];
	campaign.pBtn = GetChildChecked<IButton>( campaign.pWnd, "SelectCampaignBtn", true );
	campaign.pPictureWnd = GetChildChecked<IWindow>( campaign.pWnd, "Flag", true );
	campaign.pNameView = GetChildChecked<ITextView>( campaign.pWnd, "CampaignNameView", true );
	campaign.pDescCont = GetChildChecked<IScrollableContainer>( campaign.pWnd, "DescCont", true );
	campaign.pDescView = GetChildChecked<ITextView>( campaign.pDescCont, "DescView", true );
	campaign.pBackgroundWnd = GetChildChecked<IWindow>( campaign.pWnd, "Background", true );
	if ( campaign.pDescCont && campaign.pDescView )
		campaign.pDescCont->PushBack( campaign.pDescView, false );
	campaign.szBtnName = StrFmt( "SelectCampaignBtn%d", nWndIndex );
	if ( campaign.pBtn )
		campaign.pBtn->SetName( campaign.szBtnName );
	campaign.nDBCampaign = nCampaignIndex;

	campaigns.push_back( campaign );
}

bool CInterfaceCampaignSelectionMenu::Execute( const string &szSender, const string &szReaction )
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

int CInterfaceCampaignSelectionMenu::Check( const string &szCheckName ) const
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

bool CInterfaceCampaignSelectionMenu::OnSelectCampaign( const string &szSender )
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
	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bKRIDemo )
	{
		const int nDifficulty = pDifficulty ? pDifficulty->GetSelectedIndex() : 0;

		// 0 = USA
		// 1 = Germany
		// 2 = USSR
		const string szMissionGlobalVarName = StrFmt( "DEMO_MISSION_%d_%d", nSelected, nDifficulty );
		wstring wszCommand = NGlobal::GetVar( szMissionGlobalVarName );
		string szCommand = NStr::ToMBCS( wszCommand );
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

		NMainLoop::Command( ML_COMMAND_PLAY_MOVIE, StrFmt( "%s;campaign_selection_nothing", campaign.szOutro.c_str() ) );
	}

	return true;
}

void CInterfaceCampaignSelectionMenu::SelectCampaign( int _nIndex, bool bFirstTime )
{
	int nIndex = -1;
	for ( int i = campaigns.size() - 1; i >= 0; --i )
	{
		SCampaign &campaign = campaigns[i];
		if ( !campaign.pBtn->IsEnabled() )
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
			Max( 0, Min( CAMPAIGN_DEFAULT_DIFFICULTY, (int)( pCampaign->difficultyLevels.size() ) - 1 ) );

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
	wstring wszText;
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


void CampaignSelectionNothing( const string &szID, const vector<wstring> &paramsSet, void *pContext )
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

	string szIntro = pCampaignDB->szIntroMovie;
	if ( szIntro.empty() )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		if ( s_bCampaignAutostartMission )
		{
			wstring wszCommand = NStr::ToUnicode( "chapter_map_autostart_mission" );
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

void BackAndChapterMapAutostartMission( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NGlobal::ProcessCommand( L"chapter_map_autostart_mission" );
}

void BackAndChapterMap( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NGlobal::ProcessCommand( L"chapter_map" );
}

void DemoCampaignSelectionMenu( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	const bool bContinue = NGlobal::GetVar( "DEMO_MODE_CONTINUE_MOVIE", 0 ) != 0;
	if ( bContinue )
	{
		const string szParam = "Movies\\demo_outro.xml;demo_campaign_selection_menu";

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

REGISTER_SAVELOAD_CLASS( 0x170C0B41, CInterfaceCampaignSelectionMenu )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_CAMPAIGN_SELECTION_MENU, CICCampaignSelectionMenu )

#endif // _MP_DEMO
