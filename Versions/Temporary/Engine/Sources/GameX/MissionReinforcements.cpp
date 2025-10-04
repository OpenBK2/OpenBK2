#include "stdafx.h"
#include "MissionReinforcements.h"
#include "InterfaceState.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "Misc/StrProc.h"
#include "CommandsSender.h"
#include "ScenarioTracker.h"
#include "MissionUnitFullInfo.h"
#include "Stats_B2_M1/AIUnitCmd.h"
#include "DBWrapReinf.h"
#include "UIElementsHelper.h"
#include "System/Commands.h"
#include "System/Text.h"

const char* REINF_STATE_NORMAL = "Normal";
const char* REINF_STATE_SELECTED = "Selected";
const char* REINF_STATE_DISABLED = "Disabled";

const float ROLLER_TIME = 1.0f;

const int N_TX_REINF_RIGHT		= 1;
const int N_TX_REINF_INACTIVE	= 2;
const int N_TX_REINF_LIGHT		= 4;

static float s_fBlinkStep = 0.5f; // время показа одного положения мигающей кнопки
static float s_fBlinkDuration = 10.0f; // время мигания кнопки
static float s_fBlinkPeriod = 60.0f; // время между миганиями кнопки
static int s_nAutoShowReinf = 1;

REGISTER_SAVELOAD_CLASS( 0x17163380, CMissionReinf )

// support struct

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

// CMissionReinf::SReinfInfo

void CMissionReinf::SReinfInfo::ShowChevron( int nIndex )
{
	for ( int i = 0; i < chevrons.size(); ++i )
	{
		IWindow *pWnd = chevrons[i];
		if ( pWnd )
			pWnd->ShowWindow( i == nIndex );
	}
}

// CMissionReinf

CMissionReinf::CMissionReinf() : 
	bTrackMousePos( false ),
	ePreSelected( NDb::_RT_NONE ),
	bDisabledByInterface( false ),
	nPrevReinfCount( 0 ),
	bIsOpen( false ),
	bIsLight( false ),
	timeBlinkAbs( 0 ),
	bWasEnabled( false )
{
}

CMissionReinf::~CMissionReinf()
{
	pWorld = 0;
	pNotifications = 0;
	pUnitFullInfo = 0;
}

