#include "StdAfx.h"
#include "InterfaceChapterMapMenu.h"
#include "GameXClassIDs.h"
#include "Misc/STrProc.h"
#include "ScenarioTracker.h"
#include "SceneB2/Cursor.h"
#include "Sound/MusicSystem.h"
#include "SaveLoadHelper.h"
#include "Misc/nalgoritm.h"
#include "3DMotor/FrameTransition.h"
#include "System/Commands.h"
#include "InterfaceState.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "SceneB2/Scene.h"
#include "System/FileUtils.h"
#include "DBWrapReinf.h"
#include "InterfaceMisc.h"
#include "UIElementsHelper.h"
#include "InterfaceChapterMapMenuDialogs.h"
#include "System/Text.h"
#include "UI/SceneClassIDs.h"
#include "3DMotor/ScreenShot.h"

static int s_nTransitionEffectToPWLDuration = 700;
static int s_nFadeEffectDuration = 400;
static int s_nExpandEffectDuration = 400;
static int s_nWaitEffectDuration = 200;
static float s_fTransitionEffectToPWLLength = 0.0f; //0.1f;
static int s_nFrontLineAnimDuration = 5000;

const int TOP_PRIORITY = 500;
const int PICTURE_BORDER_SIZE = 40; // (23 pixels for original size)

const int SELECTION_OVER_DELTA_X = 0;
const int SELECTION_OVER_DELTA_Y = 0;
const int SELECTION_PUSHED_DELTA_X = 1;
const int SELECTION_PUSHED_DELTA_Y = 2;
const int SELECTION_SELECTED_DELTA_X = 1;
const int SELECTION_SELECTED_DELTA_Y = 2;

const float ROLLER_TIME = 2.0f;

const wchar_t* DYNAMIC_TAG_REINF_TYPE = L"reinf_type";

bool CInterfaceChapterMapMenu::CReactions::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "reaction_on_target_select" )
	{
		pInterface->OnTargetSelect( szSender );
		return true;
	}
	if ( szReaction == "reaction_on_target_dbl_click" )
		return pInterface->OnTargetDblClick( szSender );

	if ( szReaction == "reaction_on_save" )
		return pInterface->OnSaveGame();

	if ( szReaction == "reaction_reinf_mouse_over" )
		return pInterface->OnMouseOverReinf( szSender, true );
	if ( szReaction == "reaction_reinf_mouse_over_left" )
		return pInterface->OnMouseOverReinf( szSender, false );

	if ( szReaction == "effect_finished" )
	{
		pInterface->OnEffectFinish();
		return true;
	}

	if ( szReaction == "mission_bonus_clicked" )
		return pInterface->OnFixBonus( szSender );

	if ( szReaction == "menu_army_manager" )
		return pInterface->OnArmyManager();
	if ( szReaction == "menu_player_stats" )
		return pInterface->OnPlayerStats();

	// Reinf Desc window {
	if ( szReaction == "reaction_popup_item_clicked" )
	{
		pInterface->OnPopupClicked( szSender );
		return true;
	}
	if ( szReaction == "reaction_reinf_desc_item_clicked" )
	{
		pInterface->OnReinfDescItem( szSender );
		return true;
	}
	if ( szReaction == "reaction_reinf_desc_ok" )
	{
		pInterface->OnReinfDescOK();
		return true;
	}
	if ( szReaction == "reaction_reinf_desc_encyclopedia" )
	{
		pInterface->OnReinfDescEncyclopedia();
		return true;
	}
	if ( szReaction == "reaction_reinf_scroll_left" )
	{
		pInterface->ReinfDescScrollLeft();
		return true;
	}
	if ( szReaction == "reaction_reinf_scroll_right" )
	{
		pInterface->ReinfDescScrollRight();
		return true;
	}
	// } Reinf Desc window

	if ( szReaction == "target_over" )
		return true;
	if ( szReaction == "target_over_back" )
		return true;
	if ( szReaction == "target_pushed" )
		return pInterface->OnTargetPushed( szSender );
	if ( szReaction == "target_pushed_back" )
		return pInterface->OnTargetPushedBack( szSender );
		
	if ( szReaction == "reinf_upgrade_dialog_close" )
		return pInterface->OnReinfUpgradeDialogClose( szSender );
	if ( szReaction == "reinf_upgrade_unit_btn" )
		return pInterface->OnReinfUpgradeUnitBtn( szSender );

	if ( szReaction == "chapter_desc_close_dialog" )
		return pInterface->OnChapterDescDlgClose( szSender );
	if ( szReaction == "mission_desc_close_dialog" )
		return pInterface->OnMissionDescDlgClose( szSender );
	if ( szReaction == "show_mission_desc" )
		return pInterface->OnShowMissionDesc( szSender );

	return false;
}

// CInterfaceChapterMapMenu

CInterfaceChapterMapMenu::CInterfaceChapterMapMenu() : 
	CInterfaceScreenBase( "ChapterMapMenu", "chapter_map_menu" ),
	eUIState( EUIS_NORMAL ), eReinfDescState( ERDWS_NONE ), nDelay( 0 ), pChapter( 0 ),
	eExitDir( EED_ENTER ), nSelectedMission( -1 ), nRecommendedTarget( 0 ),
	bInitialDialogVisible( false )
{
	AddObserver( "message_box_ok", &CInterfaceChapterMapMenu::MsgMessageBoxOk );

	bonusButtons.resize( 4 );
}

CInterfaceChapterMapMenu::~CInterfaceChapterMapMenu()
{
	if ( pScreen ) 
		Scene()->RemoveScreen( pScreen );
	pReactions = 0;
}

int CInterfaceChapterMapMenu::CReactions::Check( const string &szCheckName ) const
{
	return 0;
}

