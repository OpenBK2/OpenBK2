#include "StdAfx.h"
#include "InterfaceChapterMapMenu.h"
#include "SceneB2/Scene.h"
#include "InterfaceState.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "ScenarioTracker.h"
#include "Misc/STrProc.h"
#include "UISpecificB2/EffectorB2Move.h"
#include "GameXClassIDs.h"
#include "UIElementsHelper.h"
#include "InterfaceChapterMapMenuDialogs.h"
#include "System/Text.h"
#include "DBGameRoot.h"
#include "Sound/MusicSystem.h"
#include "UI/WindowTooltip.h"

const wchar_t* DYNAMIC_TAG_MISSION_NAME = L"mission_name";
const wchar_t* DYNAMIC_TAG_MISSION_STATUS = L"mission_status";
const wchar_t* DYNAMIC_TAG_FINAL_MISSION_LOCKS = L"final_mission_locks";

// Too much is done in ChapterMap's Init() 
// So it has been moved to this file

void CInterfaceChapterMapMenu::InitLoadControls()
{
	pChapterMap = GetChildChecked<IWindow>( pScreen, "ChapterMap", true );
	pChapterMapTarget = GetChildChecked<IButton>( pScreen, "ChapterMapTarget", true );
	pChapterMapTargetBig = GetChildChecked<IButton>( pScreen, "ChapterMapTargetBig", true );
	pMapPanel = GetChildChecked<IWindow>( pScreen, "ChapterMapBgr", true );
	pChapterMapBgr = GetChildChecked<IWindow>( pScreen, "ChapterMapBgr", true );

	pMissionName = GetChildChecked<ITextView>( pMain, "MissionName", true );
	pFinalBonus = GetChildChecked<IWindow>( pMain, "FinalBonus", true );
	const NDb::SCampaign *pCampaign = Singleton<IScenarioTracker>()->GetCurrentCampaign();
	if ( pCampaign )
		pFinalBonus->SetTexture( pCampaign->pTextureChapterFinishBonus );
	pMissionDescBtn = GetChildChecked<IButton>( pScreen, "MissionDescBtn", true );

	pFrontlines = GetChildChecked<IPotentialLines>( pScreen, "Frontlines", true );
	pFrontlines->ShowWindow( true );		// It is initially hidden (to avoid crashes in editor)

	pReinfRoller1 = GetChildChecked<IPlayer>( pMain, "ReinfQty1", true );
	pReinfRoller2 = GetChildChecked<IPlayer>( pMain, "ReinfQty2", true );
	pReinfRoller3 = GetChildChecked<IPlayer>( pMain, "ReinfQty3", true );
	// Set rollers to 000 and pause
	NUIElementsHelper::InitRoller( pReinfRoller1 );
	NUIElementsHelper::InitRoller( pReinfRoller2 );
	NUIElementsHelper::InitRoller( pReinfRoller3 );

	pMissionReinfRoller1 = GetChildChecked<IPlayer>( pMain, "ReinfMission1", true );
	pMissionReinfRoller2 = GetChildChecked<IPlayer>( pMain, "ReinfMission2", true );
	// Set rollers to 00 and pause
	NUIElementsHelper::InitRoller( pMissionReinfRoller1 );
	NUIElementsHelper::InitRoller( pMissionReinfRoller2 );
	nCurrentMissionReinfs = 0;

	pMissionEnabledLight = GetChildChecked<IWindow>( pMain, "MissionEnabledLight", true );

	// Reinf grid
	pReinfGrid = GetChildChecked<IWindow>( pMain, "ReinforcementGrid", true );
	pReinfGridUpgradableTemplate = GetChildChecked<IButton>( pReinfGrid, "UpgradableTemplate", true );
	IWindow *pFirstReinfGridItem = GetChildChecked<IWindow>( pReinfGrid, "ButtonReinf02", true );

	int nReinfGridUpgradableTemplateX = 0;
	int nReinfGridUpgradableTemplateY = 0;
	if ( pReinfGridUpgradableTemplate )
		pReinfGridUpgradableTemplate->GetPlacement( &nReinfGridUpgradableTemplateX, &nReinfGridUpgradableTemplateY, 0, 0 );
	int nFirstReinfGridItemX = 0;
	int nFirstReinfGridItemY = 0;
	if ( pFirstReinfGridItem )
		pFirstReinfGridItem->GetPlacement( &nFirstReinfGridItemX, &nFirstReinfGridItemY, 0, 0 );
	vReinfGridUpgradableTemplateDelta.x = nReinfGridUpgradableTemplateX - nFirstReinfGridItemX;
	vReinfGridUpgradableTemplateDelta.y = nReinfGridUpgradableTemplateY - nFirstReinfGridItemY;

	// Bonus grid
	bonusButtons[0].pButton = GetChildChecked<IButton>( pMain, "ButtonBonus01", true );
	bonusButtons[1].pButton = GetChildChecked<IButton>( pMain, "ButtonBonus02", true );
	bonusButtons[2].pButton = GetChildChecked<IButton>( pMain, "ButtonBonus03", true );
	bonusButtons[3].pButton = GetChildChecked<IButton>( pMain, "ButtonBonus04", true );

	pBonusGrid = GetChildChecked<IWindow>( pMain, "BonusGrid", true );
	pBonusGridBonusTemplate = GetChildChecked<IWindow>( pBonusGrid, "UpgradeTemplate", true );
	IWindow *pFirstBonusGridItem = bonusButtons[0].pButton;

	int nBonusGridBonusTemplateX = 0;
	int nBonusGridBonusTemplateY = 0;
	if ( pBonusGridBonusTemplate )
		pBonusGridBonusTemplate->GetPlacement( &nBonusGridBonusTemplateX, &nBonusGridBonusTemplateY, 0, 0 );
	int nFirstBonusGridItemX = 0;
	int nFirstBonusGridItemY = 0;
	if ( pFirstBonusGridItem )
		pFirstBonusGridItem->GetPlacement( &nFirstBonusGridItemX, &nFirstBonusGridItemY, 0, 0 );
	vBonusGridBonusTemplateDelta.x = nBonusGridBonusTemplateX - nFirstBonusGridItemX;
	vBonusGridBonusTemplateDelta.y = nBonusGridBonusTemplateY - nFirstBonusGridItemY;

	// Hide unused elements of the bonus buttons
	for ( int i = 0; i < 4; ++i )
	{
		bonusButtons[i].pIcon = GetChildChecked<IWindow>( bonusButtons[i].pButton, "Icon", true );
		HideChildren( bonusButtons[i].pIcon );

		int nButtonX = 0;
		int nButtonY = 0;
		bonusButtons[i].pButton->GetPlacement( &nButtonX, &nButtonY, 0, 0 );

		bonusButtons[i].pBonusWnd = AddWindowCopy( pBonusGrid, pBonusGridBonusTemplate );
		if ( bonusButtons[i].pBonusWnd )
		{
			bonusButtons[i].pBonusWnd->SetPlacement( nButtonX + vBonusGridBonusTemplateDelta.x, 
				nButtonY + vBonusGridBonusTemplateDelta.y, 0, 0, EWPF_POS_X | EWPF_POS_Y );
		}
	}

	// Reinf Desc {
	pReinfDesc = GetChildChecked<IWindow>( pScreen, "ReinfDescriptionBackground", false );
	pReinfDesc1Unit = GetChildChecked<IWindow>( pReinfDesc, "ReinfDesc1Unit", true );
	pReinfDesc1Reinf = GetChildChecked<IWindow>( pReinfDesc, "ReinfDesc1Reinf", true );
	pReinfDescMultiReinf = GetChildChecked<IWindow>( pReinfDesc, "ReinfDescMultiReinf", true );
	pReinfDescUpgrade = GetChildChecked<IWindow>( pReinfDesc, "ReinfDescUpgrade", true );
	pReinfDesc->ShowWindow( false );

	IWindow *pUnitsArea = GetChildChecked<IWindow>( pReinfDescUpgrade, "ReinfDescUnitsArea2", true );
	for ( int i = 0; i < 5; ++i )
	{
		IButton *pButton = GetChildChecked<IButton>( pUnitsArea, StrFmt( "ReinfDescButton%1d", i + 1 ), true );
		if ( pButton )
			pButton->SetName( StrFmt( "ReinfDescButtonUpg%1d", i + 1 ) );
	}
	// } Reinf Desc

	pArmyManager = GetChildChecked<IButton>( pScreen, "ArmyManagerButton", true );
	if ( pArmyManager )
		bArmyManagerInitialEnable = pArmyManager->IsEnabled();
	pPlay = GetChildChecked<IButton>( pScreen, "PlayButton", true );

	pChapterMapLeft = GetChildChecked<IWindow>( pMain, "ChapterMapLeft", true );
	pChapterMapRight = GetChildChecked<IWindow>( pMain, "ChapterMapRight", true );
	if ( pChapterMapLeft )
		pChapterMapLeft->GetPlacement( &nChapterMapLeftX, &nChapterMapLeftY, 0, 0 );
	if ( pChapterMapRight )
		pChapterMapRight->GetPlacement( &nChapterMapRightX, &nChapterMapRightY, 0, 0 );
		
	pReinfUpgrade = new SChapterReinfUpgrade();
	pReinfUpgrade->InitControls( pReinfDesc );
	pReinfComposition = new SChapterReinfComposition();
	pReinfComposition->InitControls( pReinfDesc );
	pChapterDescDlg = new SChapterDesc();
	pChapterDescDlg->InitControls( pReinfDesc );
	pMissionDescDlg = new SMissionDesc();
	pMissionDescDlg->InitControls( pReinfDesc );

	HideDialogs();
}