void CMissionReinf::Init( IScreen *pScr, CWorldClient *_pWorld, IVisualNotifications *_pNotifications, NDb::ESeason _eSeason )
{
	pWorld = _pWorld;
	pNotifications = _pNotifications;
	eSeason = _eSeason;
	if ( !pScr )
		return;
	
	eSelected = NDb::_RT_NONE;
	bAvailNotified = false;
	bBadWeather = false;
	nCalls = 0;

	pReinfPanel = GetChildChecked<IWindow>( pScr, "ReinfPanel", true );
	if ( !pReinfPanel )
		return;
/*	int nX;
	int nSizeX;
	pReinfPanel->GetPlacement( &nX, 0, &nSizeX, 0 );
	pReinfPanel->SetPlacement( nX + nSizeX, 0, 0, 0, EWPF_POS_X );*/
	pReinfPanel->ShowWindow( false );
	pReinfPanel->Enable( false );
		
	IWindow *pRightPanel = GetChildChecked<IWindow>( pScr, "MissionActionsPanel", true );
/*	pReinfMode = GetChildChecked<IWindow>( pRightPanel, "ReinforcementBtn", true );
	if ( pReinfMode )
		pReinfMode->Enable( false );*/
	pToggleReinfBtn = GetChildChecked<IButton>( pRightPanel, "ToggleReinfBtn", true );

	pRoundProgress = GetChildChecked<IWindowRoundProgressBar>( pToggleReinfBtn, "ReinfRoundProgress", true );
	if ( pRoundProgress )
		pRoundProgress->ShowWindow( false );
	pRoundProgressMask = GetChildChecked<IWindow>( pToggleReinfBtn, "ReinfRoundProgressMask", true );
	if ( pRoundProgressMask )
		pRoundProgressMask->ShowWindow( false );

//	pCount = GetChildChecked<ITextView>( pReinfMode, "ReinfCount", true );
	
	pReinfCountPanel = GetChildChecked<IWindow>( pRightPanel, "ReinfCountPanel", true );
	pRoller1 = GetChildChecked<IPlayer>( pReinfCountPanel, "Roller1", true );
	pRoller2 = GetChildChecked<IPlayer>( pReinfCountPanel, "Roller2", true );
	NUIElementsHelper::InitRoller( pRoller1 );
	NUIElementsHelper::InitRoller( pRoller2 );
	
/*	pCloseReinfBtn = GetChildChecked<IButton>( pRightPanel, "CloseReinforcementBtn", true );
	if ( pCloseReinfBtn )
	{
		pCloseReinfBtn->ShowWindow( false );
		if ( pReinfMode )
		{
			int nX;
			int nY;
			pReinfMode->GetPlacement( &nX, &nY, 0, 0 );
			pCloseReinfBtn->SetPlacement( nX, nY, 0, 0, EWPF_POS_X | EWPF_POS_Y );
		}
	}*/

	IWindow *pReinfInfo = GetChildChecked<IWindow>( pScr, "ReinfInfo", true );
	pCommonInfo = GetChildChecked<IWindow>( pReinfInfo, "CommonInfo", true );
	pFullInfoWnd = GetChildChecked<IWindow>( pReinfInfo, "FullInfo", true );
	pNoInfoView = GetChildChecked<ITextView>( pReinfInfo, "NoInfo", true );
	if ( pNoInfoView )
		pNoInfoView->ShowWindow( false );

	if ( pCommonInfo )
	{
		for ( int i = 1; ; ++i )
		{
			IWindow *pWnd = pCommonInfo->GetChild( StrFmt( "ReinfInfoUnit%d", i ), true );
			if ( !pWnd )
				break;
				
			pWnd->ShowWindow( false );
			units.push_back( SUnitInfo() );
			SUnitInfo &info = units.back();
			info.pWnd = pWnd;

			info.pButton = GetChildChecked<IButton>( info.pWnd, "UnitButton", true );
			if ( info.pButton )
				info.pButton->SetName( StrFmt( "UnitButton%d", i ) );

			info.pIcon = GetChildChecked<IWindow>( info.pButton, "Icon", true );
			info.pFlagBg = GetChildChecked<IWindow>( info.pButton, "InfoUnitFlagBg", true );

			info.pCountView = GetChildChecked<ITextView>( info.pWnd, "Count", true );
		}
		
		pName = GetChildChecked<ITextView>( pCommonInfo, "ReinfInfoName", true );
		if ( pName )
			pName->ShowWindow( false );
	}
	SetFullInfoMode( false );
	if ( pUnitFullInfo )
		pUnitFullInfo->SetReinfUnit( 0 );

	// hide all buttons
	for ( int i = 1; ; ++i )
	{
		IWindow *pWnd = pReinfPanel->GetChild( StrFmt( "ButtonReinf%02d", i ), true );
		if ( !dynamic_cast<IButton*>( pWnd ) )
			break;
			
		pWnd->ShowWindow( false );
	}
	
	if ( const NDb::SUIConstsB2 *pUIConsts = InterfaceState()->GetUIConsts() )
	{
		for ( vector< const NDb::SReinfButton >::const_iterator it = pUIConsts->reinfButtons.begin(); 
			it != pUIConsts->reinfButtons.end(); ++it )
		{
			const NDb::SReinfButton &button = *it;
			if ( !button.pButton )
				continue;
			IButton *pButton = GetChildChecked<IButton>( pReinfPanel, button.pButton->szName, true );
			if ( pButton )
			{
				nameToTypes[button.pButton->szName] = button.eType;
				SReinfInfo info;
				info.pButton = pButton;
				info.pIconEnabledWnd = GetChildChecked<IWindow>( pButton, "IconEnabledPanel", true );
				info.pIconWnd = GetChildChecked<IWindow>( pButton, "Icon", true );
				info.pIconDisabledWnd = GetChildChecked<IWindow>( pButton, "IconDisabled", true );
//				info.pXPLevelBtn = GetChildChecked<IButton>( pButton, "XPLevel", true );
				info.pXPBar = GetChildChecked<IProgressBar>( pButton, "XPBar", true );
				info.pXPBarBg = GetChildChecked<IWindow>( pButton, "XPBarBg", true );
				info.pBadWeatherWnd = GetChildChecked<IWindow>( pButton, "BadWeather", true );
				
				pButton->ShowWindow( false/*true*/ );

				if ( info.pIconWnd )
					info.pIconWnd->SetTexture( button.pTexture );
				if ( info.pIconDisabledWnd )
					info.pIconDisabledWnd->SetTexture( button.pTextureDisabled );

				if ( const NDb::SReinforcement *pContext = GetReinfContext( button.eType ) )
				{
					if ( info.pIconWnd )
						info.pIconWnd->SetTexture( pContext->pIconTexture );
//					if ( pContext->pTooltip )
//						info.pIconWnd->SetHelpContext( pContext->pTooltip );
				}

				for ( int j = 0; j <= 3; ++j )
				{
					IWindow *pWnd = GetChildChecked<IWindow>( pButton, StrFmt( "Chevron%02d", j ), true );
					info.chevrons.push_back( pWnd );
				}
				info.ShowChevron( -1 );

				reinfInfos[button.eType] = info;
			}
		}
	}
	
	textures.resize( 8 );
	textures[0] = InterfaceState()->GetTextureEntry( "TX_TOGGLE_REINF_LEFT_NORMAL" );
	textures[N_TX_REINF_LIGHT] = InterfaceState()->GetTextureEntry( "TX_TOGGLE_REINF_LEFT_LIGHT" );
	textures[N_TX_REINF_INACTIVE] = InterfaceState()->GetTextureEntry( "TX_TOGGLE_REINF_LEFT_INACTIVE" );
	textures[N_TX_REINF_INACTIVE | N_TX_REINF_LIGHT] = InterfaceState()->GetTextureEntry( "TX_TOGGLE_REINF_LEFT_INACTIVE_LIGHT" );
	textures[N_TX_REINF_RIGHT] = InterfaceState()->GetTextureEntry( "TX_TOGGLE_REINF_RIGHT_NORMAL" );
	textures[N_TX_REINF_RIGHT | N_TX_REINF_LIGHT] = InterfaceState()->GetTextureEntry( "TX_TOGGLE_REINF_RIGHT_LIGHT" );
	textures[N_TX_REINF_RIGHT | N_TX_REINF_INACTIVE] = InterfaceState()->GetTextureEntry( "TX_TOGGLE_REINF_RIGHT_INACTIVE" );
	textures[N_TX_REINF_RIGHT | N_TX_REINF_INACTIVE | N_TX_REINF_LIGHT] = InterfaceState()->GetTextureEntry( "TX_TOGGLE_REINF_RIGHT_INACTIVE_LIGHT" );
//	SetToggleButtonState( bIsOpen, false, false, -1.0f );
	
	pUnitFullInfo = new CMissionUnitFullInfo();
	IWindow *pAppearanceWnd = GetChildChecked<IWindow>( pScr, "AppearancePanel", true );
	IWindow *pAppearanceForReinfWnd = GetChildChecked<IWindow>( pAppearanceWnd, "AppearanceForReinf", true );
	pUnitFullInfo->InitByReinf( pFullInfoWnd, pAppearanceForReinfWnd, eSeason );
	
	pCallReinfModeBtn = GetChildChecked<IButton>( pReinfPanel, "CallReinfModeBtn", true );
	pAutoShowReinfBtn = GetChildChecked<IButton>( pReinfPanel, "AutoShowReinfBtn", true );
	if ( pCallReinfModeBtn )
		pCallReinfModeBtn->Enable( false );
	if ( pCallReinfModeBtn )
		pCallReinfModeBtn->ShowWindow( false );
	if ( pAutoShowReinfBtn )
		pAutoShowReinfBtn->ShowWindow( false );

	for ( CReinfInfos::const_iterator it = reinfInfos.begin(); it != reinfInfos.end(); ++it )
	{
		const SReinfInfo &info = it->second;
		if ( info.pXPBar )
			info.pXPBar->ShowWindow( false );
		if ( info.pXPBarBg )
			info.pXPBarBg->ShowWindow( false );
	}
	
	UpdateReinfsXPAndLevel();
	
	Update();
}

