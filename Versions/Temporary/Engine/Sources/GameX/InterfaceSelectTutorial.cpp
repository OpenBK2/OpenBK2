#include "StdAfx.h"
#include "InterfaceSelectTutorial.h"
#include "GameXClassIDs.h"
#include "../UISpecificB2/UISpecificB2.h"
#include "DBGameRoot.h"
#include "../System/Text.h"
#include "GetConsts.h"
#include "../Stats_B2_M1/DbMapInfo.h"
#include "InterfaceState.h"
#include "ScenarioTracker.h"

// CInterfaceSelectTutorial

CInterfaceSelectTutorial::CInterfaceSelectTutorial() :
	CInterfaceScreenBase( "SelectTutorial", "select_tutorial_menu" )
{
}

bool CInterfaceSelectTutorial::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );

	MakeInterior();
	
	return true;
}

void CInterfaceSelectTutorial::MakeInterior()
{
	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_SINGLE );

	pMainWnd = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	pMissionsPanel = GetChildChecked<IWindow>( pMainWnd, "LeftPanel", true );
	pMinimapPanel = GetChildChecked<IWindow>( pMainWnd, "MinimapPanel", true );
	pMissionDescPanel = GetChildChecked<IWindow>( pMainWnd, "MissionDescPanel", true );
	
	pPlayBtn = GetChildChecked<IButton>( pMainWnd, "PlayBtn", true );
	if ( pPlayBtn )
		pPlayBtn->Enable( false );

	pMissionsCont = GetChildChecked<IScrollableContainer>( pMissionsPanel, "MissionsListCont", true );
	pMissionsItemWnd = GetChildChecked<IWindow>( pMissionsCont, "MissionsListItem", true );
	if ( pMissionsItemWnd )
		pMissionsItemWnd->ShowWindow( false );
	
	pMinimap = GetChildChecked<IMiniMap>( pMinimapPanel, "Minimap", true );

	pMissionNameView = GetChildChecked<ITextView>( pMissionDescPanel, "NameView", true );
	pMissionSeasonView = GetChildChecked<ITextView>( pMissionDescPanel, "SeasonView", true );
	pMissionDescCont = GetChildChecked<IScrollableContainer>( pMissionDescPanel, "MissionDescCont", true );
	pMissionDescView = GetChildChecked<ITextView>( pMissionDescPanel, "MissionDescView", true );
	pMissionDescItemBlock = GetChildChecked<IWindow>( pMissionDescPanel, "MissionDescItemBlock", true );
	if ( pMissionDescCont )
	{
		if ( pMissionDescItemBlock )
			pMissionDescCont->PushBack( pMissionDescItemBlock, false );
		if ( pMissionDescView )
			pMissionDescCont->PushBack( pMissionDescView, false );
	}
	
	const NDb::SGameRoot *pGameRoot = NGameX::GetGameRoot();
	if ( pGameRoot )
	{
		for ( int i = 0; i < pGameRoot->tutorialMaps.size(); ++i )
		{
			const NDb::SGameRoot::STutorialMap &dbMap = pGameRoot->tutorialMaps[i];
			if ( !dbMap.pMapInfo )
				continue;

			SMapInfo mapInfo;
			mapInfo.pMapInfo = dbMap.pMapInfo;
			if ( CHECK_TEXT_NOT_EMPTY_PRE(dbMap.,Difficulty) )
				mapInfo.wszDifficulty = GET_TEXT_PRE(dbMap.,Difficulty);
			maps.push_back( mapInfo );
		}
	}
	
	if ( pMissionsCont && pMissionsItemWnd && !maps.empty() )
	{
		for ( int i = 0; i < maps.size(); ++i )
		{
			SMapInfo &mapInfo = maps[i];

			mapInfo.pWnd = AddWindowCopy( pMissionsCont, pMissionsItemWnd );
			mapInfo.pNameView = GetChildChecked<ITextView>( mapInfo.pWnd, "NameView", true );
			mapInfo.pDifficultyView = GetChildChecked<ITextView>( mapInfo.pWnd, "Difficulty", true );

			wstring wszName;
			if ( CHECK_TEXT_NOT_EMPTY_PRE(mapInfo.pMapInfo->,LocalizedName) )
				wszName = GET_TEXT_PRE(mapInfo.pMapInfo->,LocalizedName);
			if ( mapInfo.pNameView )
				mapInfo.pNameView->SetText( mapInfo.pNameView->GetDBText() + wszName );

			if ( mapInfo.pDifficultyView )
				mapInfo.pDifficultyView->SetText( mapInfo.pDifficultyView->GetDBText() + mapInfo.wszDifficulty );

			if ( mapInfo.pWnd )
				mapInfo.pWnd->ShowWindow( true );
			pMissionsCont->PushBack( mapInfo.pWnd, true );
		}
		if ( pPlayBtn )
			pPlayBtn->Enable( true );
	}

	if ( pMainWnd )
		SetMainWindowTexture( pMainWnd, InterfaceState()->GetMenuBackgroundTexture() );

	int nRecommendedMission = InterfaceState()->GetTutorialRecommendedMission();
	if ( nRecommendedMission >= maps.size() )
		nRecommendedMission = -1;
	if ( nRecommendedMission < 0 )
		nRecommendedMission = 0;

	Select( nRecommendedMission );
}