bool CInterfaceChapterMapMenu::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	const NDb::SUIConstsB2 *pUIC = InterfaceState()->GetUIConsts();
	// load screen
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	pReactions = new CReactions( pScreen, this );
	AddUIScreen( pScreen, "ChapterMapMenu", pReactions );
	if ( pUIC->contexts.size() > 1 ) 
		pScreen->SetTooltipContext( 1 );

	pMain = GetChildChecked<IWindow>( pScreen, "ChapterMapMain", true );
	NI_VERIFY( pMain, "Main Window not found", return false );

	RegisterObservers();

	InitLoadControls();

	eReinfDescState = ERDWS_NONE;
	bool bMovieMusic = false;
	if ( !Singleton<IScenarioTracker>()->IsChapterActive() )
	{
		// Store old reinfs
		oldReinfs.resize( NDb::_RT_NONE );
		for ( int i = 0; i < oldReinfs.size(); ++i )
			oldReinfs[i] = Singleton<IScenarioTracker>()->GetReinforcement( 0, NDb::EReinforcementType( i ) );

		Singleton<IScenarioTracker>()->NextChapter();

		if ( Singleton<IScenarioTracker>()->IsChapterActive() )
		{
			const NDb::SChapter *pChapter = Singleton<IScenarioTracker>()->GetCurrentChapter();
			if ( pChapter )
			{
				string szParam = pChapter->szIntroMovie;
				if ( !szParam.empty() )
				{
					szParam += ";chapter_map_after_intro_movie";
					NMainLoop::Command( ML_COMMAND_PLAY_MOVIE, szParam.c_str() );
					bMovieMusic = true;
				}
			}
		}
	}

	InitReinforcements();

	if ( !Singleton<IScenarioTracker>()->IsCampaignActive() )
	{
		// игра окончена
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );

		const NDb::SCampaign *pCampaign = Singleton<IScenarioTracker>()->GetCurrentCampaign();
		NI_ASSERT( pCampaign, "Wrong campaign info" );

		if ( !pCampaign )
		{
			if ( !NMainLoop::GetPrevInterface( this ) )
			{
				InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

//				NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
				NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
			}
			return false;
		}

		ICampaignState *pCampaignState = InterfaceState()->GetCampaign( pCampaign->GetDBID() );
		if ( pCampaignState )
			pCampaignState->SetCompleted( true );

		InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

		string szParam = pCampaign->szOutroMovie;
		szParam += ";chapter_map_outro";
		NMainLoop::Command( ML_COMMAND_PLAY_MOVIE, szParam.c_str() );

		return true;
	}

	pChapter = Singleton<IScenarioTracker>()->GetCurrentChapter();
	NI_VERIFY( pChapter, "Wrong chapter info", return false );

	if ( CHECK_TEXT_NOT_EMPTY_PRE(pChapter->,LocalizedName) )
	{
		ITextView *pCName = GetChildChecked<ITextView>( pMain, "ChapterName", true );
		pCName->SetText( pCName->GetDBText() + GET_TEXT_PRE(pChapter->,LocalizedName) );
	}

	/*if ( CHECK_TEXT_NOT_EMPTY_PRE(pChapter->,LocalizedDate) )
	{
		ITextView *pCDate = GetChildChecked<ITextView>( pMain, "ChapterDate", true );
		pCDate->SetText( pCDate->GetDBText() + GET_TEXT_PRE(pChapter->,LocalizedDate) );
	}*/

	wszReinfNotEnabledPrefix = InterfaceState()->GetTextEntry( "T_CHAPTER_MAP_PREFIX_NOT_ENABLED" );
//	wszReinfDisabledPrefix = InterfaceState()->GetTextEntry( "T_CHAPTER_MAP_PREFIX_DISABLED" );
	wszReinfAvailablePrefix = InterfaceState()->GetTextEntry( "T_CHAPTER_MAP_PREFIX_AVAILABLE" );

	nCallsLeft = Singleton<IScenarioTracker>()->GetReinforcementCallsLeftInChapter();

	if ( pChapterMap )
		pChapterMap->SetTexture( pChapter->pMapPicture );

	nFrontLineAnim = -1;
	vDetailsCoeff = CVec2( 1, 1 );
	pDetailsMap = pChapter->pDetailsMap;
	if ( pDetailsMap )
	{
		int nX, nY;
		pChapterMap->GetPlacement( 0, 0, &nX, &nY );

		vDetailsCoeff.x = float( nX ) / ( pDetailsMap->nNumPatchesX * AI_TILE_SIZE * AI_TILES_IN_PATCH );
		vDetailsCoeff.y = float( nY ) / ( pDetailsMap->nNumPatchesY * AI_TILE_SIZE * AI_TILES_IN_PATCH );
	}

	Cursor()->Show( true );
	Cursor()->SetMode( NDb::USER_ACTION_UNKNOWN );

	bRequestAutoSave = Singleton<IScenarioTracker>()->IsMissionWon();

	InitMissions();

	EffectStart( EED_ENTER );
	bNeedToRunAnimation = false;

	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;
	if ( bKRIDemo )
	{
		IWindow *pChapterMapRightWnd = GetChildChecked<IWindow>( GetScreen(), "ChapterMapRight", true );
		IButton *pSaveBtn = GetChildChecked<IButton>( pChapterMapRightWnd, "SaveButton", true );
		IButton *pArmyManagerBtn = GetChildChecked<IButton>( pChapterMapRightWnd, "ArmyManagerButton", true );

		if ( pSaveBtn )
			pSaveBtn->Enable( false );
		if ( pArmyManagerBtn )
			pArmyManagerBtn->Enable( false );
	}

	// music (after all lengthy operations to allow MainMenu music to play while loading)
	if ( !bMovieMusic )
	{
		const NDb::SCampaign *pCampaign = Singleton<IScenarioTracker>()->GetCurrentCampaign();
		if ( pCampaign->pIntermissionMusic )
			Singleton<IMusicSystem>()->Init( pCampaign->pIntermissionMusic, 0 );
	}

	if ( pMain )
		SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );

	return true;
}

void CInterfaceChapterMapMenu::RegisterObservers()
{
	AddObserver( "menu_back", &CInterfaceChapterMapMenu::MsgBack );
	AddObserver( "menu_play", &CInterfaceChapterMapMenu::MsgPlay );
	AddObserver( "menu_return_from_subscreen", &CInterfaceChapterMapMenu::MsgReenter );
	AddObserver( "menu_continue_play", &CInterfaceChapterMapMenu::MsgContinuePlay );
	AddObserver( "complete_selected_mission", &CInterfaceChapterMapMenu::MsgCompleteSelectedMission );
	AddObserver( "complete_chapter", &CInterfaceChapterMapMenu::MsgCompleteChapter );
}

void CInterfaceChapterMapMenu::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );

	if ( bFocus )
	{
		if ( pMain )
			pMain->ShowWindow( true );

		Cursor()->Show( true );
	}
}

void CInterfaceChapterMapMenu::MsgReenter( const SGameMessage &msg )
{
//	eExitDir = EED_RE_ENTER;
//	bNeedToRunAnimation = true;
}

void CInterfaceChapterMapMenu::MsgCompleteSelectedMission( const SGameMessage &msg )
{
  IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	if ( pScenarioTracker && pScenarioTracker->IsMissionActive() )
	{
		pScenarioTracker->MissionWin();
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_CHAPTER_MAP_MENU, "" );
	}
}

void CInterfaceChapterMapMenu::MsgCompleteChapter( const SGameMessage &msg )
{
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	if ( pScenarioTracker && pScenarioTracker->IsChapterActive() )
	{
		pScenarioTracker->NextChapter();
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		NMainLoop::Command( ML_COMMAND_CHAPTER_MAP_MENU, "" );
	}
}

bool CInterfaceChapterMapMenu::OnSaveGame()
{
	InterfaceState()->GetScreenShotTexture()->Generate( true );
	EffectStart( EED_SAVE );

	return true;
}

void CInterfaceChapterMapMenu::MsgBack( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel", 
		InterfaceState()->GetTextEntry( "T_CHAPTER_MAP_QUESTION_BACK" ) ).c_str() );
}

void CInterfaceChapterMapMenu::MsgMessageBoxOk( const SGameMessage &msg )
{
	EffectStart( EED_BACK );
}

void CInterfaceChapterMapMenu::MsgMessageBoxCancel( const SGameMessage &msg )
{
}