void CMissionReinf::SetMousePos( const CVec2 &vPos )
{
	vMousePos = vPos;
	if ( !IsEnabled() )
		return;
	UpdatePoints();
}

void CMissionReinf::SetTrackMousePos( bool bTrack )
{
	bTrackMousePos = bTrack;
	if ( !IsEnabled() )
		return;
	UpdatePoints();
}

void CMissionReinf::UpdateNewAvail()
{
	bDisabledByInterface = !pWorld->IsReinfEnabled();
	Update();
}

void CMissionReinf::UpdateNewPoint( bool bIsPoints )
{
	if ( GetReinfCallsLeft() == 0 )
		return;

	if ( bIsPoints )
	{
		pNotifications->OnRemoveEvent( NDb::NEVT_REINF_CANT_CALL, -1 );
	}
}

void CMissionReinf::Update()
{
	if ( !pWorld )
		return;
	
	UpdateReinfs();
	UpdatePoints();
	UpdateEnable();
	UpdateReinfsVisual();
}

void CMissionReinf::Select( const string &szSender )
{
	NDb::EReinforcementType eType = NDb::_RT_NONE;
	CNameToTypes::iterator it = nameToTypes.find( szSender );
	if ( it != nameToTypes.end() )
	{
		eType = it->second;
	}
	if ( eType != NDb::_RT_NONE )
	{
		NInput::PostEvent( "reinf_mode", 1, 0 );
	}
	Select( eType );
}

void CMissionReinf::SelectDblClick( const string &szSender )
{
	Call( CVec2( -1.0f, -1.0f ) );
}

void CMissionReinf::Select( NDb::EReinforcementType eType )
{
	eSelected = NDb::_RT_NONE;
	
	for ( CReinfInfos::iterator iInfo = reinfInfos.begin(); iInfo != reinfInfos.end(); ++iInfo )
	{
		SReinfInfo &info = iInfo->second;
		if ( info.pReinf )
		{
			if ( iInfo->first == eType )
			{
				eSelected = eType;
				if ( info.pButton )
				{
					int nState = info.pButton->GetState( REINF_STATE_SELECTED );
					info.pButton->SetStateWithVisual( nState );
				}
			}
			else
			{
				if ( info.pButton )
				{
					int nState = info.pButton->GetState( REINF_STATE_NORMAL );
					info.pButton->SetStateWithVisual( nState );
				}
			}
		}
		else
		{
		}
	}

	UpdateReinfInfo();
	
	UpdateForcedAction();
}

void CMissionReinf::UpdateForcedAction()
{
	if ( !IsOpen() )
 		return;

	NDb::EUserAction eUserAction = NDb::USER_ACTION_REINF_COMMON;
	if ( const NDb::SReinforcementTypes *pReinfTypes = GetReinfTypes() )
	{
		for ( vector< NDb::SReinforcementTypeInfo >::const_iterator it = pReinfTypes->typeInfo.begin(); 
			it != pReinfTypes->typeInfo.end(); ++it )
		{
			const NDb::SReinforcementTypeInfo &info = *it;
			if ( info.eType == eSelected )
			{
				eUserAction = info.eUserAction;
				break;
			}
		}
	}
	
	if ( eSelected == NDb::_RT_NONE )
	{
		// do nothing
	}
	else if ( !CanCall( eSelected ) )
		NInput::PostEvent( "set_forced_action", NDb::USER_ACTION_REINF_NONE, 0 );
	else
		NInput::PostEvent( "set_forced_action", eUserAction, 0 );
}

void CMissionReinf::SetFullInfoMode( bool bFullInfo )
{
	bFullInfoMode = bFullInfo;
	if ( pCommonInfo )
		pCommonInfo->ShowWindow( !bFullInfoMode );
	if ( pFullInfoWnd )
		pFullInfoWnd->ShowWindow( bFullInfoMode );
}

void CMissionReinf::Close( bool bForced )
{
	bIsOpen = false;
	UpdateToggleButtonState();
	if ( pReinfPanel )
		pReinfPanel->Enable( false );
	if ( pReinfPanel )
		pReinfPanel->ShowWindow( false );

	if ( bForced )
	{
		NInput::PostEvent( "reinf_mode", 0, 0 );
		eSelected = NDb::_RT_NONE;
	}
	
	pNotifications->Notify( EVNT_SELECT_KEY_POINT, -1, VNULL2 );
}

void CMissionReinf::ShowUnitInfo( const string &szSender )
{
	for ( vector< SUnitInfo >::const_iterator it = units.begin(); it != units.end(); ++it )
	{
		const SUnitInfo &info = *it;
		if ( info.pButton && info.pButton->GetName() == szSender )
		{
			SetFullInfoMode( true );
			if ( pUnitFullInfo )
				pUnitFullInfo->SetReinfUnit( info.pStats );
			return;
		}
	}
}

NDb::EReinforcementType CMissionReinf::FindReinfInfo( const string &szName )
{
	for ( CReinfInfos::iterator iInfo = reinfInfos.begin(); iInfo != reinfInfos.end(); ++iInfo )
	{
		SReinfInfo &info = iInfo->second;
		if ( info.pButton && info.pButton->GetName() == szName )
		{
			return iInfo->first;
		}
	}
	return NDb::_RT_NONE;
}

