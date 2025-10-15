#include "stdafx.h"
#include "InterfacePlayerStats.h"
#include "GameXClassIDs.h"
#include "Main/Profiles.h"
#include "Misc/StrProc.h"
#include "ScenarioTracker.h"
#include "System/Text.h"
#include "InterfaceState.h"

#include "GameX_export.h"

// CInterfacePlayerStats

CInterfacePlayerStats::CInterfacePlayerStats() :
	CInterfaceScreenBase( "PlayerStats", "player_stats_menu" )
{
}

bool CInterfacePlayerStats::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );

	MakeInterior();
	
	return true;
}

void CInterfacePlayerStats::InitInteriorView()
{
	pMain = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	pHeaderPanel = GetChildChecked<IWindow>( pMain, "HeaderPanel", true );
	pBottomPanel = GetChildChecked<IWindow>( pMain, "BottomPanel", true );
	pPlayerInfoPanel = GetChildChecked<IWindow>( pMain, "PlayerInfoPanel", true );
	pMedalInfoPanel = GetChildChecked<IWindow>( pMain, "MedalInfoPanel", true );
	pMedalListPanel = GetChildChecked<IWindow>( pMain, "MedalListPanel", true );
	
	pPlayerNameView = GetChildChecked<ITextView>( pPlayerInfoPanel, "PlayerNameView", true );
	pPlayerRankPicture = GetChildChecked<IWindow>( pPlayerInfoPanel, "PlayerRankPicture", true );
	pUnitsKilledView = GetChildChecked<ITextView>( pPlayerInfoPanel, "KilledView", true );
	pUnitsLostView = GetChildChecked<ITextView>( pPlayerInfoPanel, "LostView", true );
	pInGameTimeView = GetChildChecked<ITextView>( pPlayerInfoPanel, "InGameTimeView", true );
	pFavoriteReinfView = GetChildChecked<ITextView>( pPlayerInfoPanel, "FavoriteReinforcementsView", true );
	pRankNameView = GetChildChecked<ITextView>( pPlayerInfoPanel, "RankNameView", true );
	pBaseCareerBar = GetChildChecked<IProgressBar>( pPlayerInfoPanel, "BaseCareerBar", true );
	pNewCareerBar = GetChildChecked<IProgressBar>( pPlayerInfoPanel, "NewCareerBar", true );
	pBaseNextRankBar = GetChildChecked<IProgressBar>( pPlayerInfoPanel, "BaseNextRankBar", true );
	pNewNextRankBar = GetChildChecked<IProgressBar>( pPlayerInfoPanel, "NewNextRankBar", true );
	pNextRankProgressView = GetChildChecked<ITextView>( pPlayerInfoPanel, "NextRankProgressView", true );

	pMedalNameView = GetChildChecked<ITextView>( pMedalInfoPanel, "MedalNameView", true );
	pMedalPicture = GetChildChecked<IWindow>( pMedalInfoPanel, "MedalPicture", true );
	pMedalDescView = GetChildChecked<ITextView>( pMedalInfoPanel, "MedalDescView", true );

	pMedalCont = GetChildChecked<IScrollableContainer>( pMedalListPanel, "MedalCont", true );
	pMedalItem = GetChildChecked<IWindow>( pMedalListPanel, "MedalListItem", true );
	if ( pMedalItem )
		pMedalItem->ShowWindow( false );
}

void CInterfacePlayerStats::MakeInterior()
{
	InitInteriorView();
	
	if ( pPlayerNameView )
		pPlayerNameView->SetText( pPlayerNameView->GetDBText() + NProfile::GetCurrentProfileName() );

	const int nLocalPlayer = 0;
	IScenarioTracker *pST = Singleton<IScenarioTracker>();

	const NDb::SPlayerRank *pRank = pST->GetPlayerRank();
	if ( pRank )
	{
		if ( pPlayerRankPicture )
			pPlayerRankPicture->SetTexture( pRank->pStrap );

		std::wstring wszRank;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pRank->,RankName) )
			wszRank = GET_TEXT_PRE(pRank->,RankName);
		if ( pRankNameView )
			pRankNameView->SetText( pRankNameView->GetDBText() + wszRank );
	}

	int nKilled = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_CAMPAIGN_UNITS_KILLED );
	int nLost = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_CAMPAIGN_UNITS_LOST );
	if ( pUnitsKilledView )
		pUnitsKilledView->SetText( pUnitsKilledView->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nKilled ) ) );
	if ( pUnitsLostView )
		pUnitsLostView->SetText( pUnitsLostView->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nLost ) ) );

	int nMissionsPassed = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_CAMPAIGN_MISSIONS_PASSED );
	if ( pMissionsPassedView )
		pMissionsPassedView->SetText( pMissionsPassedView->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nMissionsPassed ) ) );

	int nInGameTime = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_CAMPAIGN_TIME );
	int nSec = nInGameTime % 60;
	nInGameTime = nInGameTime / 60;
	int nMin = nInGameTime % 60;
	nInGameTime = nInGameTime / 60;
	int nHour = nInGameTime;