void CInterfaceChapterMapMenu::MsgPlay( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_MISSION_BRIEFING, "" );
/*	const NDb::SMapInfo *pMapInfo = Singleton<IScenarioTracker>()->GetCurrentMission();
	NI_VERIFY( pMapInfo, "Wrong mission info", return );
	pMapToStart = pMapInfo;

	eUIState = EUIS_PLAY_MISSION_PRESSED;

	GetScreen()->Enable( false );

	NInput::PostEvent( "unload_background_mission", 0, 0 );*/
}

void CInterfaceChapterMapMenu::MsgContinuePlay( const SGameMessage &msg )
{
	Singleton<IScene>()->AddScreen( pScreen );

	PlayMissionStartEffect();
}

bool CInterfaceChapterMapMenu::OnArmyManager()
{
//	EffectStart( EED_ARMY_MANAGER );
	NMainLoop::Command( ML_COMMAND_ARMY_SCREEN, "" );
	return true;
}

bool CInterfaceChapterMapMenu::OnPlayerStats()
{
//	EffectStart( EED_PLAYER_INFO );
	NMainLoop::Command( ML_COMMAND_PLAYER_STATS, "" );
	return true;
}

void CInterfaceChapterMapMenu::PlayMissionStartEffect()
{
	GetScreen()->Enable( false );
	eUIState = EUIS_PLAY_MISSION_START_EFFECT;
	timeStartEffect = Singleton<IGameTimer>()->GetAbsTime();
	eUIEffectState = EUIES_FADE;
}

void CInterfaceChapterMapMenu::PlayMissionStartMission()
{
	eUIState = EUIS_PLAY_MISSION_DONE;

	InterfaceState()->SetTransitEffectFlag( true );

	/*	NGScene::SFrameTransitionInfo ftInfo;
	ftInfo.bRandomDir = false;
	ftInfo.vTransitionDir.Set( 1.0f, 0.0f );
	ftInfo.fTransitionLength = s_fTransitionEffectToPWLLength;
	ftInfo.nEffectDuration = s_nTransitionEffectToPWLDuration;
	ftInfo.fQuadsGroup1MinZ = 1.0f;
	ftInfo.fQuadsGroup1MaxZ = 1.1f;
	ftInfo.fQuadsGroup2MinZ = 1.0f;
	ftInfo.fQuadsGroup2MaxZ = 0.9f;
	CInterfaceScreenBase::Draw( NGScene::GetFrameTransitionCapture1( ftInfo ) );*/

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MISSION, StrFmt( "%s;normal", pMapToStart->GetDBID().ToString().c_str() ) );
}

void CInterfaceChapterMapMenu::FadeMapElements( float fFade )
{
	for ( CTargets::iterator it = targets.begin(); it != targets.end(); ++it )
	{
		STarget &target = *it;

		if ( target.pWindow )
			target.pWindow->SetFadeValue( fFade );
		for ( vector< SReward >::iterator iReward = target.rewards.begin(); 
			iReward != target.rewards.end(); ++iReward )
		{
			SReward &reward = *iReward;

			if ( reward.pBtn )
				reward.pBtn->SetFadeValue( fFade );
			if ( reward.pBonusWnd )
				reward.pBonusWnd->SetFadeValue( fFade );
		}
	}
}

void CInterfaceChapterMapMenu::ExpandMap( float fProgress )
{
	CTRect<float> rcParent = GetScreen()->GetWindowRect();
	if ( pMapPanel )
	{
		pMapPanel->SetPlacement( rcInitialMapBounds.x1 + (rcParent.x1 - PICTURE_BORDER_SIZE - rcInitialMapBounds.x1) * fProgress,
			rcInitialMapBounds.y1 + (rcParent.y1 - PICTURE_BORDER_SIZE - rcInitialMapBounds.y1) * fProgress,
			rcInitialMapBounds.Width() + (rcParent.Width() + PICTURE_BORDER_SIZE * 2 - rcInitialMapBounds.Width()) * fProgress,
			rcInitialMapBounds.Height() + (rcParent.Height() + PICTURE_BORDER_SIZE * 2 - rcInitialMapBounds.Height()) * fProgress, EWPF_ALL );
	}
}

bool CInterfaceChapterMapMenu::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

void CInterfaceChapterMapMenu::AfterLoad()
{
	CInterfaceScreenBase::AfterLoad();

	RegisterObservers();

	pReinfRoller1->Stop();
	pReinfRoller2->Stop();
	pReinfRoller3->Stop();
	PlayReinfRollerAnim( nCallsLeft - nCurrentMissionReinfs, nCallsLeft - nCurrentMissionReinfs );

	pMissionReinfRoller1->Stop();
	pMissionReinfRoller2->Stop();
	PlayMissionRollerAnim( nCurrentMissionReinfs, nCurrentMissionReinfs );

	if ( nSelectedMission >= 0 )
	{
		int nStoredIndex = nSelectedMission;
		nSelectedMission = -1;
		SelectTarget( nStoredIndex );
	}
}

bool CInterfaceChapterMapMenu::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );												 

	if ( bNeedToRunAnimation && eExitDir == EED_RE_ENTER )
	{
		EffectStart( EED_RE_ENTER );
		bNeedToRunAnimation = false;
	}

	if ( eReinfDescState != ERDWS_NONE && nDelay != 0 )
	{				// Rolling reinfs some direction or other
		if ( nDelay > 0 )
			--nDelay;
		else
			++nDelay;

		ReinfDescProcessScroll( nDelay );
	}
	if ( nFrontLineAnim >= 0 )
	{
		if ( timeStartEffect == 0 )
		{
			--nDelay;
			if ( nDelay <= 0 )				// Start animations now
			{
				timeStartEffect = Singleton<IGameTimer>()->GetAbsTime();
				PlayReinfRollerAnim( Singleton<IScenarioTracker>()->GetReinforcementCallsOld(), 
					Singleton<IScenarioTracker>()->GetReinforcementCallsLeftInChapter() );
			}
		}
		else
		{
			STarget &target = targets[nFrontLineAnim];
			float fValue;
			NTimer::STime timeCurrent = Singleton<IGameTimer>()->GetAbsTime();
			int nTimePassed = timeCurrent - timeStartEffect;
			if ( nTimePassed > s_nFrontLineAnimDuration )
			{
				fValue = target.fValue1;
				nFrontLineAnim = -1;
			}
			else
			{
				fValue = target.fValue0 + ( ( target.fValue1 - target.fValue0 ) * nTimePassed ) / s_nFrontLineAnimDuration;
			}
			pFrontlines->SetNode( target.nX, target.nY, target.vEndOffset.x, target.vEndOffset.y, fValue );
		}
	}

	if ( bRequestAutoSave )
	{
		DoAutoSave();
		bRequestAutoSave = false;
	}

	return bResult;
}

