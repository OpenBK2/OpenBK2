#include "stdafx.h"

#include "InterfaceMPLadderStatistics.h"
#include "InterfaceMPBase.h"
#include "GameXClassIDs.h"
#include "MultiplayerCommandManager.h"
#include "Misc/StrProc.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "System/Text.h"
#include "DBScenario.h"

// CInterfaceMPLadderStatistics

CInterfaceMPLadderStatistics::CInterfaceMPLadderStatistics() :
CInterfaceScreenBase( "MPLadderStatistics", "ladder_statistics" )
{
}

CInterfaceMPLadderStatistics::~CInterfaceMPLadderStatistics()
{
}

bool CInterfaceMPLadderStatistics::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	AddScreen( this );
	pMain = GetChildChecked<IWindow>( pScreen, "Main", true );
	NI_VERIFY( pMain, "No main window found", return false );

	IInterfaceBase *pPrevInterface = NMainLoop::GetPrevInterface( this );
	if ( pPrevInterface )
		pPrevMPScreen = dynamic_cast<CInterfaceMPScreenBase*>( pPrevInterface );

	pWaiting = GetChildChecked<ITextView>( pMain, "WaitingLine", true );
	pAdvList = GetChildChecked<IScrollableContainer>( pMain, "RightPanel", true );
	pAdv4Template = GetChildChecked<IWindow>( pAdvList, "Item4Template", true );
	pAdv2Template = GetChildChecked<IWindow>( pAdvList, "Item2Template", true );
	pAdv1Template = GetChildChecked<ITextView>( pAdvList, "Item1Template", true );
	pMedalPopup = GetChildChecked<IWindow>( pMain, "MedalPopup", true );

	return true;
}

bool CInterfaceMPLadderStatistics::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "react_on_back" )
		return OnBackReaction();

	if ( szReaction == "react_on_medal" )
		return OnShowMedal( szSender );

	if ( szReaction == "react_on_close" )
	{
		pMedalPopup->ShowWindow( false );
		return true;
	}

	return false;
}

int CInterfaceMPLadderStatistics::Check( const string &szCheckName ) const
{
	return 0;
}

bool CInterfaceMPLadderStatistics::StepLocal( bool bAppActive )
{
	// Check MP messages
	Singleton<IMPToUIManager>()->MPUISegment();

	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );

	if ( bAppActive )
	{
		while ( 1 ) 
		{
			CPtr< SMPUIMessage> pMsg = Singleton<IMPToUIManager>()->GetUIMessage();
			if ( !pMsg )
				break;
			if ( pMsg->eMessageType == EMUI_NIVAL_NET_LADDER_STATS )
			{
				SMPUILadderStatsMessage *pStatsMsg = dynamic_cast_ptr<SMPUILadderStatsMessage*>( pMsg );
				if ( pStatsMsg )
					OnLadderStatsMessage( pStatsMsg );
			}
			else if ( pPrevMPScreen )
				pPrevMPScreen->HandleMessage( pMsg );
		}
	}

	if ( pPrevMPScreen )
		pPrevMPScreen->Step( bAppActive );
	return bResult;
}

bool CInterfaceMPLadderStatistics::OnBackReaction()
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	return true;
}

void CInterfaceMPLadderStatistics::RequestInfo( const string &szNick )
{
	Singleton<IMPToUIManager>()->AddUIMessage( new SMPUILadderInfoRequestMessage( szNick, false ) );
	ITextView *pUserName = GetChildChecked<ITextView>( pMain, "UserName", true );
	pUserName->SetText( pUserName->GetDBText() + NStr::ToUnicode( szNick ) );

	// Show "Waiting"
	pWaiting->ShowWindow( true );
}