void CMissionReinf::UpdateReinfInfo()
{
	NDb::EReinforcementType eType = ePreSelected;
	if ( eType == NDb::_RT_NONE )
		eType = eSelected;
	
	for ( CReinfInfos::iterator iInfo = reinfInfos.begin(); iInfo != reinfInfos.end(); ++iInfo )
	{
		SReinfInfo &info = iInfo->second;
		if ( iInfo->first == eType )
		{
			ShowReinfContents( info.pReinf );
			break;
		}
	}
}

bool CMissionReinf::CanCall( const NDb::EReinforcementType eType ) const
{
	if ( !IsEnabled() )
		return false;
	if ( eType == NDb::_RT_NONE )
		return false;
	if ( IsAviaAndCanNotFly( eType ) )
		return false;
	return true;
}

bool CMissionReinf::IsAvia( const NDb::EReinforcementType eType ) const
{
	switch ( eType )
	{
		case NDb::RT_FIGHTERS:
		case NDb::RT_BOMBERS:
		case NDb::RT_GROUND_ATTACK_PLANES:
		case NDb::RT_RECON:
		case NDb::RT_PARATROOPS:
			return true;
			
		case NDb::RT_ELITE_INFANTRY:
		{
			IScenarioTracker *pST = Singleton<IScenarioTracker>();
			const int nLocalPlayer = pST->GetLocalPlayer();
			const NDb::SReinforcement *pReinf = pST->GetReinforcement( nLocalPlayer, eType );
			if ( pReinf )
			{
				for ( int i = 0; i < pReinf->entries.size(); ++i )
				{
					const NDb::SReinforcementEntry &entry = pReinf->entries[i];
					if ( entry.pMechUnit && entry.pMechUnit->IsAviation() )
						return true;
				}
			}
			return false;
		}
		
		default:
			return false;
	};
}

int CMissionReinf::GetReinfCallsLeft() const
{
	if ( !Singleton<IScenarioTracker>() )
		return 0;
	return Singleton<IScenarioTracker>()->GetReinforcementCallsLeft( Singleton<IScenarioTracker>()->GetLocalPlayer() );
}

void CMissionReinf::UnitFullInfoBack()
{
	SetFullInfoMode( false );
	if ( pUnitFullInfo )
		pUnitFullInfo->SetReinfUnit( 0 );
}

void CMissionReinf::MouseOverForward( const string &szSender )
{
	ePreSelected = FindReinfInfo( szSender );
	UpdateReinfInfo();
}

void CMissionReinf::MouseOverBackward( const string &szSender )
{
	ePreSelected = NDb::_RT_NONE;
	UpdateReinfInfo();
}

void CMissionReinf::BadWeather( bool bStart )
{
	bBadWeather = bStart;

	if ( !bBadWeather )
	{
		if ( IsNonAllWeatherAviaPresents() )
		{
			IVisualNotifications::SEventParams params;
			params.nID = -1;
			params.eEventType = NDb::NEVT_AVIA_AVAILABLE;
			pNotifications->OnEvent( params );
		}
	}

	if ( !IsOpen() )
 		return;

	Update();
}

void CMissionReinf::UpdateWinLooseState()
{
	if ( !Singleton<IScenarioTracker>()->IsMissionActive() )
	{
		Update();
	}
}

void CMissionReinf::Show()
{
	if ( bIsOpen )
		return;

	bIsOpen = true;

	UpdateReinfsXPAndLevel();
	UpdateReinfsVisual();

	if ( pReinfPanel )
		pReinfPanel->Enable( true );
	UpdateToggleButtonState();

	if ( pReinfPanel )
		pReinfPanel->ShowWindow( true );

	if ( IsEnabled() )
	{
		wstring wszText = InterfaceState()->GetTextEntry( "T_REINF_OPEN" );
		InterfaceState()->WriteToMissionConsole( wszText );
	}
		
//	if ( eSelected == NDb::_RT_NONE ) // don't select default now
//		eSelected = GetDefaultReinfType();
	eSelected = NDb::_RT_NONE;
	Select( eSelected );

	bWasEnabled = true;
}

bool CMissionReinf::IsEnabled() const
{
	if ( !Singleton<IScenarioTracker>() )
		return false;
	int nCount = GetReinfCallsLeft();
	return Singleton<IScenarioTracker>()->IsMissionActive() && pWorld->IsReinfEnabled() &&
		(nCount == IAIScenarioTracker::INFINITE_CALLS || nCount > 0) && !bDisabledByInterface;
}