void CInterfaceChapterMapMenu::UpdateUIState()
{
	if ( eUIState == EUIS_PLAY_MISSION_START_EFFECT )
	{
		NTimer::STime timeCurrent = Singleton<IGameTimer>()->GetAbsTime();
		if ( eUIEffectState == EUIES_FADE )
		{
			float fFade = 1.0f - (float)( timeCurrent - timeStartEffect ) / (float)( s_nFadeEffectDuration );
			if ( fFade < 0.0f )
			{
				fFade = 0.0f;
				eUIEffectState = EUIES_EXPAND;
				timeStartEffect = timeCurrent;
				if ( pMapPanel )
				{
					rcInitialMapBounds = pMapPanel->GetWindowRect();
					pMapPanel->SetPriority( TOP_PRIORITY );
					GetScreen()->AddChild( pMapPanel );
				}
			}
			FadeMapElements( fFade );

			if ( eUIEffectState == EUIES_EXPAND )
			{
				ExpandMap( 1.0f );

				NGScene::SFrameTransitionInfo ftInfo;
				ftInfo.bRandomDir = false;
				ftInfo.vTransitionDir.Set( 1.0f, 0.0f );
				ftInfo.fTransitionLength = s_fTransitionEffectToPWLLength;
				ftInfo.nEffectDuration = s_nTransitionEffectToPWLDuration;
				ftInfo.fQuadsGroup1MinZ = 1.0f;
				ftInfo.fQuadsGroup1MaxZ = 1.1f;
				ftInfo.fQuadsGroup2MinZ = 1.0f;
				ftInfo.fQuadsGroup2MaxZ = 0.9f;
				CInterfaceScreenBase::Draw( NGScene::GetFrameTransitionCapture1( ftInfo ) );

				ExpandMap( 0.0f );
			}
		}
		else if ( eUIEffectState == EUIES_EXPAND )
		{
			float fProgress = (float)( timeCurrent - timeStartEffect ) / (float)( s_nExpandEffectDuration );
			if ( fProgress >= 1.0f )
			{
				fProgress = 1.0f;
				//				eUIEffectState = EUIES_WAIT;
				eUIEffectState = EUIES_DONE;
				timeStartEffect = timeCurrent;
			}
			ExpandMap( fProgress );
		}
		else if ( eUIEffectState == EUIES_WAIT )
		{
			if ( timeCurrent - timeStartEffect >= s_nWaitEffectDuration )
			{
				eUIState = EUIS_PLAY_MISSION_START_TRANSIT_EFFECT;
			}
		}
		else
			eUIState = EUIS_PLAY_MISSION_START_TRANSIT_EFFECT;
	}
}

void CInterfaceChapterMapMenu::Draw( NGScene::CRTPtr *pTexture )
{
	UpdateUIState();

	if ( eUIState == EUIS_PLAY_MISSION_START_TRANSIT_EFFECT )
	{
		PlayMissionStartMission();
	}
	else if ( eUIState == EUIS_PLAY_MISSION_DONE )
	{
		// do nothing
	}
	else
	{
		CInterfaceScreenBase::Draw( pTexture );
	}
}

void CInterfaceChapterMapMenu::SelectTarget( int nIndex )
{
	nSelectedMission = nIndex;

	if ( pPlay )
		pPlay->Enable( nIndex != -1 );

	// Set states for targets
	for ( int i = 0; i < targets.size(); ++i )
	{
		if ( i == nIndex )
		{
			if ( Singleton<IScenarioTracker>()->IsMissionActive() )
				Singleton<IScenarioTracker>()->MissionCancel();
			Singleton<IScenarioTracker>()->MissionStart( targets[i].pDBInfo );
			SwitchTargetState( i, true );
		}
		else
		{
			SwitchTargetState( i, false );
		}

		for ( int j = 0; j < targets[i].rewards.size(); ++j )
		{
			targets[i].rewards[j].pBtn->ShowWindow( false );
			targets[i].rewards[j].pBonusWnd->ShowWindow( false );
		}
	}

	nUpgradeMissionSelected = -1;
	nUpgradeRewardSelected = -1;

	if ( nIndex == -1 )
		MakeChapterInfo();
	else
		MakeMissionInfo( nIndex );

	// Place arrows for selected mission
	pFrontlines->ClearArrows();
	if ( nIndex >= 0 )
	{
		const SChapterMapMenuHelper::SMission &helperMission = pHelper->missions[nIndex];
		for ( int i = 0; i < helperMission.arrows.size(); ++i )
		{
			const SChapterMapMenuHelper::SArrow &arrow = helperMission.arrows[i];

			DWORD dwColor = arrow.GetColor();
			if ( arrow.nDependIndex >= 0 )
			{
				if ( targets[arrow.nDependIndex].eState != EMS_COMPLETED )
				{
					dwColor = arrow.GetDependentColor();
				}
			}
			
			pFrontlines->AddArrow( arrow.points, arrow.fWidth, arrow.pTexture, dwColor );
		}
	}
}

void CInterfaceChapterMapMenu::MakeMissionInfo( int nIndex )
{
	if ( pMissionDescBtn )
		pMissionDescBtn->Enable( true );

	const NDb::SChapter *pChapter = Singleton<IScenarioTracker>()->GetCurrentChapter();
	const NDb::SMissionEnableInfo &mission = pChapter->missionPath[nIndex];
	const NDb::SMapInfo *pMap = mission.pMap;

	if ( CHECK_TEXT_NOT_EMPTY_PRE(pMap->,LocalizedName) )
		pMissionName->SetText( pMissionName->GetDBText() + GET_TEXT_PRE(pMap->,LocalizedName) );
	else
		pMissionName->SetText( L"" );

	int nAvailMissionReinf = MakeArmyInfo();
	int nMissionReinfTypes = pMap->players[0].reinforcementTypes.size();

	int nCallsInMission = Min( mission.nRecommendedCalls, Singleton<IScenarioTracker>()->GetReinforcementCallsLeftInChapter() );
	PlayMissionRollerAnim( nCurrentMissionReinfs, nCallsInMission );
	PlayReinfRollerAnim( nCallsLeft - nCurrentMissionReinfs, nCallsLeft - nCallsInMission );
	nCurrentMissionReinfs = nCallsInMission;

	// disable selected mission until prereq
	if ( targets[nIndex].eState == EMS_DISABLED )
	{
		pPlay->Enable( false );
		pMissionEnabledLight->ShowWindow( false );
	}
	else
	{
		pPlay->Enable( true );
		pMissionEnabledLight->ShowWindow( true );
	}

	if ( nIndex == 0 )
	{
		pFinalBonus->ShowWindow( true );
		pBonusGrid->ShowWindow( false );
	}
	else
	{
		pFinalBonus->ShowWindow( false );
		pBonusGrid->ShowWindow( true );

		// Display bonus reinfs
		const NDb::SUIConstsB2 *pUIC = InterfaceState()->GetUIConsts();
		const int nBonuses = Min( mission.reward.size(), 4 );
		for ( int i = 0; i < 4; ++i )
		{
			if ( bonusButtons[i].pButton )
				bonusButtons[i].pButton->SetTooltip( L"" );
			if ( i < nBonuses )
			{
				bonusButtons[i].pButton->SetState( 0 );
				bonusButtons[i].pIcon->ShowWindow( true );
				const NDb::SChapterBonus *pBonus = mission.reward[i];
				if ( pBonus->eBonusType == NDb::CBT_REINF_CHANGE && !pBonus->bApplyToEnemy && pBonus->pReinforcementSet )
				{
					const NDb::SReinforcement *pReinf = pBonus->pReinforcementSet;
					bonusButtons[i].pReinforcement = pReinf;

					// Compose Tooltip
					if ( bonusButtons[i].pButton )
					{
						const int nLocalPlayer = 0;
						vector<IScenarioTracker::SChapterReinf> reinfs;
						Singleton<IScenarioTracker>()->GetChapterCurrentReinforcements( &reinfs, nLocalPlayer );
						bool bBonus = (reinfs[targets[nIndex].rewardDescs[i]->eReinforcementType].eState != IScenarioTracker::ERS_ENABLED);
						wstring wszPrefix;
						if ( IScreen *pScreen = GetScreen() )
							wszPrefix = pScreen->GetTextEntry( bBonus ? "T_REWARD_BONUS" : "T_REWARD_UPGRADE" );

						vector< pair<wstring, wstring> > params;
						params.push_back( pair<wstring, wstring>( DYNAMIC_TAG_REINF_TYPE, MakeTooltip( pReinf ) ) );
						SetDynamicTooltip( bonusButtons[i].pButton, wszPrefix, params );
					}

					for ( int nButtonConst = 0; nButtonConst < pUIC->reinfButtons.size(); ++nButtonConst )
					{
						if ( pUIC->reinfButtons[nButtonConst].eType == pReinf->eType )
						{
							bonusButtons[i].pIcon->SetTexture( pUIC->reinfButtons[nButtonConst].pTexture );
							break;
						}
					}

					if ( bonusButtons[i].pBonusWnd )
						bonusButtons[i].pBonusWnd->ShowWindow( false/*true*/ );
				}
				else
				{
					NI_ASSERT( false, StrFmt( "DESIGN: Incorrect ChapterBonus: chapterID \"%s\", mission %d, bonusID \"%s\"", pChapter->GetDBID().ToString().c_str(), nIndex, pBonus->GetDBID().ToString().c_str() ) );
					bonusButtons[i].pButton->SetState( 2 );
					bonusButtons[i].pIcon->ShowWindow( false );

					if ( bonusButtons[i].pBonusWnd )
						bonusButtons[i].pBonusWnd->ShowWindow( false );
				}
			}
			else
			{
				bonusButtons[i].pButton->SetState( 2 );
				bonusButtons[i].pIcon->ShowWindow( false );

				if ( bonusButtons[i].pBonusWnd )
					bonusButtons[i].pBonusWnd->ShowWindow( false );
			}
		}
	}

}