//	wstring wszInGameTime = NStr::ToUnicode( StrFmt( "%2d : %02d : %02d", nHour, nMin, nSec ) );
	std::wstring wszInGameTime;
	if ( IScreen *pScreen = GetScreen() )
	{
		wszInGameTime = NStr::ToUnicode( StrFmt( "%2d", nHour ) );
		wszInGameTime += pScreen->GetTextEntry( "T_HOUR" );
		wszInGameTime += NStr::ToUnicode( StrFmt( "%2d", nMin ) );
		wszInGameTime += pScreen->GetTextEntry( "T_MINUTE" );
	}
	if ( pInGameTimeView )
		pInGameTimeView->SetText( pInGameTimeView->GetDBText() + wszInGameTime );

	pST->GetLastVisiblePlayerStatsExp( &fLastVisiblePlayerStatsExpCareer, &fLastVisiblePlayerStatsExpNextRank );

	expProgressCareer.Init();
	expProgressRank.Init();
	
	expProgressCareer.fStart = (std::min)( expProgressCareer.fTarget, (std::max)( expProgressCareer.fStart, fLastVisiblePlayerStatsExpCareer ) );
	expProgressCareer.fCur = (std::max)( expProgressCareer.fCur, expProgressCareer.fStart );
	expProgressRank.fStart = (std::min)( expProgressRank.fTarget, (std::max)( expProgressRank.fStart, fLastVisiblePlayerStatsExpNextRank ) );
	expProgressRank.fCur = (std::max)( expProgressRank.fCur, expProgressRank.fStart );

	ExpProgressStep();

	std::wstring wszRankHP = NStr::ToUnicode( StrFmt( "%d/%d",
		(int)( expProgressRank.fTarget - expProgressRank.fTargetLevelExp ), 
		(int)( expProgressRank.fTargetNextLevelExp - expProgressRank.fTargetLevelExp ) ) );
	if ( pNextRankProgressView )
		pNextRankProgressView->SetText( pNextRankProgressView->GetDBText() + wszRankHP );

	std::vector<const IScenarioTracker::SMissionStats*> missions;
	pST->GetAllMissionStats( &missions );
	for ( int i = 0; i < missions.size(); ++i )
	{
		const IScenarioTracker::SMissionStats *pMission = missions[i];
		
		for ( std::list< CDBPtr<NDb::SMedal> >::const_iterator it = pMission->medals.begin(); it != pMission->medals.end(); ++it )
		{
			const NDb::SMedal *pMedal = *it;
			if ( !pMedal )	
				continue;

			medals.push_back( SMedal() );
			SMedal &medal = medals.back();

			if ( CHECK_TEXT_NOT_EMPTY_PRE(pMedal->,LocalizedName) )
				medal.wszName = GET_TEXT_PRE(pMedal->,LocalizedName);
			if ( CHECK_TEXT_NOT_EMPTY_PRE(pMedal->,LocalizedDesc) )
				medal.wszDesc = GET_TEXT_PRE(pMedal->,LocalizedDesc);
			medal.pIcon = pMedal->pIconTexture;
			medal.pPicture = pMedal->pPictureTexture;
			
			IWindow *pWnd = AddWindowCopy( pMedalCont, pMedalItem );
			if ( !pWnd )
				continue;

			if ( pMedalCont )
				pMedalCont->PushBack( pWnd, true );
			
			medal.pWnd = pWnd;
			medal.pIconWnd = GetChildChecked<IWindow>( pWnd, "Icon", true );
			medal.pNameView = GetChildChecked<ITextView>( pWnd, "Name", true );

			if ( medal.pWnd )
				medal.pWnd->ShowWindow( true );
			if ( medal.pIconWnd )
				medal.pIconWnd->SetTexture( medal.pIcon );
			if ( medal.pNameView )
				medal.pNameView->SetText( medal.pNameView->GetDBText() + medal.wszName );
		}
	}
	
	std::wstring wszFavoriteReinf;
	if ( IScreen *pScreen = GetScreen() )
		wszFavoriteReinf = pScreen->GetTextEntry( "T_NO_FAVORITE_REINF" );
	NDb::EReinforcementType eType = pST->GetFavoriteReinf();
	if ( eType != NDb::_RT_NONE )
		wszFavoriteReinf = pST->GetReinfName( eType );
	if ( pFavoriteReinfView )
		pFavoriteReinfView->SetText( pFavoriteReinfView->GetDBText() + wszFavoriteReinf );

	if ( GetScreen() )
		GetScreen()->SetTexture( InterfaceState()->GetMenuBackgroundTexture() );
	
	SelectMedal( 0, true );
}