void CInterfaceChapterMapMenu::InitReinforcements()
{
	const NDb::SUIConstsB2 *pUIC = InterfaceState()->GetUIConsts();

	// Associate reinforcement buttons
	reinfButtons.resize( NDb::_RT_NONE );
	for ( int i = 0; i < NDb::_RT_NONE; ++i )
	{
		reinfButtons[i].pButton = 0;
		reinfButtons[i].pIcon = 0;
		reinfButtons[i].pDefaultTexture = 0;
	}

	// Find and tune reinf.buttons in Mission Info
	for ( int i = 0; i < pUIC->reinfButtons.size(); ++i )
	{
		const NDb::SWindowMSButton *pRButton = pUIC->reinfButtons[i].pButton;
		NI_VERIFY( pRButton, "Wrong button in UIConstsB2", continue );
		IButton *pButtonElement = GetChildChecked<IButton>( pReinfGrid, pRButton->szName, true );

		if ( !pButtonElement )
			continue;

		int nIndex = pUIC->reinfButtons[i].eType;
		reinfButtons[nIndex].pButton = pButtonElement;

		if ( pRButton && CHECK_TEXT_NOT_EMPTY_PRE(pRButton->,Tooltip) )
			reinfButtons[nIndex].wszDefaultTooltip = GET_TEXT_PRE(pRButton->,Tooltip);

		reinfButtons[nIndex].pIcon = pButtonElement->GetChild( "Icon", true );
		reinfButtons[nIndex].pUnknownWnd = GetChildChecked<IWindow>( pButtonElement, "Unknown", true );
		if ( reinfButtons[nIndex].pUnknownWnd )
			reinfButtons[nIndex].pUnknownWnd->ShowWindow( false );

		// Hide unused elements of the button
		HideChildren( reinfButtons[nIndex].pIcon );

		if ( pUIC->reinfButtons[i].pTexture )
		{
			reinfButtons[nIndex].pIcon->SetTexture( pUIC->reinfButtons[i].pTexture );
			reinfButtons[nIndex].pDefaultTexture = pUIC->reinfButtons[i].pTexture;
		}
		if ( pUIC->reinfButtons[i].pTextureDisabled )
			reinfButtons[nIndex].pDisabledTexture = pUIC->reinfButtons[i].pTextureDisabled;
			
		if ( pReinfGrid )
		{
			int nButtonX = 0;
			int nButtonY = 0;
			reinfButtons[nIndex].pButton->GetPlacement( &nButtonX, &nButtonY, 0, 0 );
			
			IWindow *pUpgradableWnd = AddWindowCopy( pReinfGrid, pReinfGridUpgradableTemplate );
			reinfButtons[nIndex].pBonusWnd = pUpgradableWnd;
			if ( pUpgradableWnd )
			{
				pUpgradableWnd->SetName( StrFmt( "FixUpgrade%d", nIndex ) );
				pUpgradableWnd->SetPlacement( nButtonX + vReinfGridUpgradableTemplateDelta.x, 
					nButtonY + vReinfGridUpgradableTemplateDelta.y, 0, 0, EWPF_POS_X | EWPF_POS_Y );
			}
		}
	}

	nUpgradeMissionSelected = -1;
	nUpgradeRewardSelected = -1;
}