void CMissionReinf::UpdateEnable()
{
	if ( !Singleton<IScenarioTracker>() )
		return;
	bool bEnabled = IsEnabled();

	UpdateToggleButtonState();

	int nCount = GetReinfCallsLeft();
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	if ( nCount == IAIScenarioTracker::INFINITE_CALLS )
		nCount = 99;

	vector<IPlayer*> rollers;
	rollers.push_back( pRoller2 );
	rollers.push_back( pRoller1 );
	if ( nPrevReinfCount != nCount )
		NUIElementsHelper::PlayRollerAnim( rollers, nPrevReinfCount, nCount, ROLLER_TIME );
	nPrevReinfCount = nCount;

	if ( pScenarioTracker->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	{
		int nSeconds = pScenarioTracker->GetReinforcementXP( pScenarioTracker->GetLocalPlayer(), NDb::_RT_NONE );
		if ( nSeconds > 0 )
		{
			int nMinutes = nSeconds / 60;
			nSeconds = nSeconds % 60;
			wstring wszTooltip = InterfaceState()->GetTextEntry( "T_REINF_RECYCLE_TOOLTIP" ) + NStr::ToUnicode( StrFmt( "%d:%02d", nMinutes, nSeconds ) );
			pRoller1->SetTooltip( wszTooltip );
			pRoller2->SetTooltip( wszTooltip );
		}
	}
		
	if ( Singleton<IScenarioTracker>()->IsMissionActive() )
	{
		if ( IsEnabled() )
			NotifyAvailReinf();
		else
			DisableNotifyAvailReinf();
		
		SetBlink( true, !HaveUnits() );
	}
	else
		SetBlink( false, false );

	if ( pCallReinfModeBtn )
		pCallReinfModeBtn->Enable( bEnabled && CanCall( eSelected ) );
}

void CMissionReinf::UpdateReinfs()
{
	 const CEnabledReinforcements &enabledReinf = pWorld->GetEnabledReinfs();
	 
	// clear previous state
	for ( CReinfInfos::iterator it = reinfInfos.begin(); it != reinfInfos.end(); ++it )
	{
		SReinfInfo &reinf = it->second;
		reinf.pReinf = 0;
	}
	
	// fill new data
	for ( CEnabledReinforcements::const_iterator it = enabledReinf.begin(); 
		it != enabledReinf.end(); ++it )
	{
		const NDb::SReinforcement *pReinf = *it;
		CReinfInfos::iterator iInfo = reinfInfos.find( pReinf->eType );
		if ( iInfo == reinfInfos.end() )
			continue;
//		NI_VERIFY( iInfo != reinfInfos.end(), "Forbidden reinf type", continue );
		SReinfInfo &info = iInfo->second;
		info.pReinf = pReinf;
	}
	
	UpdateReinfsXPAndLevel();
	
	UpdateForcedAction();
}

void CMissionReinf::UpdateReinfsVisual()
{
	bool bEnabled = IsEnabled();
	for ( CReinfInfos::iterator it = reinfInfos.begin(); it != reinfInfos.end(); ++it )
	{
		NDb::EReinforcementType eType = it->first;
		SReinfInfo &info = it->second;
		bool bIsReinf = (info.pReinf != 0);
		bool bCanCall = bEnabled && CanCall( eType );

		if ( info.pButton )
			info.pButton->ShowWindow( bIsReinf );
		
		if ( info.pIconWnd )
			info.pIconWnd->ShowWindow( bIsReinf && bCanCall );
		if ( info.pIconDisabledWnd )
			info.pIconDisabledWnd->ShowWindow( bIsReinf && !bCanCall );
			
		if ( info.pBadWeatherWnd )
			info.pBadWeatherWnd->ShowWindow( bIsReinf && IsAviaAndCanNotFly( eType ) );
	}
}

void CMissionReinf::UpdateMPReinfsXPAndLevel()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( !pST )
		return;
	if ( !IsOpen() )
		return;
	if ( !pST->GetMultiplayerInfo() )
		return;

	for ( CReinfInfos::iterator it = reinfInfos.begin(); it != reinfInfos.end(); ++it )
	{
		const NDb::EReinforcementType eType = it->first;
		SReinfInfo &info = it->second;

		const float fExp = pST->GetReinforcementXP( pST->GetLocalPlayer(), eType );
		const int nRank = pST->GetReinforcementXPLevel( pST->GetLocalPlayer(), eType );
		const float fCurRankExp = pST->GetReinforcementXPForLevel( eType, nRank );
		const float fNextRankExp = pST->GetReinforcementXPForLevel( eType, nRank + 1 );
		const float fDeltaExp = fNextRankExp - fCurRankExp;

		float fPosition;
		if ( fDeltaExp > 0.0f )
		{
			if ( fCurRankExp < fNextRankExp )
				fPosition = (fExp - fCurRankExp) / fDeltaExp;
			else
				fPosition = 1.0f; // reached the cap
		}
		else
			fPosition = 1.0f;

		// leader's rank
		info.ShowChevron( nRank );

		if ( info.pXPBar )
			info.pXPBar->SetPosition( fPosition );

		//		if ( info.pXPLevelBtn )
		//			info.pXPLevelBtn->ShowWindow( info.pReinf != 0 );
		if ( info.pXPBar )
			info.pXPBar->ShowWindow( info.pReinf != 0 );
		if ( info.pXPBarBg )
			info.pXPBarBg->ShowWindow( info.pReinf != 0 );

		const int nLocalPlayer = pST->GetLocalPlayer();
		wstring wszTooltip = NDBWrap::GetReinfXPLevelName( pST->GetReinforcementXPLevel( nLocalPlayer, eType ) );
		if ( const NDb::SReinforcement *pContext = GetReinfContext( eType ) )
		{
			if ( CHECK_TEXT_NOT_EMPTY_PRE(pContext->,Tooltip) )
				wszTooltip = GET_TEXT_PRE(pContext->,Tooltip) + L"<br>" + wszTooltip;
		}
		if ( info.pButton )
			info.pButton->SetTooltip( wszTooltip );
	}
}