void CInterfaceChapterMapMenu::MakeChapterInfo()
{
	if ( pMissionDescBtn )
		pMissionDescBtn->Enable( false );

	if ( pMissionName )
		pMissionName->SetText( L"" );

	int nCallsInMission = 0;
	PlayMissionRollerAnim( nCurrentMissionReinfs, nCallsInMission );
	PlayReinfRollerAnim( nCallsLeft - nCurrentMissionReinfs, nCallsLeft - nCallsInMission );
	nCurrentMissionReinfs = nCallsInMission;

	if ( pPlay )
		pPlay->Enable( false );
	if ( pMissionEnabledLight )
		pMissionEnabledLight->ShowWindow( false );

	if ( pFinalBonus )
		pFinalBonus->ShowWindow( false );
	if ( pBonusGrid )
		pBonusGrid->ShowWindow( true );

	for ( int i = 0; i < 4; ++i )
	{
		bonusButtons[i].pButton->SetState( 2 );
		bonusButtons[i].pIcon->ShowWindow( false );

		if ( bonusButtons[i].pBonusWnd )
			bonusButtons[i].pBonusWnd->ShowWindow( false );
	}

	// Fill reinforcements
	for ( int i = 0; i < NDb::_RT_NONE; ++i )
	{
		IButton *pButton = 0;
		if ( reinfButtons[i].pBonusWnd )
			pButton = dynamic_cast_ptr<IButton*>( reinfButtons[i].pBonusWnd );

		if ( !reinfButtons[i].pButton )
			continue;

		if ( pButton )
		{
			pButton->ShowWindow( false );
			pButton->SetState( 0 );
		}
		reinfButtons[i].pButton->SetState( 2 );
		reinfButtons[i].pIcon->ShowWindow( false );
//		reinfButtons[i].pButton->SetTooltip( L""/*wszReinfDisabledPrefix + reinfButtons[i].wszDefaultTooltip*/ );
		if ( reinfButtons[i].pDefaultTexture )
			reinfButtons[i].pIcon->SetTexture( reinfButtons[i].pDefaultTexture );
	}
}

int CInterfaceChapterMapMenu::MakeArmyInfo( bool bShow )
{
	int nAvailReinfs = 0; 

	// Fill reinforcements
	for ( int i = 0; i < NDb::_RT_NONE; ++i )
	{
		IButton *pButton = 0;
		if ( reinfButtons[i].pBonusWnd )
			pButton = dynamic_cast_ptr<IButton*>( reinfButtons[i].pBonusWnd );

		if ( !reinfButtons[i].pButton )
			continue;

		const NDb::SReinforcement *pReinf = 
			Singleton<IScenarioTracker>()->GetReinforcement( 0, NDb::EReinforcementType( i ) );
		if ( pReinf )
		{
			if ( bShow )
			{
				if ( pButton )
				{
					pButton->ShowWindow( false/*NDBWrap::HasReinfUpgrade( pChapter, pReinf )*/ );
					pButton->SetState( 0 );
				}
				reinfButtons[i].pButton->SetState( 0 );
				reinfButtons[i].pIcon->ShowWindow( true );
				if ( pReinf->pIconTexture )
					reinfButtons[i].pIcon->SetTexture( pReinf->pIconTexture );
				else if ( reinfButtons[i].pDefaultTexture )
					reinfButtons[i].pIcon->SetTexture( reinfButtons[i].pDefaultTexture );

				// Compose Tooltip
				vector< pair<wstring, wstring> > params;
				params.push_back( pair<wstring, wstring>( DYNAMIC_TAG_REINF_TYPE, MakeTooltip( pReinf ) ) );
				SetDynamicTooltip( reinfButtons[i].pButton, wszReinfAvailablePrefix, params );

				if ( reinfButtons[i].pUnknownWnd )
					reinfButtons[i].pUnknownWnd->ShowWindow( false );
			}

			++nAvailReinfs;
		}
		else if ( bShow )
		{
			IScenarioTracker::EReinforcementState eRState = 
				Singleton<IScenarioTracker>()->GetReinforcementEnableState( 0, NDb::EReinforcementType( i ) );
			if ( eRState == IScenarioTracker::ERS_NOT_ENABLED )
			{
				reinfButtons[i].pButton->SetState( 1 );
				reinfButtons[i].pIcon->ShowWindow( true );

				vector< pair<wstring, wstring> > params;
				params.push_back( pair<wstring, wstring>( DYNAMIC_TAG_REINF_TYPE, reinfButtons[i].wszDefaultTooltip ) );
				SetDynamicTooltip( reinfButtons[i].pButton, wszReinfNotEnabledPrefix, params );

				if ( reinfButtons[i].pDisabledTexture )
					reinfButtons[i].pIcon->SetTexture( reinfButtons[i].pDisabledTexture );
				if ( reinfButtons[i].pUnknownWnd )
					reinfButtons[i].pUnknownWnd->ShowWindow( true );

				if ( pButton )
				{
					pButton->ShowWindow( false/*true*/ );
					pButton->SetState( 0 );
				}
			}
			else
			{
				if ( pButton )
				{
					pButton->ShowWindow( false );
					pButton->SetState( 0 );
				}
				reinfButtons[i].pButton->SetState( 2 );
				reinfButtons[i].pIcon->ShowWindow( false );
//				reinfButtons[i].pButton->SetTooltip( L""/*wszReinfDisabledPrefix + reinfButtons[i].wszDefaultTooltip*/ );
				if ( reinfButtons[i].pDefaultTexture )
					reinfButtons[i].pIcon->SetTexture( reinfButtons[i].pDefaultTexture );
				if ( reinfButtons[i].pUnknownWnd )
					reinfButtons[i].pUnknownWnd->ShowWindow( false );
			}
		}
	}

	return nAvailReinfs;
}