void CInterfaceMPLadderStatistics::OnLadderStatsMessage( struct SMPUILadderStatsMessage *pMsg )
{
	SLadderStatistics &info = pMsg->info;
	const NDb::SMultiplayerConsts *pMPConsts = NGameX::GetMPConsts();
	countryNames.resize( pMPConsts->sides.size() );

	////////////////////////////////////////// Initial counting
	int nWins = 0;
	int nLosses = 0;
	for ( int i = 0; i < countryNames.size(); ++i )
	{
		nWins += info.raceWinsSolo[i] + info.raceWinsTeam[i];
		nLosses += info.raceLossesSolo[i] + info.raceLossesTeam[i];
		countryNames[i] = GET_TEXT_PRE( pMPConsts->sides[i]., Name );
	}
	int nPlayMinutes = info.nTotalPlayTime / 60;
	int nPlayHours = nPlayMinutes / 60;
	int nPlayDays = nPlayHours / 24;
	nPlayHours %= 24;
	nPlayMinutes %= 60;

	//////////////////////////////////////////////// Info Panel
	IWindow *pPicture = GetChildChecked<IWindow>( pMain, "RankPicture", true );
	ITextView *pXP = GetChildChecked<ITextView>( pMain, "InfoXP", true );
	IProgressBar *pXPProgress = GetChildChecked<IProgressBar>( pMain, "InfoXPProgress", true );
	ITextView *pLevel = GetChildChecked<ITextView>( pMain, "InfoLevel", true );
	ITextView *pRank = GetChildChecked<ITextView>( pMain, "InfoRank", true );
	ITextView *pBattles = GetChildChecked<ITextView>( pMain, "InfoTotalBattles", true );
	ITextView *pWinLose = GetChildChecked<ITextView>( pMain, "InfoWinLose", true );
	ITextView *pGameTime = GetChildChecked<ITextView>( pMain, "InfoGameTime", true );

	pPicture->SetTexture( pMPConsts->sides[info.nRace].ladderRanks[pMsg->nRank].pTexture );
	pXP->SetText( pXP->GetDBText() + NStr::ToUnicode( StrFmt( "%d/%d", info.nXP, info.nNextLevelXP ) ) );
	float fPBPos = info.nXP - info.nLevelXP;
	fPBPos = ( info.nNextLevelXP - info.nLevelXP > 0 ) ? ( fPBPos / ( info.nNextLevelXP - info.nLevelXP ) ) : 1.0f;
	pXPProgress->SetPosition( fPBPos );
	pLevel->SetText( pLevel->GetDBText() + NStr::ToUnicode( StrFmt( "%d", info.nLevel ) ) );
	pRank->SetText( pLevel->GetDBText() + GET_TEXT_PRE( pMPConsts->sides[info.nRace].ladderRanks[pMsg->nRank]., Name ) );
	pBattles->SetText( pBattles->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nWins + nLosses ) ) );
	pWinLose->SetText( pWinLose->GetDBText() + NStr::ToUnicode( StrFmt( "%d/%d", nWins, nLosses ) ) );
	pGameTime->SetText( pGameTime->GetDBText() + NStr::ToUnicode( StrFmt( "%d:%02d:%02d", nPlayDays, nPlayHours, nPlayMinutes ) ) );

	//////////////////////////////////////////// Medals Panel
	IScrollableContainer *pMedalsList = GetChildChecked<IScrollableContainer>( pMain, "MedalsList", true );
	IWindow *pMedalTemplate = GetChildChecked<IWindow>( pMedalsList, "ItemMedalTemplate", true );
	IWindow *pMedalCountryTemplate = GetChildChecked<IWindow>( pMedalsList, "ItemCountryTemplate", true );
	int nMedalCounter = 1;
	for ( int nCountry = 0; nCountry < countryNames.size(); ++nCountry )
	{
		const NDb::SMultiplayerSide &side = pMPConsts->sides[nCountry];
		bool bHeaderAdded = false;
		for ( int i = 0; i < 16; ++i )
		{
			if ( !SLadderStatistics::HasMedal( info.medals, nCountry, i ) )
				continue;

			if ( !bHeaderAdded )
			{
				IWindow *pCountryItem = AddWindowCopy( pMedalsList, pMedalCountryTemplate );
				ITextView *pCountryName = GetChildChecked<ITextView>( pCountryItem, "ItemCountryName", true );
				if ( pCountryName )
					pCountryName->SetText( pCountryName->GetDBText() + countryNames[nCountry] );
				pCountryItem->ShowWindow( true );
				pMedalsList->PushBack( pCountryItem, false );
				bHeaderAdded = true;
			}

			SMedalDesc aMedal;
			aMedal.szControlName = StrFmt( "ShowMedal%d", nMedalCounter++ );

			IWindow *pItem = AddWindowCopy( pMedalsList, pMedalTemplate );
			ITextView *pMedalName = GetChildChecked<ITextView>( pItem, "ItemMedalName", true );
			IWindow *pMedalIcon = GetChildChecked<IWindow>( pItem, "ItemMedalIcon", true );
			IButton *pMedalButton = GetChildChecked<IButton>( pItem, "ItemMedalButton", true );
			pMedalButton->SetName( aMedal.szControlName );
			if ( i < side.medals.size() )
			{
				if ( pMedalName ) 
					pMedalName->SetText( pMedalName->GetDBText() + GET_TEXT_PRE( side.medals[i]->, LocalizedName ) );
				if ( pMedalIcon )
					pMedalIcon->SetTexture( side.medals[i]->pIconTexture );

				aMedal.pMedal = side.medals[i];
			}
			else
			{
				if ( pMedalName ) 
					pMedalName->SetText( pMedalName->GetDBText() + NStr::ToUnicode( StrFmt( "Medal %d", i ) ) );
			}
			medals.push_back( aMedal );
			pItem->ShowWindow( true );
			pMedalsList->PushBack( pItem, false );
		}
	}
	pMedalsList->Update();

	//////////////////////////////////////// Advanced Panel
	// Win/Lose
	Add4Line( L"", GetScreen()->GetTextEntry( "T_HDR_WON" ), 
		GetScreen()->GetTextEntry( "T_HDR_LOST" ), GetScreen()->GetTextEntry( "T_HDR_WON_PERCENT" ) );
	Add4Line( L"", L"", L"", L"" );
	AddWinLoseSummary( info, GetScreen()->GetTextEntry( "T_HDR_SOLO" ), 1, 0 );
	AddWinLoseSummary( info, GetScreen()->GetTextEntry( "T_HDR_TEAM" ), 0, 1 );
	AddWinLoseSummary( info, GetScreen()->GetTextEntry( "T_HDR_TOTAL" ), 1, 1 );

	// Efficiency
	int nOverallEff = ( info.nKeyPointEff * 30 + info.nUnitEff * 70 ) / 100;
	Add1Line( GetScreen()->GetTextEntry( "T_HDR_AVERAGE_EFF" ) );
	Add2Line( GetScreen()->GetTextEntry( "T_HDR_OVERALL" ), NStr::ToUnicode( StrFmt( "%d%%", nOverallEff ) ) );
	Add2Line( GetScreen()->GetTextEntry( "T_HDR_TACTICAL" ), NStr::ToUnicode( StrFmt( "%d%%", info.nKeyPointEff ) ) );
	Add2Line( GetScreen()->GetTextEntry( "T_HDR_STRATEGIC" ), NStr::ToUnicode( StrFmt( "%d%%", info.nUnitEff ) ) );
	Add1Line( L"" );

	// Favourite things
	wstring wszReinfName = L"-";
	bool bNameFound = false;
	for ( int nSide = 0; nSide < pMPConsts->sides.size() && !bNameFound; ++nSide )
	{
		const NDb::SMultiplayerSide &side = pMPConsts->sides[nSide];
		for ( int nTL = 0; nTL < side.techLevels.size() && !bNameFound; ++nTL )
		{
			const NDb::STechLevelReinfSet &reinfSet = side.techLevels[nTL];
			for ( int i = 0; i < reinfSet.reinforcements.size(); ++i )
			{
				const NDb::SReinforcement *pReinf = reinfSet.reinforcements[i];
				if ( !pReinf )
					continue;
				if ( pReinf->eType == info.nFavouriteReinforcement && CHECK_TEXT_NOT_EMPTY_PRE( pReinf->, LocalizedName ) )
				{
					wszReinfName = GET_TEXT_PRE( pReinf->, LocalizedName );
					bNameFound = true;
					break;
				}
			}
		}
	}
	Add2Line( GetScreen()->GetTextEntry( "T_HDR_FAVOURITE_REINF" ), wszReinfName );
	Add1Line( L"" );
	
	wstring wszName = L"-";
	if ( info.nStrongestAgainst >= 0 && info.nStrongestAgainst < pMPConsts->sides.size() )
		wszName = GET_TEXT_PRE( pMPConsts->sides[info.nStrongestAgainst]., Name );
	Add2Line( GetScreen()->GetTextEntry( "T_HDR_MOST_EFFECTIVE" ), wszName );
	wszName = L"-";
	if ( info.nWeakestAgainst >= 0 && info.nWeakestAgainst < pMPConsts->sides.size() )
		wszName = GET_TEXT_PRE( pMPConsts->sides[info.nWeakestAgainst]., Name );
	Add2Line( GetScreen()->GetTextEntry( "T_HDR_LEAST_EFFECTIVE" ), wszName );

	pAdvList->Update();

	pWaiting->ShowWindow( false );
}