void CMissionReinf::UpdateReinfsXPAndLevel()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( !pST )
		return;
	const NDb::SCampaign *pCampaign = pST->GetCurrentCampaign();
	if ( !pCampaign )
		return;
	if ( !IsOpen() )
		return;
	for ( CReinfInfos::iterator it = reinfInfos.begin(); it != reinfInfos.end(); ++it )
	{
		NDb::EReinforcementType eType = it->first;
		SReinfInfo &info = it->second;

		bool bIsLeader = false;
		float fExp = 0.0f;
		int nRank = -1;
		float fCurRankExp = 0.0f;
		float fNextRankExp = 0.0f;
		float fDeltaExp = 0.0f;

		if ( const IScenarioTracker::SLeaderInfo *pLeader = pST->GetLeaderInfo( eType ) )
		{
			bIsLeader = true;
			fExp = pLeader->info.fExp;
			nRank = pLeader->info.nRank;
			fCurRankExp = pST->GetLeaderRankExp( eType, nRank );
			fNextRankExp = pST->GetLeaderRankExp( eType, nRank + 1 );
			fDeltaExp = fNextRankExp - fCurRankExp;
		}
		
		float fPosition;
		if ( fDeltaExp > 0.0f )
		{
			if ( fCurRankExp < fNextRankExp )
				fPosition = (fExp - fCurRankExp) / fDeltaExp;
			else
				fPosition = 1.0f; // reached the cap
		}
		else
			fPosition = 0.0f;

		// leader's rank
		info.ShowChevron( nRank );

		if ( info.pXPBar )
			info.pXPBar->SetPosition( fPosition );

//		if ( info.pXPLevelBtn )
//			info.pXPLevelBtn->ShowWindow( info.pReinf != 0 );
		if ( info.pXPBar )
			info.pXPBar->ShowWindow( info.pReinf != 0 && bIsLeader );
		if ( info.pXPBarBg )
			info.pXPBarBg->ShowWindow( info.pReinf != 0 && bIsLeader );

		const int nLocalPlayer = 0;
		wstring wszTooltip = NDBWrap::GetReinfXPLevelName( pST->GetReinforcementXPLevel( nLocalPlayer, eType ) );
		if ( const NDb::SReinforcement *pContext = GetReinfContext( eType ) )
		{
			if ( CHECK_TEXT_NOT_EMPTY_PRE(pContext->,Tooltip) )
				wszTooltip = GET_TEXT_PRE(pContext->,Tooltip) + L"<br>" + wszTooltip;
		}
		if ( info.pButton )
			info.pButton->SetTooltip( wszTooltip );
	}
}

void CMissionReinf::UpdatePoints()
{
	int nBestID = -1;
	float fBestDist2 = 0.0f;
	
	if ( IsOpen() && bTrackMousePos )
	{
		const CKeyBuildings &keyBuildings = pWorld->GetKeyBuildings();
		for ( CKeyBuildings::const_iterator it = keyBuildings.begin(); it != keyBuildings.end(); ++it )
		{
			CMapObj *pMO = it->second;

			if ( pMO->GetKeyObjectPlayer() == Singleton<IScenarioTracker>()->GetLocalPlayer() )
			{
				int nID = it->first;
				CVec2 vPos( pMO->GetCenter().x, pMO->GetCenter().y );
				float fDist2 = fabs2( vMousePos - vPos );
				if ( nBestID == -1 || fDist2 < fBestDist2 )
				{
					nBestID = nID;
					fBestDist2 = fDist2;
				}
			}
		}
	}

	pNotifications->Notify( EVNT_SELECT_KEY_POINT, nBestID, VNULL2 );
}

NDb::EReinforcementType CMissionReinf::GetDefaultReinfType() const
{
	for ( CReinfInfos::const_iterator it = reinfInfos.begin(); it != reinfInfos.end(); ++it )
	{
		const SReinfInfo &reinf = it->second;
		if ( reinf.pReinf )
			return reinf.pReinf->eType;
	}

	return NDb::_RT_NONE;
}

void CMissionReinf::ShowReinfContents( const NDb::SReinforcement *pReinf )
{
	SetFullInfoMode( false );
	if ( pUnitFullInfo )
		pUnitFullInfo->SetReinfUnit( 0 );

	if ( pNoInfoView )
		pNoInfoView->ShowWindow( pReinf == 0 );
	if ( !pReinf )
	{
		for ( int i = 0; i < units.size(); ++i )
		{
			IWindow *pWnd = units[i].pWnd;
			pWnd->ShowWindow( false );
		}
		if ( pName )
			pName->ShowWindow( false );
	}
	else
	{
		vector<STmpReinfEntry> tmpReinfEntries;
		for ( int i = 0; i < pReinf->entries.size(); ++i )
		{
			const NDb::SReinforcementEntry &entry = pReinf->entries[i];

			STmpReinfEntry tmpReinfEntry;
			tmpReinfEntry.pStats = entry.pMechUnit;
			if ( !tmpReinfEntry.pStats )
				tmpReinfEntry.pStats = entry.pSquad;
			NI_VERIFY( tmpReinfEntry.pStats, StrFmt("Incorrect data in reinforcement \"%s\" entry %d", pReinf->GetDBID().ToString().c_str(), i), continue );
			tmpReinfEntry.nCount = 1;
			
			tmpReinfEntries.push_back( tmpReinfEntry );
		}

		// merge dupe
		for ( vector<STmpReinfEntry>::iterator it = tmpReinfEntries.begin(); it != tmpReinfEntries.end(); ++it )
		{
			STmpReinfEntry &tmpReinfEntry = *it;
			while ( true )
			{
				vector<STmpReinfEntry>::iterator it2 = it;
				it2 = find( ++it2, tmpReinfEntries.end(), tmpReinfEntry );
				if ( it2 != tmpReinfEntries.end() )
				{
					tmpReinfEntry.nCount++;
					tmpReinfEntries.erase( it2 );
				}
				else
					break;
			}
		}
		
		//NI_ASSERT( tmpReinfEntries.size() <= units.size(), "Can't show all unit's types in the reinforcement" );
		for ( int i = 0; i < units.size(); ++i )
		{
			SUnitInfo &info = units[i];
			if ( i >= tmpReinfEntries.size() )
			{
				info.pWnd->ShowWindow( false );
				continue;
			}
			info.pWnd->ShowWindow( true );
			info.pStats = tmpReinfEntries[i].pStats;
			info.nCount = tmpReinfEntries[i].nCount;
			
			const NDb::STexture *pIconTexture = 0;
			const NDb::STexture *pFlagBgTexture = 0;
			if ( info.pStats )
			{
				pIconTexture = info.pStats->pIconTexture;
				pFlagBgTexture = info.pStats->pIconFlagBackground;
			}
			if ( info.pIcon )
				info.pIcon->SetTexture( pIconTexture );
			if ( info.pFlagBg )
				info.pFlagBg->SetTexture( pFlagBgTexture );
				
			if ( info.pCountView )
				info.pCountView->SetText( info.pCountView->GetDBText() + NStr::ToUnicode( StrFmt( "%d", info.nCount ) ) );
		}

		if ( pName )
		{
			if ( CHECK_TEXT_NOT_EMPTY_PRE(pReinf->,LocalizedName) )
				pName->SetText( pName->GetDBText() + GET_TEXT_PRE(pReinf->,LocalizedName) );
			else
				pName->SetText( L"" );
			pName->ShowWindow( true );
		}
	}
}