void CInterfaceChapterMapMenu::InitMissions()
{
	const NDb::SUIConstsB2 *pUIC = InterfaceState()->GetUIConsts();

	// Arrange missions on map
	IButton *pBonusTemplate = GetChildChecked<IButton>( pMain, "MissionBonusTemplate", true );

	CMissions missionsOpen;
	CMissions missionsWon;
	Singleton<IScenarioTracker>()->GetEnabledMissions( &missionsOpen );
	Singleton<IScenarioTracker>()->GetCompletedMissions( &missionsWon );
	bool bIsMissionWon = Singleton<IScenarioTracker>()->IsMissionWon();
	const NDb::SMapInfo *pLastMission = Singleton<IScenarioTracker>()->GetLastMission();

	pHelper = new SChapterMapMenuHelper( pChapter, pChapterMap );
	const NDb::SCampaign *pCampaign = Singleton<IScenarioTracker>()->GetCurrentCampaign();
	
	int nIndex = 0;
	for ( vector< const NDb::SMissionEnableInfo >::const_iterator it = pChapter->missionPath.begin(); 
		it != pChapter->missionPath.end(); ++it, ++nIndex )
	{
		const NDb::SMissionEnableInfo &info = *it;

		STarget target;

		IWindow *pElement;
		bool bFinalMission = false;
		if ( it == pChapter->missionPath.begin() )
		{
			bFinalMission = true;
			pElement = AddWindowCopy( pChapterMapBgr, pChapterMapTargetBig );
			//target.pFlame = GetChildChecked<IWindowFrameSequence>( pElement, "TargetBigFlame", true );
			target.pRecommended = GetChildChecked<IWindow>( pElement, "Recommended", true );
		}
		else
		{
			pElement = AddWindowCopy( pChapterMapBgr, pChapterMapTarget );
			//target.pFlame = GetChildChecked<IWindowFrameSequence>( pElement, "TargetFlame", true );
			//target.pCompleted = GetChildChecked<IWindow>( pElement, "TargetCompleted", true );
			target.pRecommended = GetChildChecked<IWindow>( pElement, "Recommended", true );
			//target.pBonusIcon = GetChildChecked<IWindow>( pElement, "Bonus", true );
			if ( target.pCompleted && pCampaign )
				target.pCompleted->SetTexture( pCampaign->pTextureMissionCompleted );
		}
		if ( target.pFlame )
			target.pFlame->Run( true );
		if ( target.pRecommended )
			target.pRecommended->GetPlacement( &target.nRecommendedX, &target.nRecommendedY, 0, 0 );

		IButton *pTargetButton = dynamic_cast<IButton*>( pElement );

		target.nX = pHelper->missions[nIndex].vPos.x;
		target.nY = pHelper->missions[nIndex].vPos.y;
		target.vEndOffset = pHelper->missions[nIndex].vEndOffset; 

		int nX, nY;
		pElement->GetPlacement( 0, 0, &nX, &nY );
		nX = target.nX - nX / 2;
		nY = target.nY - nY / 2;

		pElement->SetName( StrFmt( "Target%d", nIndex ) );
		pElement->ShowWindow( true );
		pElement->SetPlacement( nX, nY, 0, 0, EWPF_POS_X | EWPF_POS_Y );

		target.pWindow = dynamic_cast<IButton*>( pElement );
		target.pDBInfo = info.pMap;
		target.eState = EMS_DISABLED;
		target.nCalls = info.nRecommendedCalls;
		target.nPriority = info.nRecommendedOrder;

		for ( int i = 0; i < missionsOpen.size(); ++i )
		{
			if ( target.pDBInfo == missionsOpen[i] )
			{
				target.eState = EMS_ENABLED;
				break;
			}
		}

		if ( target.eState == EMS_DISABLED )
		{
			for ( int i = 0; i < missionsWon.size(); ++i )
			{
				if ( target.pDBInfo == missionsWon[i] )
				{
					target.eState = EMS_COMPLETED;
					break;
				}
			}
		}

		wstring wszMapName;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(info.pMap->,LocalizedName) )
			wszMapName = GET_TEXT_PRE(info.pMap->,LocalizedName);
		wstring wszTooltipTemplate;
		wstring wszMapStatus;
		bool bFinalLocked = bFinalMission && (target.eState == EMS_DISABLED);
		if ( IScreen *pScreen = GetScreen() )
		{
			wszTooltipTemplate = pScreen->GetTextEntry( !bFinalLocked ? "T_MISSION_TOOLTIP" : "T_MISSION_TOOLTIP_FINAL_LOCKED" );

			if ( target.eState == EMS_COMPLETED )
				wszMapStatus = pScreen->GetTextEntry( "T_MISSION_COMPLETED" );
			else if ( target.eState == EMS_ENABLED )
				wszMapStatus = pScreen->GetTextEntry( "T_MISSION_AVAILABLE" );
			else if ( target.eState == EMS_DISABLED )
				wszMapStatus = pScreen->GetTextEntry( "T_MISSION_UNAVAILABLE" );
		}
		vector< pair<wstring, wstring> > params;
		params.push_back( pair<wstring, wstring>( DYNAMIC_TAG_MISSION_NAME, wszMapName ) );
		params.push_back( pair<wstring, wstring>( DYNAMIC_TAG_MISSION_STATUS, wszMapStatus ) );
		if ( bFinalLocked )
		{
			IScenarioTracker *pST = Singleton<IScenarioTracker>();
			wstring wszCount = NStr::ToUnicode( StrFmt( "%d", pST->GetMissionToEnableCount() ) );
			params.push_back( pair<wstring, wstring>( DYNAMIC_TAG_FINAL_MISSION_LOCKS, wszCount ) );
		}
		SetDynamicTooltip( pElement, wszTooltipTemplate, params );

		// Add reward symbols
		int nSizeX, nSizeY;
		pElement->GetPlacement( &nX, &nY, &nSizeX, &nSizeY );
		nX += nSizeX / 2;
		nY += nSizeY;
		pBonusTemplate->GetPlacement( 0, 0, &nSizeX, &nSizeY );
		//nX -= ( nSizeX + 1 ) * info.reward.size() / 2;
		target.rewardDescs = info.reward;			// store dbptrs to bonus descs
		for ( int i = 0; i < info.reward.size(); ++i )
		{
			const NDb::SChapterBonus *pBonus = info.reward[i];

			NI_ASSERT( pBonus, "Null bonus for mission" );
			if ( !pBonus )
				continue;

			// Add symbol
			IButton *pRewardButton = dynamic_cast<IButton*>( AddWindowCopy( pChapterMapBgr, pBonusTemplate ) );
			if ( pRewardButton )
				pRewardButton->ShowWindow( true );
			else
				continue;
			wstring wszRewardPrefix = L"";
			if ( pRewardButton->DemandTooltip() )
			{
				CDynamicCast<CWindowTooltip> pTooltip = pRewardButton->DemandTooltip();
				if ( pTooltip )
					wszRewardPrefix = pTooltip->GetText();
			}
			pRewardButton->SetName( StrFmt( "Reward%d_%d", nIndex, i ) );
			// Position symbol
			pRewardButton->SetPlacement( nX, nY, 0, 0, EWPF_POS_X | EWPF_POS_Y );
			//nX += nSizeX + 1;
			// Set state
			IWindow *pReinfIcon = GetChildChecked<IWindow>( pRewardButton, "Icon", true );
			pRewardButton->SetPriority( 100 + i );
			NI_ASSERT( pReinfIcon, "Required part not found in mission bonus button" );
			switch ( pBonus->eBonusType )
			{
			case NDb::CBT_ADD_CALLS:
				{
					NI_ASSERT( 0, StrFmt( "Design: Illegal reward type CBT_ADD_CALLS for mission %d", nIndex ) );
					/*pRewardButton->SetState( 2 );
					pRewardButton->SetTextString( NStr::ToUnicode( StrFmt( "<center>%d", pBonus->nNumberOfCalls ) ) );
					pReinfIcon->ShowWindow( false );*/
				}
				break;

			case NDb::CBT_REINF_DISABLE:
				NI_ASSERT( 0, StrFmt( "Design: Illegal reward type CBT_REINF_DISABLE for mission %d", nIndex ) );
				/*pRewardButton->SetState( pBonus->bApplyToEnemy ? 1 : 0 );
				pReinfIcon->GetChild( 0 )->ShowWindow( true );			// Show cross-over
				for ( int j = 0; j < pUIC->reinfButtons.size(); ++j )
				{
					if ( pUIC->reinfButtons[j].eType == pBonus->eReinforcementType && pUIC->reinfButtons[j].pTexture )
					{
						pReinfIcon->SetTexture( pUIC->reinfButtons[j].pTexture );
						break;
					}
				}
				pReinfIcon->ShowWindow( true );*/
				break;

			case NDb::CBT_REINF_CHANGE:
				pRewardButton->SetState( pBonus->bApplyToEnemy ? 1 : 0 );
				if ( pBonus->pReinforcementSet->pIconTexture )
					pReinfIcon->SetTexture( pBonus->pReinforcementSet->pIconTexture );
				else
				{
					for ( int j = 0; j < pUIC->reinfButtons.size(); ++j )
					{
						if ( pUIC->reinfButtons[j].eType == pBonus->eReinforcementType && pUIC->reinfButtons[j].pTexture )
						{
							pReinfIcon->SetTexture( pUIC->reinfButtons[j].pTexture );
							break;
						}
					}
				}
				pReinfIcon->ShowWindow( true );
				pReinfIcon->SetTooltip( wszRewardPrefix + MakeTooltip( pBonus->pReinforcementSet ) );
				break;
			}

			pRewardButton->ShowWindow( false );

			int nRewardX = 0;
			int nRewardY = 0;
			if ( pRewardButton )
				pRewardButton->GetPlacement( &nRewardX, &nRewardY, 0, 0 );

			SReward reward;
			reward.pBtn = pRewardButton;
			reward.pBonusWnd = AddWindowCopy( pChapterMapBgr, pBonusGridBonusTemplate );
			if ( reward.pBonusWnd )
				reward.pBonusWnd->SetPlacement( nRewardX + vBonusGridBonusTemplateDelta.x, 
					nRewardY + vBonusGridBonusTemplateDelta.y, 0, 0, EWPF_POS_X | EWPF_POS_Y );
			target.rewards.push_back( reward );
			target.rewardDescs[i] = pBonus;
		}

		// Add to frontlines
		target.fValue0 = info.fPotentialIncomplete;
		target.fValue1 = info.fPotentialComplete;
		float fTargetValue;
		if ( target.eState == EMS_COMPLETED )
			fTargetValue = target.fValue1;
		else
			fTargetValue = target.fValue0;

		if ( pLastMission == info.pMap.GetPtr() && bIsMissionWon )
		{
			nFrontLineAnim = targets.size();														// Start front-line animation
			fTargetValue = target.fValue0;
			timeStartEffect = 0;
			nDelay = 3;
		}
		pFrontlines->SetNode( target.nX, target.nY, target.vEndOffset.x, target.vEndOffset.y, fTargetValue );

		targets.push_back( target );

		// Select correct button state
		SwitchTargetState( nIndex, false );
	}

	// Choose Recommended mission
	nRecommendedTarget = 0;				// Final is selected by default
	int nRecommendedValue = -1;
	for ( int i = 1; i < targets.size(); ++i )
	{
		if ( targets[i].eState == EMS_ENABLED && ( nRecommendedValue == -1 || targets[i].nPriority < nRecommendedValue ) )
		{
			nRecommendedTarget = i;
			nRecommendedValue = targets[i].nPriority;
		}
	}
	targets[nRecommendedTarget].eState = EMS_RECOMMENDED;
	SwitchTargetState( nRecommendedTarget, false );

	SelectTarget( nRecommendedTarget );
	pFrontlines->SetParams( pChapter->szSeaNoiseMask, pChapter->szDifferentColourMap, pHelper->vMainStrike, pChapter->nPositiveColour, pChapter->nNegativeColour );
}