void CInterfaceChapterMapMenu::SwitchTargetState( int nTarget, bool bSelected )
{
	STarget &target = targets[nTarget];
	IButton *pButton = target.pWindow;
	NI_VERIFY( pButton, "Wrong button", return );

	if ( bSelected )
	{
		if ( target.pRecommended )
			target.pRecommended->ShowWindow( target.eState == EMS_RECOMMENDED );
		if ( target.eState == EMS_ENABLED )
		{
			pButton->SetState( 1 );
			pPlay->Enable( true );
			if ( target.pFlame )
				target.pFlame->ShowWindow( true );
		}
		else if ( target.eState == EMS_RECOMMENDED )
		{
			pButton->SetState( 6 );
			//pButton->SetState( 1 );
			pPlay->Enable( true );
			if ( target.pFlame )
				target.pFlame->ShowWindow( true );
		}
		else
		{
			pButton->SetState( 3 );
			pPlay->Enable( false );
			if ( target.pFlame )
				target.pFlame->ShowWindow( true );
		}
	}
	else
	{
		if ( target.pFlame )
			target.pFlame->ShowWindow( false );
		if ( target.pRecommended )
			target.pRecommended->ShowWindow( target.eState == EMS_RECOMMENDED );
		switch ( target.eState )
		{
		case EMS_DISABLED:
			pButton->SetState( 2 );
			pButton->Enable( true );
			break;
		case EMS_ENABLED:
			pButton->SetState( 0 );
			pButton->Enable( true );
			break;
		case EMS_RECOMMENDED:
//			pButton->SetState( 5 );
			pButton->SetState( 0 );
			pButton->Enable( true );
			break;
		case EMS_COMPLETED:
			pButton->SetState( 4 );
			pButton->Enable( false );
			if ( target.pCompleted )
				target.pCompleted->ShowWindow( true );
			break;
		}
	}

	UpdateRecommendedButton( target, false );
}

struct STmpReinfEntry
{
	const NDb::SHPObjectRPGStats *pStats;
	int nCount;

	bool operator==( const STmpReinfEntry &reinf ) const
	{
		bool bResult = (pStats == reinf.pStats);
		return bResult;
	}
};

const wstring CInterfaceChapterMapMenu::MakeTooltip( const NDb::SReinforcement *pReinf )
{
	wstring wszTooltip = L"";
	//CRAP
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pReinf->,LocalizedName) )
		wszTooltip = GET_TEXT_PRE(pReinf->,LocalizedName);

	/*list<STmpReinfEntry> units;

	for ( int i = 0; i < pReinf->entries.size(); ++i )
	{
		STmpReinfEntry entry;
		entry.nCount = 1;

		if ( pReinf->entries[i].pMechUnit )
			entry.pStats = pReinf->entries[i].pMechUnit;
		else if ( pReinf->entries[i].pSquad )
			entry.pStats = pReinf->entries[i].pSquad;
		else
			continue;

		bool bFound = false;
		for ( list<STmpReinfEntry>::iterator it = units.begin(); it != units.end(); ++it )
		{
			if ( *it == entry )
			{
				(*it).nCount += 1;
				bFound = true;
				break;
			}
		}
		if ( !bFound )
			units.push_back( entry );
	}


	for ( list<STmpReinfEntry>::iterator it = units.begin(); it != units.end(); ++it )
	{
		wstring wszUnit = L"<br>";

		if ( *it == units.back() )
			wszUnit = L"";

		if ( (*it).pStats->pLocalizedName )
			wszUnit = (*it).pStats->pLocalizedName->wszText + wszUnit;

		wszUnit = NStr::ToUnicode( StrFmt( "%d x ", (*it).nCount ) ) + wszUnit;

		wszTooltip = wszTooltip + wszUnit;
	}*/

	return wszTooltip;
}

void CInterfaceChapterMapMenu::OnTargetSelect( const string &szSender )
{
	for ( int i = 0; i < targets.size(); ++i )
	{
		if ( targets[i].pWindow->GetName() == szSender )
		{
			SelectTarget( i );
			return;
		}
	}
//	SelectTarget( -1 );
}

bool CInterfaceChapterMapMenu::OnTargetDblClick( const string &szSender )
{
	if ( nSelectedMission >= 0 && targets[nSelectedMission].pWindow->GetName() == szSender &&
		pPlay && pPlay->IsEnabled() )
	{
		NMainLoop::Command( ML_COMMAND_MISSION_BRIEFING, "" );
	}
	
	return true;
}

void CInterfaceChapterMapMenu::DoAutoSave()
{
	/*if ( !Singleton<IScenarioTracker>()->IsMissionWon() )
		return;*/

	if ( const NDb::SMapInfo *pLastMission = Singleton<IScenarioTracker>()->GetLastMission() )
	{
		wstring wszMissionName;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pLastMission->,LocalizedName) )
			wszMissionName = GET_TEXT_PRE(pLastMission->,LocalizedName);

		wstring wszName = InterfaceState()->GetTextEntry( "T_AUTO_SAVE_NAME" );
		if ( !wszName.empty() )
		{
			Draw( 0 );
			InterfaceState()->GetScreenShotTexture()->Generate( true );
			NSaveLoad::MakeUniqueSave( wszName + wszMissionName, false, true, true );
		}
	}
}