void CInterfaceMPLadderStatistics::AddWinLoseSummary( const SLadderStatistics &info, const wstring &wszName, int nUseSolo, int nUseTeam )
{
	int nWins = 0;
	int nLosses = 0;
	for ( int i = 0; i < countryNames.size(); ++i )
	{
		nWins += nUseSolo * info.raceWinsSolo[i] + nUseTeam * info.raceWinsTeam[i];
		nLosses += nUseSolo * info.raceLossesSolo[i] + nUseTeam * info.raceLossesTeam[i];
	}
	int nPercent = ( nWins + nLosses > 0 ) ?  nWins * 100 / ( nWins + nLosses ) : 0;
	Add4Line( wszName, NStr::ToUnicode( StrFmt( "%d", nWins ) ), 
		NStr::ToUnicode( StrFmt( "%d", nLosses ) ), NStr::ToUnicode( StrFmt( "%d%%", nPercent ) ) );
	for ( int i = 0; i < countryNames.size(); ++i )
	{
		nWins = nUseSolo * info.raceWinsSolo[i] + nUseTeam * info.raceWinsTeam[i];
		nLosses = nUseSolo * info.raceLossesSolo[i] + nUseTeam * info.raceLossesTeam[i];
		nPercent = ( nWins + nLosses > 0 ) ?  nWins * 100 / ( nWins + nLosses ) : 0;
		Add4Line( countryNames[i], NStr::ToUnicode( StrFmt( "%d", nWins ) ), 
			NStr::ToUnicode( StrFmt( "%d", nLosses ) ), NStr::ToUnicode( StrFmt( "%d%%", nPercent ) ) );
	}
	Add4Line( L"", L"", L"", L"" );
}

