#include "StdAfx.h"
#include "InterfaceMissionBriefing.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "DBGameRoot.h"
#include "ScenarioTracker.h"
#include "Misc/StrProc.h"
#include "System/Text.h"

const float TEXTURE_POINT_X = 4.0f; 
const float TEXTURE_POINT_Y = 7.0f;

// CInterfaceMissionBriefing

CInterfaceMissionBriefing::CInterfaceMissionBriefing() : 
	CInterfaceScreenBase( "MissionBriefing", "mission_briefing" )
{
}

bool CInterfaceMissionBriefing::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );
	
	MakeInterior();

	return true;
}

void CInterfaceMissionBriefing::MakeInterior()
{
	pMapInfo = Singleton<IScenarioTracker>()->GetCurrentMission();

	pMainWnd = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	pTopPanel = GetChildChecked<IWindow>( pMainWnd, "TopPanel", true );
	pBottomPanel = GetChildChecked<IWindow>( pMainWnd, "BottomPanel", true );
	pMissionDescPanel = GetChildChecked<IWindow>( pMainWnd, "LeftPanel", true );
	pMinimapPanel = GetChildChecked<IWindow>( pMainWnd, "RightTopPanel", true );
	pObjectivesListPanel = GetChildChecked<IWindow>( pMainWnd, "RightBottomPanel", true );
	
	pHeaderView = GetChildChecked<ITextView>( pTopPanel, "HeaderView", true );
	
	if ( pMapInfo )
	{
		wstring wszName;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pMapInfo->,LocalizedName) )
			wszName = GET_TEXT_PRE(pMapInfo->,LocalizedName);
		if ( wszName.empty() )
		{
			if ( IScreen *pScreen = GetScreen() )
				wszName = pScreen->GetTextEntry( "TEXT_DEFAULT_HEADER" );
		}
		if ( pHeaderView )
			pHeaderView->SetText( pHeaderView->GetDBText() + wszName );
	}
	
	pMissionDescCont = GetChildChecked<IScrollableContainer>( pMissionDescPanel, "MissionDescCont", true );
	pMissionDescView = GetChildChecked<ITextView>( pMissionDescPanel, "MissionDescView", true );

	if ( pMapInfo )
	{
		wstring wszMissionDesc;
		if ( pMapInfo && CHECK_TEXT_NOT_EMPTY_PRE(pMapInfo->,LoadingDescription) )
			wszMissionDesc = GET_TEXT_PRE(pMapInfo->,LoadingDescription);
		if ( pMissionDescView )
			pMissionDescView->SetText( pMissionDescView->GetDBText() + wszMissionDesc );
	}
	
	if ( pMissionDescCont && pMissionDescView )
		pMissionDescCont->PushBack( pMissionDescView, false );
	
	pMinimap = GetChildChecked<IMiniMap>( pMinimapPanel, "Minimap", true );

	pObjectivesListCont = GetChildChecked<IScrollableContainer>( pObjectivesListPanel, "ObjectivesListCont", true );
	pObjectivesView = GetChildChecked<ITextView>( pObjectivesListPanel, "ObjectivesListView", true );
	
	if ( pMapInfo )
	{
		wstring wszDesc;
		if ( pMapInfo && CHECK_TEXT_NOT_EMPTY_PRE(pMapInfo->,LocalizedDescription) )
			wszDesc = GET_TEXT_PRE(pMapInfo->,LocalizedDescription);
		if ( pObjectivesView )
			pObjectivesView->SetText( pObjectivesView->GetDBText() + wszDesc );
	}
	
	if ( pObjectivesListCont && pObjectivesView )
		pObjectivesListCont->PushBack( pObjectivesView, false );

	if ( pMinimap && pMapInfo )
	{
		float x = pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE;
		float y = pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE;

		const NDb::STexture *pTexture = 0;
		if ( pMapInfo->pMiniMap )
			pTexture = pMapInfo->pMiniMap->pTexture;

		pMinimap->SetLoadingMapParams( x, y );
		pMinimap->SetTexture( pTexture );
	}
	
	if ( pMainWnd )
		SetMainWindowTexture( pMainWnd, InterfaceState()->GetMenuBackgroundTexture() );
}

bool CInterfaceMissionBriefing::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "menu_back" )
		return OnBack();
	if ( szReaction == "menu_play" )
		return OnPlay();

	return false;
}

int CInterfaceMissionBriefing::Check( const string &szCheckName ) const
{
	return 0;
}

bool CInterfaceMissionBriefing::OnBack()
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );

	return true;
}

bool CInterfaceMissionBriefing::OnPlay()
{
	MissionStart();

	return true;
}

void CInterfaceMissionBriefing::MissionStart()
{
	if ( !pMapInfo )
		return;

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );

	NMainLoop::Command( ML_COMMAND_MISSION, StrFmt( "%s;normal", pMapInfo->GetDBID().ToString().c_str() ) );
}

// CICMissionBriefing

void CICMissionBriefing::PreCreate()
{
}

void CICMissionBriefing::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMissionBriefing::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x17264481, CInterfaceMissionBriefing )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MISSION_BRIEFING, CICMissionBriefing )