void CMissionReinf::NotifyAvailReinf()
{
	if ( bAvailNotified )
		return;
	
	bAvailNotified = true;
	pNotifications->Notify( EVNT_REINFORCEMENT_AVAILABLE, -1, VNULL2 );
}

void CMissionReinf::DisableNotifyAvailReinf()
{
	bAvailNotified = false;
	pNotifications->Notify( EVNT_REINFORCEMENT_AVAILABLE_CANCEL, -1, VNULL2 );
}

void CMissionReinf::Call( const CVec2 &vPos )
{
	NI_ASSERT( CanCall( eSelected ), "Wrong call" );
	if ( CanCall( eSelected ) )
	{
		SAIUnitCmd cmd;
		cmd.nCmdType = ACTION_COMMAND_ORDER;
		cmd.nNumber = eSelected;
		cmd.vPos = vPos;
		pWorld->GetCommandsSender()->CommandUnitCommand( &cmd );
		
		nCalls++;
		bDisabledByInterface = true;

/*		if ( GetReinfCallsLeft() == 0 )
		{
			IVisualNotifications::SEventParams params;
			params.nID = -1;
			params.eEventType = NDb::NEVT_REINF_CANT_CALL;
			pNotifications->OnEvent( params );
		}*/
		
		IScenarioTracker *pST = Singleton<IScenarioTracker>();
		pST->MarkFavoriteReinf( eSelected );
	}

	Close( true );
}

void CMissionReinf::CallNoReinf()
{
	wstring wszText = InterfaceState()->GetTextEntry( "T_REINF_CANT_CALL" );
	InterfaceState()->WriteToMissionConsole( wszText );
}

void CMissionReinf::Step()
{
	UpdateMPReinfsXPAndLevel();
	UpdateReinfsXPAndLevel();

	UpdateEnable();
	
	UpdateBlink();
	
	bool bEnabled = IsEnabled();
	if ( IsAutoShowReinf() && bEnabled && !bWasEnabled && NGlobal::GetVar( "History.Playing", 0 ) == 0 )
	{
		if ( !IsOpen() )
		{
			Show();
		}
	}
	
	if ( !bEnabled )
		bWasEnabled = false;
}

const NDb::SReinforcement* CMissionReinf::GetReinfContext( const NDb::EReinforcementType eType ) const
{
	return Singleton<IScenarioTracker>()->GetReinforcement( 0, eType );
}

const NDb::SReinforcementTypes* CMissionReinf::GetReinfTypes() const
{
	if ( const NDb::SCampaign *pCampaign = Singleton<IScenarioTracker>()->GetCurrentCampaign() )
		return pCampaign->pReinforcementTypes;
	return 0;
}

void CMissionReinf::OnClickFullInfoMember( const string &szSender )
{
	pUnitFullInfo->OnClickMember( szSender );
}

void CMissionReinf::OnFullInfoMemberOverOn( const string &szSender )
{
	pUnitFullInfo->OnMemberOverOn( szSender );
}

void CMissionReinf::OnFullInfoMemberOverOff( const string &szSender )
{
	pUnitFullInfo->OnMemberOverOff( szSender );
}

void CMissionReinf::OnFullInfoWeaponOverOn( const string &szSender )
{
	pUnitFullInfo->OnWeaponOverOn( szSender );
}

void CMissionReinf::OnFullInfoWeaponOverOff( const string &szSender )
{
	pUnitFullInfo->OnWeaponOverOff( szSender );
}

void CMissionReinf::OnReinfCallMode()
{
	Close( false );
}

void CMissionReinf::OnReinfAutoShowReinf( bool bOn )
{
	if ( pAutoShowReinfBtn ) 
		pAutoShowReinfBtn->SetStateWithVisual( bOn ? 0 : 1 );
}

bool CMissionReinf::IsAutoShowReinf() const
{
	return s_nAutoShowReinf != 0;
}

bool CMissionReinf::IsAviaPresents() const
{
	for ( CReinfInfos::const_iterator it = reinfInfos.begin(); it != reinfInfos.end(); ++it )
	{
		NDb::EReinforcementType eType = it->first;
		const SReinfInfo &info = it->second;
		
		if ( IsAvia( eType ) && info.pReinf )
			return true;
	}
	return false;
}

bool CMissionReinf::IsNonAllWeatherAviaPresents() const
{
	for ( CReinfInfos::const_iterator it = reinfInfos.begin(); it != reinfInfos.end(); ++it )
	{
		NDb::EReinforcementType eType = it->first;
		const SReinfInfo &info = it->second;
		
		if ( info.pReinf && IsAvia( eType ) && !IsAllWeatherAvia( eType ) ) 
			return true;
	}
	return false;
}

bool CMissionReinf::IsAviaAndCanNotFly( const NDb::EReinforcementType eType ) const
{
	return bBadWeather && IsAvia( eType ) && !IsAllWeatherAvia( eType );
}

bool CMissionReinf::IsAllWeatherAvia( const NDb::EReinforcementType eType ) const
{
	switch ( eType )
	{
		case NDb::RT_FIGHTERS:
		case NDb::RT_BOMBERS:
		case NDb::RT_GROUND_ATTACK_PLANES:
		case NDb::RT_RECON:
		case NDb::RT_PARATROOPS:
		{
			IScenarioTracker *pST = Singleton<IScenarioTracker>();
			int nLocalPlayer = pST->GetLocalPlayer();
			int nLevel = pST->GetReinforcementXPLevel( nLocalPlayer, eType );
			const NDb::SReinforcement *pReinf = pST->GetReinforcement( nLocalPlayer, eType );
			bool bAllWeather = HasMasterPilot( pReinf, nLevel );
			return bAllWeather;
		}
			
		default:
			return false;
	};
}