void CInterfaceChapterMapMenu::EffectStart( EExitDestination eWhereTo )
{
	pScreen->Enable( false ); // CRAP

	eExitDir = eWhereTo;
	
	if ( eWhereTo == EED_RE_ENTER )
	{
		if ( pChapterMapLeft )
			pChapterMapLeft->SetPlacement( nChapterMapLeftX, nChapterMapLeftY, 0, 0, EWPF_POS_X | EWPF_POS_Y );
		if ( pChapterMapRight )
			pChapterMapRight->SetPlacement( nChapterMapRightX, nChapterMapRightY, 0, 0, EWPF_POS_X | EWPF_POS_Y );
	}

	bool bForward = ( eWhereTo == EED_ENTER || eWhereTo == EED_RE_ENTER );

	//CRAP - to have access to subscreens
	if ( eWhereTo != EED_ENTER && eWhereTo != EED_BACK )
	{
		if ( eWhereTo != EED_ARMY_MANAGER && eWhereTo != EED_PLAYER_INFO && eWhereTo != EED_RE_ENTER )
		{
			nEffectCounter = 1;
			OnEffectFinish();
			return;
		}
	}

	CPtr<SWindowContextB2Move> pContext = new SWindowContextB2Move();
	pContext->bGoOut = !bForward;
	pContext->fMaxMoveTime = 0.0f;
	nEffectCounter = 2;
	GetScreen()->RunStateCommandSequience( "effect_on_left", pScreen, pContext, true );
	GetScreen()->RunStateCommandSequience( "effect_on_right", pScreen, pContext, true );

	//{ CRAP
	if ( eWhereTo == EED_RE_ENTER )
		pScreen->Enable( true );
	//}

	bNeedToRunAnimation = false;
}