void CInterfaceMPLadderStatistics::Add4Line( const wstring &wszItem1, const wstring &wszItem2, const wstring &wszItem3, const wstring &wszItem4 )
{
	IWindow *pItem = AddWindowCopy( pAdvList, pAdv4Template );
	if ( !pItem )
		return;

	ITextView *pItem41 = GetChildChecked<ITextView>( pItem, "Item41", true );
	if ( pItem41 )
		pItem41->SetText( pItem41->GetDBText() + wszItem1 );
	ITextView *pItem42 = GetChildChecked<ITextView>( pItem, "Item42", true );
	if ( pItem42 )
		pItem42->SetText( pItem42->GetDBText() + wszItem2 );
	ITextView *pItem43 = GetChildChecked<ITextView>( pItem, "Item43", true );
	if ( pItem43 )
		pItem43->SetText( pItem43->GetDBText() + wszItem3 );
	ITextView *pItem44 = GetChildChecked<ITextView>( pItem, "Item44", true );
	if ( pItem44 )
		pItem44->SetText( pItem44->GetDBText() + wszItem4 );

	pItem->ShowWindow( true );
	pAdvList->PushBack( pItem, false );
}

void CInterfaceMPLadderStatistics::Add2Line( const wstring &wszItem1, const wstring &wszItem2 )
{
	IWindow *pItem = AddWindowCopy( pAdvList, pAdv2Template );
	if ( !pItem )
		return;

	ITextView *pItem21 = GetChildChecked<ITextView>( pItem, "Item21", true );
	if ( pItem21 )
		pItem21->SetText( pItem21->GetDBText() + wszItem1 );
	ITextView *pItem22 = GetChildChecked<ITextView>( pItem, "Item22", true );
	if ( pItem22 )
		pItem22->SetText( pItem22->GetDBText() + wszItem2 );

	pItem->ShowWindow( true );
	pAdvList->PushBack( pItem, false );
}

void CInterfaceMPLadderStatistics::Add1Line( const wstring &wszItem )
{
	ITextView *pItem = dynamic_cast<ITextView*>( AddWindowCopy( pAdvList, pAdv1Template ) );
	if ( !pItem )
		return;
	pItem->ShowWindow( true );
	pItem->SetText( pItem->GetDBText() + wszItem );
	pAdvList->PushBack( pItem, false );
}

bool CInterfaceMPLadderStatistics::OnShowMedal( const string &szSender )
{
	const NDb::SMedal *pMedal = 0;
	for ( int i = 0; i < medals.size(); ++i )
	{
		if ( medals[i].szControlName == szSender )
		{
			pMedal = medals[i].pMedal;
			break;
		}
	}
	if ( !pMedal )
		return true;

	IWindow *pIcon = GetChildChecked<IWindow>( pMedalPopup, "PopupPicture", true );
	ITextView *pName = GetChildChecked<ITextView>( pMedalPopup, "MedalPopupName", true );
	ITextView *pDesc = GetChildChecked<ITextView>( pMedalPopup, "MedalPopupDesc", true );

	if ( pIcon )
		pIcon->SetTexture( pMedal->pPictureTexture );
	if ( pName )
		pName->SetText( pName->GetDBText() + GET_TEXT_PRE( pMedal->, LocalizedName ) );
	if ( pDesc )
		pDesc->SetText( pDesc->GetDBText() + GET_TEXT_PRE( pMedal->, LocalizedDesc ) );

	pMedalPopup->ShowWindow( true );
	return true;
}

// CICMPLadderStatistics

void CICMPLadderStatistics::PreCreate()
{
}

void CICMPLadderStatistics::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
	CInterfaceMPLadderStatistics *pLS = dynamic_cast<CInterfaceMPLadderStatistics*>( pInterface );
	if ( pLS )
		pLS->RequestInfo( szNick );
}

void CICMPLadderStatistics::Configure( const char *pszConfig )
{
	szNick = pszConfig;
}

REGISTER_SAVELOAD_CLASS( 0x19263380, CInterfaceMPLadderStatistics );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MP_LADDER_STATISTICS, CICMPLadderStatistics );