void CInterfacePlayerStats::ExpProgressStep()
{
	if ( !expProgressCareer.bFinal )
	{
		expProgressCareer.Step( false );
		if ( pBaseCareerBar )
			pBaseCareerBar->SetPosition( expProgressCareer.fProgress );
		if ( pNewCareerBar )
			pNewCareerBar->SetPosition( expProgressCareer.fNewProgress );
	}
	
	if ( !expProgressRank.bFinal )
	{
		expProgressRank.Step( false );
		if ( pBaseNextRankBar )
			pBaseNextRankBar->SetPosition( expProgressRank.fProgress );
		if ( pNewNextRankBar )
			pNewNextRankBar->SetPosition( expProgressRank.fNewProgress );
	}
	
	fLastVisiblePlayerStatsExpCareer = expProgressCareer.fCur;
	fLastVisiblePlayerStatsExpNextRank = expProgressRank.fCur;
}

bool CInterfacePlayerStats::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );

	ExpProgressStep();

	return bResult;
}

bool CInterfacePlayerStats::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "menu_back" )
		return OnMenuBack();
	if ( szReaction == "select_medal" )
		return OnSelectMedal();

	return false;
}

int CInterfacePlayerStats::Check( const std::string &szCheckName ) const
{
	return 0;
}

bool CInterfacePlayerStats::OnMenuBack()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	pST->SetLastVisiblePlayerStatsExp( fLastVisiblePlayerStatsExpCareer, fLastVisiblePlayerStatsExpNextRank );

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NInput::PostEvent( "menu_return_from_subscreen", 0, 0 );
	
	return true;
}

bool CInterfacePlayerStats::OnSelectMedal()
{
	if ( pMedalCont )
	{
		IWindow *pWnd = pMedalCont->GetSelectedItem();
		SelectMedal( pMedalCont->GetItemNumber( pWnd ), false );
	}
	return true;
}

void CInterfacePlayerStats::SelectMedal( int nIndex, bool bForce )
{
	if ( pMedalNameView )
		pMedalNameView->ShowWindow( false );
	if ( pMedalPicture )
		pMedalPicture->ShowWindow( false );
	if ( pMedalDescView )
		pMedalDescView->ShowWindow( false );

	if ( nIndex < 0 || nIndex >= medals.size() )
		return;

	SMedal &medal = medals[nIndex];
	
	if ( bForce )
	{
		if ( pMedalCont )
			pMedalCont->Select( medal.pWnd );
		return;
	}

	if ( pMedalNameView )
	{
		pMedalNameView->ShowWindow( true );
		pMedalNameView->SetText( pMedalNameView->GetDBText() + medal.wszName );
	}
	if ( pMedalPicture )
	{
		pMedalPicture->ShowWindow( true );
		pMedalPicture->SetTexture( medal.pPicture );
	}
	if ( pMedalDescView )
	{
		pMedalDescView->ShowWindow( true );
		pMedalDescView->SetText( pMedalDescView->GetDBText() + medal.wszDesc );
	}
}

// CICPlayerStats

void CICPlayerStats::PreCreate()
{
}

void CICPlayerStats::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICPlayerStats::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x1722A301, CInterfacePlayerStats )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_PLAYER_STATS, CICPlayerStats )