void CInterfaceChapterMapMenu::OnEffectFinish()
{
	--nEffectCounter;
	if ( nEffectCounter > 0 )
		return;

	pScreen->Enable( true );

	switch ( eExitDir )
	{
	case EED_RE_ENTER:
		// Do nothing?
		break;
	case EED_ENTER:
		{
			int nOldCalls = Singleton<IScenarioTracker>()->GetReinforcementCallsOld();
			PlayReinfRollerAnim( nOldCalls, nCallsLeft - nCurrentMissionReinfs );

			if ( Singleton<IScenarioTracker>()->IsMissionWon() )
			{
				const NDb::SMapInfo *pLastMission = Singleton<IScenarioTracker>()->GetLastMission();
				for ( int i = 0; i < targets.size(); ++i )
				{
					STarget &target = targets[i];

					if ( target.pDBInfo == pLastMission )
					{
						nFrontLineAnim = i;														// Start front-line animation
						timeStartEffect = 0;
						nDelay = 1;

						break;
					}
				}
			}

			ProceedInitialDialogs();

			break;
		}
	case EED_BACK:
		{
			IScenarioTracker *pST = Singleton<IScenarioTracker>();
			bool bIsTutorial = (pST != 0 ) ? pST->IsTutorialCampaign() : false;
			bool bIsCustom = (pST != 0 ) ? pST->IsCustomCampaign() : false;

			InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

			NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
			//	NMainLoop::Command( ML_COMMAND_MISSION_BACKGROUND, "" );
			if ( bIsTutorial )
				NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
			else if ( bIsCustom )
				NMainLoop::Command( ML_COMMAND_CUSTOM_CAMPAIGN, "" );
			else
			{
				NMainLoop::Command( ML_COMMAND_CAMPAIGN_SELECTION_MENU, "" );

				// music
				const NDb::SGameRoot *pGameRoot = InterfaceState()->GetGameRoot();
				if ( pGameRoot && pGameRoot->pMainMenuMusic )
					Singleton<IMusicSystem>()->Init( pGameRoot->pMainMenuMusic, 0 );
			}

			break;
		}
	case EED_ARMY_MANAGER:
		{
			NMainLoop::Command( ML_COMMAND_ARMY_SCREEN, "" );

			break;
		}
	case EED_SAVE:
		{
			NMainLoop::Command( ML_COMMAND_HIDE_UNFOCUSED_SCREEN, "" );
			NMainLoop::Command( ML_COMMAND_SAVE_LOAD_MENU, "save_from_main_menu" );
			break;
		}
	case EED_PLAYER_INFO:
		{
			NMainLoop::Command( ML_COMMAND_PLAYER_STATS, "" );
			break;
		}
	}
}