bool CMissionReinf::HasMasterPilot( const NDb::SReinforcement *pReinf, int nLevel ) const
{
	if ( !pReinf )
		return false;

	for ( int i = 0; i < pReinf->entries.size(); ++i )
	{
		const NDb::SReinforcementEntry &entry = pReinf->entries[i];
		if ( entry.pMechUnit )
		{
			const NDb::SUnitActions *pActions = entry.pMechUnit->pActions;
			if ( pActions )
			{
				for ( int j = 0; j < pActions->specialAbilities.size(); ++j )
				{
					if ( j > nLevel )
						break;
					const NDb::SUnitSpecialAblityDesc *pAbility = pActions->specialAbilities[j];
					if ( pAbility )
					{
						if ( pAbility->eName == NDb::ABILITY_MASTER_PILOT )
							return true;
					}
				}
			}
		}
	}
	return false;
}

void CMissionReinf::AfterLoad()
{
	pRoller1->Stop();
	pRoller2->Stop();
	vector<IPlayer*> rollers;
	rollers.push_back( pRoller2 );
	rollers.push_back( pRoller1 );
	NUIElementsHelper::PlayRollerAnim( rollers, nPrevReinfCount, nPrevReinfCount, 0 );
}

void CMissionReinf::UpdateToggleButtonState()
{
	NTimer::STime timeToEnable = pWorld->GetReinfRecycleEnd();
	float fProgress = pWorld->GetReinfRecycleProgress();
	int nCount = GetReinfCallsLeft();
	bool bHaveReinf = ( nCount == IAIScenarioTracker::INFINITE_CALLS || nCount > 0 );
	bool bClockVisible = Singleton<IScenarioTracker>()->IsMissionActive() && 
		!pWorld->IsReinfEnabled() && timeToEnable != 0 && bHaveReinf;

	int nIndex = (bIsOpen ? N_TX_REINF_RIGHT : 0) |
		(pWorld->IsReinfEnabled() ? 0 : N_TX_REINF_INACTIVE) |
		(!bClockVisible && bIsLight ? N_TX_REINF_LIGHT : 0);
	if ( pToggleReinfBtn )
		pToggleReinfBtn->SetTexture( textures[nIndex] );

	if ( pRoundProgress )
	{
		pRoundProgress->ShowWindow( bClockVisible );
		if ( bClockVisible )
			pRoundProgress->SetAngles( FP_PI2, FP_PI2 - fProgress * FP_2PI );
	}
	if ( pRoundProgressMask )
		pRoundProgressMask->ShowWindow( bClockVisible );
}

void CMissionReinf::SetBlink( bool _bBlink, bool _bFullTime )
{
	if ( bBlink != _bBlink || bBlinkFullTime != _bFullTime )
	{
		bBlink = _bBlink;
		bBlinkFullTime = _bFullTime;

		fBlinkStep = 0.0f;
		fBlinkDuration = 0.0f;
		fBlinkPeriod = 0.0f;

		timeBlinkAbs = Singleton<IGameTimer>()->GetAbsTime();
		
		if ( bBlink )
		{
			bIsLight = true;
			UpdateToggleButtonState();
		}
	}
}

void CMissionReinf::UpdateBlink()
{
	if ( timeBlinkAbs == 0 )
	{
		timeBlinkAbs = Singleton<IGameTimer>()->GetAbsTime();
		return;
	}
	if ( !bBlink )
		return;

	NTimer::STime timeAbs = Singleton<IGameTimer>()->GetAbsTime();
	float fDeltaTime = (timeAbs - timeBlinkAbs) / 1000.0f;

	fBlinkStep += fDeltaTime;
	if ( fBlinkStep >= s_fBlinkStep )
	{
		fBlinkStep = 0.0f;
		bIsLight = !bIsLight;
	}

	if ( !HaveUnits() != bBlinkFullTime )
	{
		SetBlink( true, !bBlinkFullTime );
		return;
	}

	if ( !bBlinkFullTime )
	{
		fBlinkDuration += fDeltaTime;
		fBlinkPeriod += fDeltaTime;
		if ( fBlinkDuration >= s_fBlinkDuration )
		{
			bIsLight = false;
		}
		if ( fBlinkPeriod >= s_fBlinkPeriod )
		{
			bIsLight = true;
			fBlinkStep = 0.0f;
			fBlinkDuration = 0.0f;
			fBlinkPeriod = 0.0f;
		}
	}
	
	UpdateToggleButtonState();

	timeBlinkAbs = timeAbs;
}

bool CMissionReinf::HaveUnits() const
{
	return pWorld->IsOwnUnitsPresent();
}

bool CMissionReinf::ResetReinfMode()
{
	if ( !IsOpen() )
	{
		Select( NDb::_RT_NONE );
		return true;
	}
	return false;
}

START_REGISTER(MissionReinforcements)

REGISTER_VAR_EX( "reinf_button_blink_step", NGlobal::VarFloatHandler, &s_fBlinkStep, 0.5f, STORAGE_NONE );
REGISTER_VAR_EX( "reinf_button_blink_duration", NGlobal::VarFloatHandler, &s_fBlinkDuration, 10.0f, STORAGE_NONE );
REGISTER_VAR_EX( "reinf_button_blink_period", NGlobal::VarFloatHandler, &s_fBlinkPeriod, 60.0f, STORAGE_NONE );

REGISTER_VAR_EX( "auto_show_reinf", NGlobal::VarIntHandler, &s_nAutoShowReinf, 1, STORAGE_USER );

FINISH_REGISTER