bool CInterfaceSelectTutorial::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "menu_play" )
		return OnPlay();
	if ( szReaction == "menu_back" )
		return OnBack();
	if ( szReaction == "on_select" )
		return OnSelect();

	return false;
}

int CInterfaceSelectTutorial::Check( const string &szCheckName ) const
{
	return 0;
}

bool CInterfaceSelectTutorial::OnPlay()
{
	IWindow *pWnd = pMissionsCont->GetSelectedItem();
	int nIndex = pWnd ? pMissionsCont->GetItemNumber( pWnd ) : -1;
	if ( nIndex >= 0 && nIndex < maps.size() )
	{
		SMapInfo &mapInfo = maps[nIndex];
		
		InterfaceState()->MarkTutorialRecommendedMission( nIndex + 1 );
		Singleton<IScenarioTracker>()->CustomMissionStart( mapInfo.pMapInfo, 0, true );
		
		NMainLoop::Command( ML_COMMAND_MISSION_BRIEFING, "" );
	}

	return true;
}

bool CInterfaceSelectTutorial::OnBack()
{
	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_NONE );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "single_player_submenu" );

	return true;
}

bool CInterfaceSelectTutorial::OnSelect()
{
	if ( pMissionsCont )
	{
		IWindow *pWnd = pMissionsCont->GetSelectedItem();
		int nIndex = pWnd ? pMissionsCont->GetItemNumber( pWnd ) : -1;
		if ( nIndex >= 0 && nIndex < maps.size() )
		{
			SMapInfo &mapInfo = maps[nIndex];

			if ( pMinimap )
			{
				float x = mapInfo.pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE;
				float y = mapInfo.pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE;

				const NDb::STexture *pTexture = 0;
				if ( mapInfo.pMapInfo->pMiniMap )
					pTexture = mapInfo.pMapInfo->pMiniMap->pTexture;

				pMinimap->SetLoadingMapParams( x, y );
				pMinimap->SetTexture( pTexture );
			}

			wstring wszName;
			if ( CHECK_TEXT_NOT_EMPTY_PRE(mapInfo.pMapInfo->,LocalizedName) )
				wszName = GET_TEXT_PRE(mapInfo.pMapInfo->,LocalizedName);
			if ( pMissionNameView )
				pMissionNameView->SetText( pMissionNameView->GetDBText() + wszName );

			wstring wszSeason = InterfaceState()->GetSeasonName( mapInfo.pMapInfo->eSeason );
			if ( pMissionSeasonView )
				pMissionSeasonView->SetText( pMissionSeasonView->GetDBText() + wszSeason );

			wstring wszDesc;
			if ( CHECK_TEXT_NOT_EMPTY_PRE(mapInfo.pMapInfo->,LoadingDescription) )
				wszDesc = GET_TEXT_PRE(mapInfo.pMapInfo->,LoadingDescription);
			if ( pMissionDescView )
				pMissionDescView->SetText( pMissionDescView->GetDBText() + wszDesc );
			if ( pMissionDescCont )
				pMissionDescCont->Update();
		}
	}
		
	return true;
}

bool CInterfaceSelectTutorial::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive );
}


void CInterfaceSelectTutorial::Select( int nIndex )
{
	if ( nIndex < 0 || nIndex >= maps.size() )
		return;

	SMapInfo &mapInfo = maps[nIndex];
	if ( pMissionsCont )
		pMissionsCont->Select( mapInfo.pWnd );
}

// CICSelectTutorial

void CICSelectTutorial::PreCreate()
{
}

void CICSelectTutorial::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICSelectTutorial::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x17255300, CInterfaceSelectTutorial )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_SELECT_TUTORIAL, CICSelectTutorial )