void CInterfaceChapterMapMenu::OnPopupClicked( const string &szSender )
{
	for( int i = 0; i < bonusButtons.size(); ++i )
	{
		if ( bonusButtons[i].pButton->GetName() == szSender )
		{
//			ShowReinfDesc( bonusButtons[i].pReinforcement );
			const NDb::SReinforcement *pNew = bonusButtons[i].pReinforcement;
			const NDb::SReinforcement *pCurrent = 0;
//			if ( pNew )
//				pCurrent = Singleton<IScenarioTracker>()->GetReinforcement( 0, pNew->eType );

			vector<IScenarioTracker::SChapterReinf> chapterReinfs;
			IScenarioTracker *pST = Singleton<IScenarioTracker>();
			pST->GetChapterCurrentReinforcements( &chapterReinfs, pST->GetLocalPlayer() );
			for ( int i = 0; i < chapterReinfs.size(); ++i )
			{
				IScenarioTracker::SChapterReinf &reinf = chapterReinfs[i];
				if ( reinf.pDBReinf && pNew && reinf.pDBReinf->eType == pNew->eType && reinf.eState == IScenarioTracker::ERS_ENABLED )
				{
					pCurrent = reinf.pDBReinf;
					break;
				}
			}

			HideDialogs();
			pReinfUpgrade->ShowReinf( pCurrent, pNew );
			return;
		}
	}

	for( int i = 0; i < reinfButtons.size(); ++i )
	{
		if ( !reinfButtons[i].pButton )
			continue;

		if ( reinfButtons[i].pButton->GetName() == szSender )
		{
//			ShowReinfDesc( Singleton<IScenarioTracker>()->GetReinforcement( 0, NDb::EReinforcementType( i ) ) );
			HideDialogs();
			pReinfComposition->ShowReinf( Singleton<IScenarioTracker>()->GetReinforcement( 0, NDb::EReinforcementType( i ) ) );
			return;
		}
	}

	if ( nUpgradeMissionSelected >= 0 && nUpgradeRewardSelected >= 0 )
	{
		const NDb::SChapterBonus *pReward = targets[nUpgradeMissionSelected].rewardDescs[nUpgradeRewardSelected];
		const NDb::SReinforcement *pCurrent = Singleton<IScenarioTracker>()->GetReinforcement( 0, pReward->eReinforcementType );
		const NDb::SReinforcement *pNew = pReward->pReinforcementSet;
//		ShowReinfDesc( pCurrent, pNew );
		HideDialogs();
		pReinfUpgrade->ShowReinf( pCurrent, pNew );
		return;
	}

	NI_ASSERT( false, "popup_clicked received from unknown item" );
}

bool CInterfaceChapterMapMenu::OnFixBonus( const string &szSender )
{
	bool bFound = false;
	for( int i = 0; i < reinfButtons.size(); ++i )
	{
		if ( !reinfButtons[i].pButton )
			continue;

		IButton *pButton = dynamic_cast_ptr<IButton*>( reinfButtons[i].pBonusWnd );
		if ( !pButton )
			continue;

		if ( pButton->GetName() == szSender )
		{
			if ( pButton->GetState() == 1 )
			{
				pButton->SetState( 0 );
				nUpgradeMissionSelected = -1;
				nUpgradeRewardSelected = -1;

				break;
			}

			pButton->SetState( 1 );

			for ( int k = 0; k < targets.size() && !bFound; ++k )
			{
				STarget &target = targets[k];

				for ( int j = 0; j < target.rewards.size(); ++j )
				{
					if ( target.rewardDescs[j]->eBonusType == NDb::CBT_REINF_CHANGE && target.rewardDescs[j]->eReinforcementType == i )
					{
						SReward &reward = target.rewards[j];

						if ( nUpgradeMissionSelected >= 0 && nUpgradeRewardSelected >= 0 )
						{
							SReward &rewardOld = targets[nUpgradeMissionSelected].rewards[nUpgradeRewardSelected];
							if ( rewardOld.pBtn )
								rewardOld.pBtn->ShowWindow( false );
							if ( rewardOld.pBonusWnd )
								rewardOld.pBonusWnd->ShowWindow( false );
						}

						if ( reward.pBtn )
							reward.pBtn->ShowWindow( true );
						if ( reward.pBonusWnd )
							reward.pBonusWnd->ShowWindow( false/*true*/ );

						nUpgradeMissionSelected = k;
						nUpgradeRewardSelected = j;
						bFound = true;
						break;
					}
				}
			}
		}
		else
		{
			pButton->SetState( 0 );
		}
	}

	return true;
}

void CInterfaceChapterMapMenu::ShowAllRewards( bool bShow )
{
	for ( int i = 0; i < targets.size(); ++i )
	{
		STarget &target = targets[i];
		
		for ( int j = 0; j < target.rewards.size(); ++j )
		{
			SReward &reward = target.rewards[j];
			if ( reward.pBtn )
				reward.pBtn->ShowWindow( bShow );

			if ( reward.pBonusWnd )
				reward.pBonusWnd->ShowWindow( false/*true*/ );
		}
	}
}

void CInterfaceChapterMapMenu::HideChildren( IWindow *pParent )
{
	if ( !pParent )
		return;

	for ( int j = 0; j < pParent->GetNumChildren(); ++j )
		pParent->GetChild( j )->ShowWindow( false );
}

void CInterfaceChapterMapMenu::PlayReinfRollerAnim( int nStart, int nEnd )
{
	vector<IPlayer*> rollers;
	rollers.push_back( pReinfRoller1 );
	rollers.push_back( pReinfRoller2 );
	rollers.push_back( pReinfRoller3 );
	
	NUIElementsHelper::PlayRollerAnim( rollers, nStart, nEnd, ROLLER_TIME );
}

void CInterfaceChapterMapMenu::PlayMissionRollerAnim( int nStart, int nEnd )
{
	vector<IPlayer*> rollers;
	rollers.push_back( pMissionReinfRoller2 );
	rollers.push_back( pMissionReinfRoller1 );
	
	NUIElementsHelper::PlayRollerAnim( rollers, nStart, nEnd, ROLLER_TIME );
}

bool CInterfaceChapterMapMenu::OnTargetOver( const string &szSender )
{
	return true;
	for ( int i = 0; i < targets.size(); ++i )
	{
		STarget &target = targets[i];
		if ( target.pWindow->GetName() == szSender )
		{
			UpdateRecommendedButton( target, false );
			break;
		}
	}

	return true;
}

bool CInterfaceChapterMapMenu::OnTargetPushed( const string &szSender )
{
	for ( int i = 0; i < targets.size(); ++i )
	{
		STarget &target = targets[i];
		if ( target.pWindow->GetName() == szSender )
		{
			UpdateRecommendedButton( target, true );
			break;
		}
	}

	return true;
}

bool CInterfaceChapterMapMenu::OnTargetPushedBack( const string &szSender )
{
	for ( int i = 0; i < targets.size(); ++i )
	{
		STarget &target = targets[i];
		if ( target.pWindow->GetName() == szSender )
		{
			UpdateRecommendedButton( target, false );
			break;
		}
	}

	return true;
}

void CInterfaceChapterMapMenu::UpdateRecommendedButton( STarget &target, bool bPushed )
{
	if ( target.pWindow->GetState() == 1 )
	{
		if ( target.pRecommended )
			target.pRecommended->SetPlacement( target.nRecommendedX + SELECTION_SELECTED_DELTA_X, 
				target.nRecommendedY + SELECTION_SELECTED_DELTA_Y, 0, 0, EWPF_POS_X | EWPF_POS_Y );
	}
	else if ( bPushed )
	{
		if ( target.pRecommended )
			target.pRecommended->SetPlacement( target.nRecommendedX + SELECTION_PUSHED_DELTA_X, 
				target.nRecommendedY + SELECTION_PUSHED_DELTA_Y, 0, 0, EWPF_POS_X | EWPF_POS_Y );
	}
	else
	{
		if ( target.pRecommended )
			target.pRecommended->SetPlacement( target.nRecommendedX, 
				target.nRecommendedY, 0, 0, EWPF_POS_X | EWPF_POS_Y );
	}
}

bool CInterfaceChapterMapMenu::OnReinfUpgradeDialogClose( const string &szSender )
{
	pReinfUpgrade->Hide();
	pReinfComposition->Hide();
	return true;
}

bool CInterfaceChapterMapMenu::OnReinfUpgradeUnitBtn( const string &szSender )
{
	pReinfUpgrade->UnitBtnPressed( szSender );
	pReinfComposition->UnitBtnPressed( szSender );
	return true;
}

bool CInterfaceChapterMapMenu::OnChapterDescDlgClose( const string &szSender )
{
	pChapterDescDlg->Hide();
	bInitialDialogVisible = false;

	ProceedInitialDialogs();

	return true;
}

bool CInterfaceChapterMapMenu::OnMissionDescDlgClose( const string &szSender )
{
	pMissionDescDlg->Hide();
	return true;
}

bool CInterfaceChapterMapMenu::OnShowMissionDesc( const string &szSender )
{
	if ( nSelectedMission >= 0 )
	{
		const NDb::SChapter *pChapter = Singleton<IScenarioTracker>()->GetCurrentChapter();
		const NDb::SMissionEnableInfo &mission = pChapter->missionPath[nSelectedMission];
		pChapterDescDlg->Hide(); // CRAP
		pMissionDescDlg->Show( mission );
	}
	return true;
}

void CInterfaceChapterMapMenu::HideDialogs()
{
	if ( pReinfDesc )
		pReinfDesc->ShowWindow( false );
	if ( pReinfDesc1Unit )
		pReinfDesc1Unit->ShowWindow( false );
	if ( pReinfDesc1Reinf )
		pReinfDesc1Reinf->ShowWindow( false );
	if ( pReinfDescMultiReinf )
		pReinfDescMultiReinf->ShowWindow( false );
	if ( pReinfDescUpgrade )
		pReinfDescUpgrade->ShowWindow( false );

	pReinfUpgrade->Hide();
	pReinfComposition->Hide();
	pChapterDescDlg->Hide();
	pMissionDescDlg->Hide();
}

void CInterfaceChapterMapMenu::ProceedInitialDialogs()
{
	if ( bInitialDialogVisible )
		return;

	if ( InterfaceState()->IsFirstTimeInChapter() )
	{
		InterfaceState()->SetFirstTimeInChapter( false );
		const NDb::SChapter *pChapter = Singleton<IScenarioTracker>()->GetCurrentChapter();
		if ( pChapter )
		{
			pChapterDescDlg->Show( pChapter );
			bInitialDialogVisible = true;
			return;
		}
	}
	
	if ( InterfaceState()->IsAutoShowCommanderScreen() )
		NMainLoop::Command( ML_COMMAND_ARMY_SCREEN, "" );
}

// CICChapterMapMenu

#ifndef _SINGLE_DEMO
void CICChapterMapMenu::PreCreate()
{
}

void CICChapterMapMenu::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICChapterMapMenu::Configure( const char *pszConfig )
{
}

#endif // _SINGLE_DEMO

void ChapterMapOutro( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	InterfaceState()->VerifyScenarioTracker( IInterfaceState::ESTT_NONE );

//	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
}

void ChapterMap( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	NMainLoop::Command( ML_COMMAND_CHAPTER_MAP_MENU, "" );
}

void ChapterMapAutostartMission( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( !pST->IsChapterActive() )
	{
		pST->NextChapter();
	}
	const NDb::SChapter *pChapter = pST->GetCurrentChapter();
	if ( pChapter )
	{
		int nRecommendedTarget = -1;
		int nRecommendedValue = -1;
		for ( int i = 0; i < pChapter->missionPath.size(); ++i )
		{
			const NDb::SMissionEnableInfo &info = pChapter->missionPath[i];
			if ( nRecommendedTarget == -1 || nRecommendedValue > info.nRecommendedOrder )
			{
				nRecommendedTarget = i;
				nRecommendedValue = info.nRecommendedOrder;
			}
		}
		
		const NDb::SMapInfo *pMap = 0;
		if ( 0 <= nRecommendedTarget && nRecommendedTarget < pChapter->missionPath.size() )
			pMap = pChapter->missionPath[nRecommendedTarget].pMap;

		if ( pMap )
		{
			const NDb::SCampaign *pCampaign = pST->GetCurrentCampaign();
			if ( pCampaign && pCampaign->pIntermissionMusic )
				Singleton<IMusicSystem>()->Init( pCampaign->pIntermissionMusic, 0 );

			pST->MissionStart( pMap );

			NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
			NMainLoop::Command( ML_COMMAND_MISSION, StrFmt( "%s;normal", pMap->GetDBID().ToString().c_str() ) );
		}
	}
}

void ChapterMapAfterIntroMovie( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	// start chapter music
	const NDb::SCampaign *pCampaign = Singleton<IScenarioTracker>()->GetCurrentCampaign();
	if ( pCampaign->pIntermissionMusic )
		Singleton<IMusicSystem>()->Init( pCampaign->pIntermissionMusic, 0 );
}

#if !defined(_SINGLE_DEMO) || defined(_MP_DEMO)
START_REGISTER(ChapterMapCommands)
REGISTER_CMD( "chapter_map_outro", ChapterMapOutro );
REGISTER_CMD( "chapter_map", ChapterMap );
REGISTER_CMD( "chapter_map_autostart_mission", ChapterMapAutostartMission );
REGISTER_CMD( "chapter_map_after_intro_movie", ChapterMapAfterIntroMovie );

REGISTER_VAR_EX( "transition_effect_to_pwl_duration", NGlobal::VarIntHandler, &s_nTransitionEffectToPWLDuration, 700, STORAGE_NONE );
REGISTER_VAR_EX( "fade_effect_to_pwl_duration", NGlobal::VarIntHandler, &s_nFadeEffectDuration, 400, STORAGE_NONE );
REGISTER_VAR_EX( "expand_effect_to_pwl_duration", NGlobal::VarIntHandler, &s_nExpandEffectDuration, 400, STORAGE_NONE );
REGISTER_VAR_EX( "wait_effect_to_pwl_duration", NGlobal::VarIntHandler, &s_nWaitEffectDuration, 200, STORAGE_NONE );
REGISTER_VAR_EX( "transition_effect_to_pwl_length", NGlobal::VarFloatHandler, &s_fTransitionEffectToPWLLength, 0.4f, STORAGE_NONE );
REGISTER_VAR_EX( "front_line_move_duration", NGlobal::VarIntHandler, &s_nFrontLineAnimDuration, 5000, STORAGE_NONE );

FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x170C1381, CInterfaceChapterMapMenu )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_CHAPTER_MAP_MENU, CICChapterMapMenu )
REGISTER_SAVELOAD_CLASS_NM( 0x170C1382, CReactions, CInterfaceChapterMapMenu );
#endif // !defined(_SINGLE_DEMO) || defined(_MP_DEMO)


