#include "stdafx.h"
#include "Stats_B2_M1/ActionCommand.h"
#include "InterfaceArmyScreen.h"
#include "GameXClassIDs.h"
#include "Misc/StrProc.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "InterfaceState.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "InterfaceMisc.h"
#include "B2_M1_World/MapObj.h"
#include "DBWrapReinf.h"
#include "UISpecificB2/EffectorB2Move.h"
#include "System/Text.h"

#include "GameX_export.h"

#include <fmt/format.h>

const int WAIT_TIME = 300; // msec
const int STEP_WAIT_TIME = 50; // msec
const float EXP_PROGRESS_STEP_FRACTION = 0.05f;
const wchar_t* TEXT_TAG_ABILITY_TOOLTIP = L"ability_tooltip";
const wchar_t* DYNAMIC_TAG_RANK_NUMBER = L"rank_number";
const char* DYNAMIC_TAG_RANK_NAME_FORMAT = "rank{}_name";

class CInterfaceArmyScreen::CReinfData : public CObjectBase
{
	OBJECT_BASIC_METHODS( CReinfData );
public:
	struct SAbility
	{
		ZDATA
		std::wstring wszName;
		std::wstring wszDesc;
		CDBPtr<NDb::STexture> pEnabledIcon;
		std::wstring wszTooltip;
		CDBPtr<NDb::STexture> pDisabledIcon;
		int nLevel;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszName); f.Add(3,&wszDesc); f.Add(4,&pEnabledIcon); f.Add(5,&wszTooltip); f.Add(6,&pDisabledIcon); f.Add(7,&nLevel); return 0; }
	};

	ZDATA
	NDb::EReinforcementType eType;
	CDBPtr<NDb::SReinforcement> pDBReinf;
	CDBPtr<NDb::STexture> pTexture;
	ZSKIP //wstring wszLine01;
	ZSKIP //wstring wszLine02;
	CObj<CLeaderInfo> pLeader;
	bool bEnabled;
	std::wstring wszName;
	std::wstring wszProfileUnit;
	CPtr<IListControlItem> pItem;
	bool bFromPrevChapter;
	std::wstring wszProfileUnitRank;
	std::vector<SAbility> abilities;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eType); f.Add(3,&pDBReinf); f.Add(4,&pTexture); f.Add(7,&pLeader); f.Add(8,&bEnabled); f.Add(9,&wszName); f.Add(10,&wszProfileUnit); f.Add(11,&pItem); f.Add(12,&bFromPrevChapter); f.Add(13,&wszProfileUnitRank); f.Add(14,&abilities); return 0; }

	void FillAbilities();
};

class CInterfaceArmyScreen::CLeaderInfo : public CObjectBase
{
	OBJECT_BASIC_METHODS( CLeaderInfo );
public:
	struct SLastSeen
	{
		enum EState
		{
			ST_NONE,
			ST_EXP,
			ST_RANK,
			ST_LOST,
			ST_KILLED,
			ST_DONE,
		};

		ZDATA
		EState eState;
		bool bExpChanged;
		bool bRankChanged;
		bool bLostChanged;
		bool bKilledChanged;
		float fCurExp;
		int nCurRank;
		int nCurLost;
		int nCurKilled;
		int nWaitTime;
		int fExpStep;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&bExpChanged); f.Add(4,&bRankChanged); f.Add(5,&bLostChanged); f.Add(6,&bKilledChanged); f.Add(7,&fCurExp); f.Add(8,&nCurRank); f.Add(9,&nCurLost); f.Add(10,&nCurKilled); f.Add(11,&nWaitTime); f.Add(12,&fExpStep); return 0; }
		
		SLastSeen() : eState( ST_NONE ) {}
	};
	
	ZDATA
	std::wstring wszName;
	CDBPtr<NDb::STexture> pIcon;
	float fExp; // [0..1]
	ZSKIP //wstring wszRank;
	std::wstring wszSpecialization;
	int nKilled;
	int nLost;
	ZSKIP //vector<SAbility> abilities;
	bool bPermanent;
	IScenarioTracker::SUndoLeaderInfo undo;
	int nRank;
	SLastSeen lastSeen;
	NDb::EReinforcementType eType;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszName); f.Add(3,&pIcon); f.Add(4,&fExp); f.Add(6,&wszSpecialization); f.Add(7,&nKilled); f.Add(8,&nLost); f.Add(10,&bPermanent); f.Add(11,&undo); f.Add(12,&nRank); f.Add(13,&lastSeen); f.Add(14,&eType); return 0; }
};

class CInterfaceArmyScreen::CReinfViewer : public IDataViewer
{
	OBJECT_BASIC_METHODS( CReinfViewer );
public:
	//{ IDataViewer
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
	//}
};

// CInterfaceArmyScreen::CReinfData

void CInterfaceArmyScreen::CReinfData::FillAbilities()
{
	const NDb::SUnitBaseRPGStats *pDBUnit = 0;
	if ( pDBReinf && !pDBReinf->entries.empty() )
	{
		const NDb::SReinforcementEntry &entry = pDBReinf->entries.front();
		if ( entry.pMechUnit )
			pDBUnit = entry.pMechUnit;
		else if ( entry.pSquad && !entry.pSquad->members.empty() )
			pDBUnit = entry.pSquad->members.front();
	}
	if ( pDBUnit && pDBUnit->pActions )
	{
		const NDb::SUnitActions *pActions = pDBUnit->pActions;
		for ( int i = 0; i < pActions->specialAbilities.size(); ++i )
		{
			const NDb::SUnitSpecialAblityDesc *pAbility = pActions->specialAbilities[i];
			if ( pAbility )
			{
				NDb::EUserAction eAction = GetActionByAbility( pAbility->eName );
				SAbility ability;
				ability.nLevel = i;
				if ( CHECK_TEXT_NOT_EMPTY_PRE(pAbility->,LocalizedName) )
					ability.wszName = GET_TEXT_PRE(pAbility->,LocalizedName);
				if ( CHECK_TEXT_NOT_EMPTY_PRE(pAbility->,LocalizedDesc) )
					ability.wszDesc = GET_TEXT_PRE(pAbility->,LocalizedDesc);

				const NDb::SUIConstsB2 *pUIConsts = InterfaceState()->GetUIConsts();
				for ( int j = 0; j < pUIConsts->actionButtonInfos.size(); ++j )
				{
					const NDb::SActionButtonInfo *pActionButtonInfo = pUIConsts->actionButtonInfos[j];
					if ( pActionButtonInfo )
					{
						if ( pActionButtonInfo->eAction == eAction )
						{
							ability.pEnabledIcon = pActionButtonInfo->pIcon;
							ability.pDisabledIcon = pActionButtonInfo->pIconDisabled;
							if ( CHECK_TEXT_NOT_EMPTY_PRE(pActionButtonInfo->,Tooltip) )
								ability.wszTooltip = GET_TEXT_PRE(pActionButtonInfo->,Tooltip);
							break;
						}
					}
				}

				abilities.push_back( ability );
			}
		}
	}
}

// CReinfViewer

void CInterfaceArmyScreen::CReinfViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *_pData ) const
{
	CDynamicCast<IWindow> pWnd = pWindow;
	CDynamicCast<CReinfData> pData = _pData;
	if ( pWnd && pData )
	{
		IWindow *pIcon = GetChildChecked<IWindow>( pWnd, "Icon", true );
		ITextView *pLine01View = GetChildChecked<ITextView>( pWnd, "ReinfNameView", true );
		ITextView *pAssignedView = GetChildChecked<ITextView>( pWnd, "Assigned", true );
		ITextView *pUnassignedView = GetChildChecked<ITextView>( pWnd, "Unassigned", true );
		
		if ( pIcon )
			pIcon->SetTexture( pData->pTexture );
		if ( pLine01View )
		{
			std::wstring wszLine = pData->wszName;
			pLine01View->SetText( pLine01View->GetDBText() + wszLine );
		}
		if ( pAssignedView )
			pAssignedView->ShowWindow( pData->pLeader != 0 );
		if ( pUnassignedView )
			pUnassignedView->ShowWindow( pData->pLeader == 0 );
	}
}

// CInterfaceArmyScreen

CInterfaceArmyScreen::CInterfaceArmyScreen() :
	CInterfaceScreenBase( "ArmyScreen", "army_screen_menu" ),
	timeAbs( 0 )
{
}

CInterfaceArmyScreen::~CInterfaceArmyScreen()
{
	pReinfViewer = 0;
	reinforcements.clear();
}

bool CInterfaceArmyScreen::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );

	MakeInterior();
	
	CheckAutoAssign();
	
	InterfaceState()->SetAutoShowCommanderScreen( false );

	return true;
}

void CInterfaceArmyScreen::GetElements()
{
	pMain = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	pAutoAssignDlg = GetChildChecked<IWindow>( GetScreen(), "AutoAssignDlg", true );
	pUndoPromotionsDlg = GetChildChecked<IWindow>( GetScreen(), "UndoPromotionsDlg", true );
	pUndoAllPromotionsDlg = GetChildChecked<IWindow>( GetScreen(), "UndoAllPromotionsDlg", true );
	if ( pAutoAssignDlg )
		pAutoAssignDlg->ShowWindow( false );
	if ( pUndoPromotionsDlg )
		pUndoPromotionsDlg->ShowWindow( false );
	if ( pUndoAllPromotionsDlg )
		pUndoAllPromotionsDlg->ShowWindow( false );
	
	pLeftPanel = GetChildChecked<IWindow>( pMain, "LeftPanel", true );
	pRightPanel = GetChildChecked<IWindow>( pMain, "RightPanel", true );
	pTopPanel = GetChildChecked<IWindow>( pMain, "TopPanel", true );
	pBottomPanel = GetChildChecked<IWindow>( pMain, "BottomPanel", true );

	pUndoBtn = GetChildChecked<IButton>( pBottomPanel, "UndoBtn", true );
	if ( pUndoBtn )
		pUndoBtn->Enable( false );

	pCommanderInfoBlock = GetChildChecked<IWindow>( pLeftPanel, "CommanderInfo", true );
	pReinfCurrentBlock = GetChildChecked<IWindow>( pLeftPanel, "ReinfCurrent", true );
	pPictureBlock = GetChildChecked<IWindow>( pLeftPanel, "PictureBlock", true );
	
	pReinfList = GetChildChecked<IListControl>( pRightPanel, "ArmyList", true );

	pPromoteCommanderView = GetChildChecked<ITextView>( pLeftPanel, "PromoteCommanderView", true );
	pPromotionsView = GetChildChecked<ITextView>( pLeftPanel, "PromotionsAvailableView", true );
	
	pPermanentCommanderWnd = GetChildChecked<IWindow>( pPictureBlock, "Permanent", true );
	pPermanentCommanderIcon = GetChildChecked<IWindow>( pPermanentCommanderWnd, "Icon", true );
	pSelectCommanderBtn = GetChildChecked<IButton>( pPictureBlock, "SelectCommanderBtn", true );
	pSelectCommanderIcon = GetChildChecked<IWindow>( pSelectCommanderBtn, "Icon", true );
	pUnselectCommanderBtn = GetChildChecked<IButton>( pPictureBlock, "UnselectCommanderBtn", true );
	pUnselectCommanderIcon = GetChildChecked<IWindow>( pUnselectCommanderBtn, "Icon", true );
	
	pLeaderNameView = GetChildChecked<ITextView>( pCommanderInfoBlock, "NameView", true );
	pLeaderExpBar = GetChildChecked<IProgressBar>( pCommanderInfoBlock, "ExpBar", true );
	pLeaderRankView = GetChildChecked<ITextView>( pCommanderInfoBlock, "RankView", true );
	pLeaderRankLabel = GetChildChecked<ITextView>( pCommanderInfoBlock, "RankLabel", true );
	pLeaderKilledView = GetChildChecked<ITextView>( pCommanderInfoBlock, "KilledView", true );
	pLeaderLostView = GetChildChecked<ITextView>( pCommanderInfoBlock, "LostView", true );
	
	for ( int i = 1; ; ++i )
	{
		IWindow *pWnd = pLeftPanel ? pLeftPanel->GetChild( fmt::format( "Ability{}", i ), true ) : 0;
		if ( !pWnd )
			break;

		SVisAbility visAbility;
		visAbility.pWnd = pWnd;
		visAbility.pIconWnd = GetChildChecked<IWindow>( pWnd, "Icon", true );
		visAbility.pNameView = GetChildChecked<ITextView>( pWnd, "AbilityNameView", true );
		visAbility.pRankLabel = GetChildChecked<ITextView>( pWnd, "RankLabel", true );
		visAbilities.push_back( visAbility );
	}
	
	pAssignLeaderDlg = GetChildChecked<IWindow>( GetScreen(), "AssignLeaderDlg", true );
	pAssignLeaderEdit = GetChildChecked<IEditLine>( pAssignLeaderDlg, "AssignLeaderNameEdit", true );

	pReinfInfoNameView = GetChildChecked<ITextView>( pReinfCurrentBlock, "Name", true );
	pReinfInfoIcon = GetChildChecked<IWindow>( pReinfCurrentBlock, "Icon", true );
	pProfileUnitNameView = GetChildChecked<ITextView>( pReinfCurrentBlock, "ProfileUnitName", true );
	pProfileUnitExpView = GetChildChecked<ITextView>( pReinfCurrentBlock, "ReinfCurrentExpView", true );
}

void CInterfaceArmyScreen::MakeInterior()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();

	GetElements();

	pReinfViewer = new CReinfViewer();
	if ( pReinfList )
		pReinfList->SetViewer( pReinfViewer );
	
	const int nLocalPlayer = 0;
	const NDb::SUIConstsB2 *pUIConsts = InterfaceState()->GetUIConsts();
	std::vector<IScenarioTracker::SChapterReinf> reinfs;
	pST->GetChapterCurrentReinforcements( &reinfs, nLocalPlayer );
	for ( int i = 0; i < reinfs.size(); ++i )
	{
		IScenarioTracker::SChapterReinf &reinf = reinfs[i];
		if ( reinf.eState == IScenarioTracker::ERS_DISABLED )
			continue;

		CPtr<CReinfData> pReinf = new CReinfData();
		pReinf->eType = (NDb::EReinforcementType)( i );
		pReinf->bEnabled = (reinf.eState == IScenarioTracker::ERS_ENABLED);
		const NDb::SReinforcement *pDBReinf = reinf.pDBReinf;
		pReinf->pDBReinf = pDBReinf;
		pReinf->bFromPrevChapter = reinf.bFromPrevChapter;
		pReinf->FillAbilities();
		if ( pDBReinf )
		{
			if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBReinf->,LocalizedName) )
			{
				pReinf->wszName = GET_TEXT_PRE(pDBReinf->,LocalizedName);
				if ( !pDBReinf->entries.empty() )
				{
					const NDb::SReinforcementEntry &entry = pDBReinf->entries.front();
					const NDb::SHPObjectRPGStats *pDBStats = (entry.pMechUnit != 0) ? 
						checked_cast_ptr<const NDb::SHPObjectRPGStats*>( entry.pMechUnit ) : 
						checked_cast_ptr<const NDb::SHPObjectRPGStats*>( entry.pSquad );
					if ( pDBStats && CHECK_TEXT_NOT_EMPTY_PRE(pDBStats->,LocalizedName) )
						pReinf->wszProfileUnit = GET_TEXT_PRE(pDBStats->,LocalizedName);
				}
			}
		}
		if ( reinf.eState == IScenarioTracker::ERS_NOT_ENABLED )
		{
			// icon
			if ( pUIConsts )
			{
				for ( std::vector< NDb::SReinfButton >::const_iterator it = pUIConsts->reinfButtons.begin();
					it != pUIConsts->reinfButtons.end(); ++it )
				{
					const NDb::SReinfButton &button = *it;
					if ( button.eType == pReinf->eType )
					{
						pReinf->pTexture = button.pTextureDisabled;
						break;
					}
				}
			}

			// line02
			if ( pReinf->bFromPrevChapter )
				pReinf->wszProfileUnitRank = InterfaceState()->GetTextEntry( "T_ARMY_SCREEN_REINF_UNAVAILABLE" );
			else
				pReinf->wszProfileUnitRank = NDBWrap::GetReinfXPLevelName( 0 );
		}
		else
		{
			if ( pDBReinf )
			{
				pReinf->pTexture = pDBReinf->pIconTexture;
				pReinf->wszProfileUnitRank = NDBWrap::GetReinfXPLevelName( pST->GetReinforcementXPLevel( nLocalPlayer, pReinf->eType ) );
			}
			
			IScenarioTracker::SUndoLeaderInfo undo;
			FillLeaderInfo( pReinf, undo );
			if ( pReinf && pReinf->pLeader )
				pReinf->pLeader->bPermanent = true;
		}

		if ( pReinf->bEnabled && !pReinf->bFromPrevChapter && // show enabled only
			pReinf->eType != NDb::RT_SUPER_WEAPON ) // CRAP - don't show super weapon
		{
			reinforcements.push_back( pReinf.GetPtr() );
		}
	}
	
	if ( pPromotionsView )
		pPromotionsView->SetText( pPromotionsView->GetDBText() + NStr::ToUnicode( std::to_string(  pST->GetAvailablePromotions() ) ) );

	for ( int i = 0; i < reinforcements.size(); ++i )
	{
		CReinfData *pReinf = reinforcements[i];
		
		pReinf->pItem = pReinfList->AddItem( pReinf );
	}
	pSelection = reinforcements.empty() ? 0 : reinforcements.front();
	if ( pSelection && pReinfList )
	{
		pReinfList->SelectItem( pSelection );
	}
	
	if ( IScreen *pScreen = GetScreen() )
		wszChangedValueTag = pScreen->GetTextEntry( "CHANGED_VALUE_TAG" );

	const NDb::SCampaign *pCampaign = pST->GetCurrentCampaign();
	for ( int i = 0; i < visAbilities.size(); ++i )
	{
		SVisAbility &visAbility = visAbilities[i];

		std::wstring wszTag = NStr::ToUnicode( fmt::format( DYNAMIC_TAG_RANK_NAME_FORMAT, i + 1 ) );
		std::wstring wszRankName;
		if ( i < pCampaign->leaderRanks.size() )
		{
			const NDb::SLeaderExpLevel &rank = pCampaign->leaderRanks[i];
			if ( CHECK_TEXT_NOT_EMPTY_PRE(rank.,RankName) )
				wszRankName = GET_TEXT_PRE(rank.,RankName);
		}

		std::vector< std::pair<std::wstring, std::wstring> > params;
		params.push_back( std::pair<std::wstring, std::wstring>( wszTag, wszRankName ) );
		SetDynamicTextView( visAbility.pRankLabel, params );
	}
	
	if ( pMain )
		SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );

	nAssignmentCount = 0;
	UpdateSelectionInfo();
}

void CInterfaceArmyScreen::FillLeaderInfo( CReinfData *pReinf, const IScenarioTracker::SUndoLeaderInfo &undo )
{
	if ( !pReinf )
		return;
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const IScenarioTracker::SLeaderInfo *pSTLeader = pST->GetLeaderInfo( pReinf->eType );
	if ( pSTLeader )
	{
		CLeaderInfo *pLeader = new CLeaderInfo();
		pLeader->eType = pReinf->eType;
		pLeader->undo = undo;
		pReinf->pLeader = pLeader;

		pLeader->bPermanent = false;
		pLeader->wszName = pSTLeader->wszName;
		pLeader->pIcon = pSTLeader->pPicture;
		pLeader->fExp = pSTLeader->info.fExp;

		pLeader->nRank = pSTLeader->info.nRank;
		pLeader->wszSpecialization = pReinf->wszName;
		pLeader->nKilled = pSTLeader->info.nUnitsKilled;
		pLeader->nLost = pSTLeader->info.nUnitsLost;
		
		// fill last seen info
		pLeader->lastSeen.fCurExp = pSTLeader->lastSeenInfo.fExp;
		pLeader->lastSeen.nCurRank = pSTLeader->lastSeenInfo.nRank;
		pLeader->lastSeen.nCurLost = pSTLeader->lastSeenInfo.nUnitsLost;
		pLeader->lastSeen.nCurKilled = pSTLeader->lastSeenInfo.nUnitsKilled;

		if ( pLeader->lastSeen.fCurExp != pLeader->fExp )
		{
			pLeader->lastSeen.bExpChanged = true;
			pLeader->lastSeen.fExpStep = (pLeader->fExp - pLeader->lastSeen.fCurExp) * EXP_PROGRESS_STEP_FRACTION;
			if ( pLeader->lastSeen.fExpStep < 1 )	
				pLeader->lastSeen.fExpStep = 1;
		}
		else
			pLeader->lastSeen.bExpChanged = false;
		if ( pLeader->lastSeen.nCurRank != pLeader->nRank )
			pLeader->lastSeen.bRankChanged = true;
		else
			pLeader->lastSeen.bRankChanged = false;
		if ( pLeader->lastSeen.nCurLost != pLeader->nLost )
			pLeader->lastSeen.bLostChanged = true;
		else
			pLeader->lastSeen.bLostChanged = false;
		if ( pLeader->lastSeen.nCurKilled != pLeader->nKilled )
			pLeader->lastSeen.bKilledChanged = true;
		else
			pLeader->lastSeen.bKilledChanged = false;

		if ( pLeader->lastSeen.bExpChanged )
			pLeader->lastSeen.eState = CLeaderInfo::SLastSeen::ST_EXP;
		else if ( pLeader->lastSeen.bRankChanged )
			pLeader->lastSeen.eState = CLeaderInfo::SLastSeen::ST_RANK;
		else if ( pLeader->lastSeen.bLostChanged )
			pLeader->lastSeen.eState = CLeaderInfo::SLastSeen::ST_LOST;
		else if ( pLeader->lastSeen.bKilledChanged )
			pLeader->lastSeen.eState = CLeaderInfo::SLastSeen::ST_KILLED;

		pLeader->lastSeen.nWaitTime = WAIT_TIME;
	}
}

void CInterfaceArmyScreen::ShowLeaderInfo( bool bEnabled, const CLeaderInfo *pLeader, bool bExist )
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	int nPromotionCount = pST->GetAvailablePromotions();
	bool bAssigned = (pLeader != 0);


	if ( pPromoteCommanderView )
		pPromoteCommanderView->ShowWindow( !bAssigned && nPromotionCount > 0 );
	if ( pPromotionsView )
	{
		pPromotionsView->ShowWindow( !bAssigned );
		pPromotionsView->SetText( pPromotionsView->GetDBText() + NStr::ToUnicode( std::to_string( nPromotionCount ) ) );
	}
	if ( pCommanderInfoBlock )
		pCommanderInfoBlock->ShowWindow( bAssigned );
	if ( pPictureBlock )
		pPictureBlock->ShowWindow( bAssigned || nPromotionCount > 0 );

	if ( pPermanentCommanderWnd )
		pPermanentCommanderWnd->ShowWindow( false );
	if ( pSelectCommanderBtn )
		pSelectCommanderBtn->ShowWindow( false );
	if ( pUnselectCommanderBtn )
		pUnselectCommanderBtn->ShowWindow( false );

	if ( bAssigned )
	{
		if ( pLeader->bPermanent )
		{
			if ( pPermanentCommanderWnd )
				pPermanentCommanderWnd->ShowWindow( true );
			if ( pPermanentCommanderIcon )
				pPermanentCommanderIcon->SetTexture( pLeader->pIcon );
		}
		else
		{
			if ( pUnselectCommanderBtn )
				pUnselectCommanderBtn->ShowWindow( true );
			if ( pUnselectCommanderIcon )
				pUnselectCommanderIcon->SetTexture( pLeader->pIcon );
		}
	}
	else
	{
		if ( pSelectCommanderBtn )
			pSelectCommanderBtn->ShowWindow( true );
		if ( pSelectCommanderIcon )
			pSelectCommanderIcon->ShowWindow( false );
	}

	UpdateLeaderVisualInfo( pLeader );
}

void CInterfaceArmyScreen::ShowReinfAbilities( bool bEnabled, const CReinfData *pReinf, const CLeaderInfo *pLeader )
{
	if ( !pReinf )
		return;

	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const IScenarioTracker::SLeaderInfo *pSTLeader = pST->GetLeaderInfo( pReinf->eType );

	for ( int i = 0; i < visAbilities.size(); ++i )
	{
		SVisAbility &visAbility = visAbilities[i];

		const NDb::STexture *pIcon = 0;
		if ( i < pReinf->abilities.size() )
		{
			const CReinfData::SAbility &ability = pReinf->abilities[i];

			if ( visAbility.pWnd )
			{
				visAbility.pWnd->ShowWindow( true );
				
				std::vector< std::pair<std::wstring, std::wstring> > params;
				params.push_back( std::pair<std::wstring, std::wstring>( TEXT_TAG_ABILITY_TOOLTIP, ability.wszTooltip ) );
				SetDynamicTooltip( visAbility.pWnd, visAbility.pWnd->GetDBTooltipStr(), params );
			}

			if ( ability.nLevel == 0 || (pSTLeader && ability.nLevel <= pSTLeader->info.nRank ) )
				pIcon = ability.pEnabledIcon;
			else
				pIcon = ability.pDisabledIcon;
			if ( visAbility.pIconWnd )
			{
				visAbility.pIconWnd->SetTexture( pIcon );
			}

			if ( visAbility.pNameView )
				visAbility.pNameView->SetText( visAbility.pNameView->GetDBText() + ability.wszName );
		}
		else
		{
			if ( visAbility.pWnd )
				visAbility.pWnd->ShowWindow( false );
		}
	}
}

struct SDBUnit
{
	int nCount;
	const NDb::SHPObjectRPGStats *pDBStats;
	
	bool operator==( const NDb::SHPObjectRPGStats *_pDBStats ) const
	{
		return pDBStats == _pDBStats;
	}
};

void CInterfaceArmyScreen::ShowReinfInfo( bool bEnabled, const CReinfData *pReinf )
{
	if ( !pReinf )
		return;

	if ( pReinfInfoNameView )
	{
		pReinfInfoNameView->ShowWindow( true );
		pReinfInfoNameView->SetText( pReinfInfoNameView->GetDBText() + pReinf->wszName );
	}
	if ( pReinfInfoIcon )
	{
		pReinfInfoIcon->ShowWindow( true );
		pReinfInfoIcon->SetTexture( pReinf->pTexture );
	}
	if ( pProfileUnitNameView )
	{
		pProfileUnitNameView->ShowWindow( true );
		pProfileUnitNameView->SetText( pProfileUnitNameView->GetDBText() + pReinf->wszProfileUnit );
	}
	if ( pProfileUnitExpView )
	{
		pProfileUnitExpView->ShowWindow( true );
		pProfileUnitExpView->SetText( pProfileUnitExpView->GetDBText() + pReinf->wszProfileUnitRank );
	}
}

void CInterfaceArmyScreen::UpdateSelectionInfo()
{
	if ( !pSelection )
	{
		ShowNoSelection();
		return;
	}

	ShowLeaderInfo( pSelection->bEnabled, pSelection->pLeader, true );
	ShowReinfInfo( pSelection->bEnabled, pSelection );
	ShowReinfAbilities( pSelection->bEnabled, pSelection, pSelection->pLeader );
}

bool CInterfaceArmyScreen::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "menu_back" )
		return OnBack();
	if ( szReaction == "reinf_list_select" )
		return OnSelect();
	if ( szReaction == "set_leader" )
		return OnSetLeader();
	if ( szReaction == "assign_leader_ok" )
		return OnAssignLeaderOk();
	if ( szReaction == "assign_leader_cancel" )
		return OnAssignLeaderCancel();
	if ( szReaction == "edit_line_esc" )
		return OnAssignLeaderCancel();

	if ( szReaction == "menu_undo" )
		return OnMenuUndo();
	if ( szReaction == "undo_assign_commander" )
		return OnUndoAssignCommander();

	if ( szReaction == "auto_assign_ok" )
		return OnAutoAssignOk();
	if ( szReaction == "auto_assign_cancel" )
		return OnAutoAssignCancel();
	if ( szReaction == "undo_promotions_ok" )
		return OnUndoPromotionsOk();
	if ( szReaction == "undo_promotions_cancel" )
		return OnUndoPromotionsCancel();
	if ( szReaction == "undo_all_promotions_ok" )
		return OnUndoAllPromotionsOk();
	if ( szReaction == "undo_all_promotions_cancel" )
		return OnUndoAllPromotionsCancel();

	return false;
}

int CInterfaceArmyScreen::Check( const std::string &szCheckName ) const
{
	return 0;
}

bool CInterfaceArmyScreen::CheckAutoAssign()
{
	bool bAutoAssign = (Singleton<IScenarioTracker>()->GetAvailablePromotions() > 0);
	if ( bAutoAssign )
	{
		bAutoAssign = false;
		for ( int i = 0; i < reinforcements.size(); ++i )
		{
			CReinfData *pData = reinforcements[i];
			if ( pData->bEnabled && !pData->pLeader )
			{
				bAutoAssign = true;
				break;
			}
		}
	}
	if ( bAutoAssign )
	{
		if ( pAutoAssignDlg )
			pAutoAssignDlg->ShowWindow( true );
	}
	return bAutoAssign;
}

bool CInterfaceArmyScreen::OnBack()
{
	Back();

	return true;
}

bool CInterfaceArmyScreen::OnSelect()
{
	pSelection = 0;
	if ( pReinfList )
	{
		IListControlItem *pItem = pReinfList->GetSelectedListItem();
		pSelection = dynamic_cast<CReinfData*>( pItem->GetUserData() );
	}
	UpdateSelectionInfo();

	return true;
}

bool CInterfaceArmyScreen::OnSetLeader()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	if ( pSelection && !pSelection->pLeader && pST->GetAvailablePromotions() > 0 )
	{
		pST->AutoGenerateLeaderInfo( &generateLeaderInfo );
		generateLeaderInfo.wszFullName = generateLeaderInfo.wszName;

		if ( pAssignLeaderEdit )
			pAssignLeaderEdit->SetText( generateLeaderInfo.wszFullName.c_str() );
		if ( pAssignLeaderDlg )
			pAssignLeaderDlg->ShowWindow( true );
	}
	
	return true;
}

bool CInterfaceArmyScreen::OnAssignLeaderOk()
{
	if ( pSelection )
	{
		IScenarioTracker *pST = Singleton<IScenarioTracker>();
		if ( pAssignLeaderEdit )
			generateLeaderInfo.wszFullName = pAssignLeaderEdit->GetText();
		IScenarioTracker::SUndoLeaderInfo undo;
		pST->AssignLeader( pSelection->eType, generateLeaderInfo, &undo );
		const IScenarioTracker::SLeaderInfo *pSTLeader = pST->GetLeaderInfo( pSelection->eType );
		if ( pSTLeader )
			pST->SetLeaderLastSeenInfo( pSelection->eType, pSTLeader->info );
		nAssignmentCount++;
		FillLeaderInfo( pSelection, undo );

		if ( pPromotionsView )
			pPromotionsView->SetText( pPromotionsView->GetDBText() + NStr::ToUnicode( std::to_string( pST->GetAvailablePromotions() ) ) );

		UpdateSelectionInfo();

		pReinfViewer->MakeInterior( pSelection->pItem, pSelection );

		if ( pUndoBtn )
			pUndoBtn->Enable( nAssignmentCount > 0 );
	}

	if ( pAssignLeaderDlg )
		pAssignLeaderDlg->ShowWindow( false );
		
	return true;
}

bool CInterfaceArmyScreen::OnAssignLeaderCancel()
{
	if ( pAssignLeaderDlg )
		pAssignLeaderDlg->ShowWindow( false );
		
	return true;
}

void CInterfaceArmyScreen::AutoAssignCommanders()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	while ( pST->GetAvailablePromotions() > 0 )
	{
		bool bFound = false;
		for ( int i = 0; i < reinforcements.size(); ++i )
		{
			CReinfData *pReinf = reinforcements[i];
			if ( pReinf->bEnabled && !pReinf->pLeader )
			{
				bFound = true;

				IScenarioTracker::SGenerateLeaderInfo leader;
				pST->AutoGenerateLeaderInfo( &leader );
				leader.wszFullName = leader.wszName;
				IScenarioTracker::SUndoLeaderInfo undo;
				pST->AssignLeader( pReinf->eType, leader, &undo );
				const IScenarioTracker::SLeaderInfo *pSTLeader = pST->GetLeaderInfo( pReinf->eType );
				if ( pSTLeader )
					pST->SetLeaderLastSeenInfo( pReinf->eType, pSTLeader->info );

				nAssignmentCount++;
				FillLeaderInfo( pReinf, undo );

				pReinfViewer->MakeInterior( pReinf->pItem, pReinf );

				if ( pUndoBtn )
					pUndoBtn->Enable( nAssignmentCount > 0 );

				break;
			}
		}
		if ( !bFound )
			break;
	}

	UpdateSelectionInfo();

	if ( pPromotionsView )
		pPromotionsView->SetText( pPromotionsView->GetDBText() + NStr::ToUnicode( std::to_string( pST->GetAvailablePromotions() ) ) );
}

void CInterfaceArmyScreen::Back()
{
	// save leaders' last seen info
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	for ( int i = 0; i < reinforcements.size(); ++i )
	{
		CReinfData *pReinf = reinforcements[i];
		CLeaderInfo *pLeader = pReinf->pLeader;
		if ( !pLeader )
			continue;
		const IScenarioTracker::SLeaderInfo *pSTLeader = pST->GetLeaderInfo( pReinf->eType );
		if ( !pSTLeader )
			continue;
		if ( pLeader->lastSeen.eState != CLeaderInfo::SLastSeen::ST_NONE && 
			pLeader->lastSeen.eState != CLeaderInfo::SLastSeen::ST_DONE )
		{
			IScenarioTracker::SLeaderInfo::SLeaderStatSet info;
			info.nRank = pLeader->lastSeen.nCurRank;
			info.fExp = pLeader->lastSeen.fCurExp;
			info.nUnitsLost = pLeader->lastSeen.nCurLost;
			info.nUnitsKilled = pLeader->lastSeen.nCurKilled;
			pST->SetLeaderLastSeenInfo( pReinf->eType, info );
		}
		else
			pST->SetLeaderLastSeenInfo( pReinf->eType, pSTLeader->info );
	}

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NInput::PostEvent( "menu_return_from_subscreen", 0, 0 );
}

void CInterfaceArmyScreen::ShowNoSelection()
{
	if ( pPromoteCommanderView )
		pPromoteCommanderView->ShowWindow( false );
	if ( pPromotionsView )
		pPromotionsView->ShowWindow( false );
	if ( pCommanderInfoBlock )
		pCommanderInfoBlock->ShowWindow( false );

	if ( pPermanentCommanderWnd )
		pPermanentCommanderWnd->ShowWindow( false );
	if ( pSelectCommanderBtn )
		pSelectCommanderBtn->ShowWindow( false );
	if ( pUnselectCommanderBtn )
		pUnselectCommanderBtn->ShowWindow( false );
		
	if ( pReinfCurrentBlock )
		pReinfCurrentBlock->ShowWindow( false );
	if ( pPictureBlock )
		pPictureBlock->ShowWindow( false );

	for ( int i = 0; i < visAbilities.size(); ++i )
	{
		SVisAbility &visAbility = visAbilities[i];
		
		if ( visAbility.pWnd )
			visAbility.pWnd->ShowWindow( false );
	}
}

bool CInterfaceArmyScreen::OnMenuUndo()
{
	if ( pUndoAllPromotionsDlg )
		pUndoAllPromotionsDlg->ShowWindow( true );

	return true;
}

void CInterfaceArmyScreen::UndoAllAssignCommander()
{
	if ( nAssignmentCount > 0 )
	{
		IScenarioTracker *pST = Singleton<IScenarioTracker>();

		for ( int i = 0; i < reinforcements.size(); ++i )
		{
			CReinfData *pReinf = reinforcements[i];
			
			if ( pReinf->pLeader && !pReinf->pLeader->bPermanent )
			{
				pST->UndoAssignLeader( pReinf->pLeader->undo ) ;
				pReinf->pLeader = 0;

				pReinfViewer->MakeInterior( pReinf->pItem, pReinf );
			}
		}

		if ( pPromotionsView )
			pPromotionsView->SetText( pPromotionsView->GetDBText() + NStr::ToUnicode( std::to_string( pST->GetAvailablePromotions() ) ) );

		UpdateSelectionInfo();

		nAssignmentCount = 0;
		if ( pUndoBtn )
			pUndoBtn->Enable( nAssignmentCount > 0 );
	}
}

bool CInterfaceArmyScreen::OnUndoAssignCommander()
{
	if ( pUndoPromotionsDlg )
		pUndoPromotionsDlg->ShowWindow( true );
	return true;
}

void CInterfaceArmyScreen::UndoAssignCommander()
{
	if ( pSelection && pSelection->pLeader && !pSelection->pLeader->bPermanent )
	{
		IScenarioTracker *pST = Singleton<IScenarioTracker>();

		pST->UndoAssignLeader( pSelection->pLeader->undo ) ;
		nAssignmentCount--;
		pSelection->pLeader = 0;

		if ( pPromotionsView )
			pPromotionsView->SetText( pPromotionsView->GetDBText() + NStr::ToUnicode( std::to_string(  pST->GetAvailablePromotions() ) ) );

		UpdateSelectionInfo();

		pReinfViewer->MakeInterior( pSelection->pItem, pSelection );
		
		if ( pUndoBtn )
			pUndoBtn->Enable( nAssignmentCount > 0 );
	}
}

bool CInterfaceArmyScreen::OnAutoAssignOk()
{
	if ( pAutoAssignDlg )
		pAutoAssignDlg->ShowWindow( false );
	AutoAssignCommanders();
	return true;
}

bool CInterfaceArmyScreen::OnAutoAssignCancel()
{
	if ( pAutoAssignDlg )
		pAutoAssignDlg->ShowWindow( false );
	return true;
}

bool CInterfaceArmyScreen::OnUndoPromotionsOk()
{
	if ( pUndoPromotionsDlg )
		pUndoPromotionsDlg->ShowWindow( false );
	UndoAssignCommander();
	return true;
}

bool CInterfaceArmyScreen::OnUndoPromotionsCancel()
{
	if ( pUndoPromotionsDlg )
		pUndoPromotionsDlg->ShowWindow( false );
	return true;
}

bool CInterfaceArmyScreen::OnUndoAllPromotionsOk()
{
	if ( pUndoAllPromotionsDlg )
		pUndoAllPromotionsDlg->ShowWindow( false );
	UndoAllAssignCommander();
	return true;
}

bool CInterfaceArmyScreen::OnUndoAllPromotionsCancel()
{
	if ( pUndoAllPromotionsDlg )
		pUndoAllPromotionsDlg->ShowWindow( false );
	return true;
}

bool CInterfaceArmyScreen::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );
	
	if ( timeAbs == 0 )
	{
		timeAbs = Singleton<IGameTimer>()->GetAbsTime();
		return bResult;
	}
	
	NTimer::STime timeAbsCur = Singleton<IGameTimer>()->GetAbsTime();
	int nDeltaTime = timeAbsCur - timeAbs;
	timeAbs = timeAbsCur;
	if ( pSelection && IsMainScreenActive() )
	{
		CLeaderInfo *pLeader = pSelection->pLeader;
		if ( pLeader )
		{
			if ( pLeader->lastSeen.nWaitTime > nDeltaTime )
			{
				pLeader->lastSeen.nWaitTime -= nDeltaTime;
			}
			else
			{
				pLeader->lastSeen.nWaitTime = 0;
				switch ( pLeader->lastSeen.eState )
				{
					case CLeaderInfo::SLastSeen::ST_EXP:
					{
						if ( pLeader->lastSeen.fCurExp + pLeader->lastSeen.fExpStep >= pLeader->fExp )
						{
							pLeader->lastSeen.nWaitTime = WAIT_TIME;
							pLeader->lastSeen.eState = CLeaderInfo::SLastSeen::ST_RANK;
							pLeader->lastSeen.fCurExp = pLeader->fExp;
						}
						else
						{
							pLeader->lastSeen.nWaitTime = STEP_WAIT_TIME;
							pLeader->lastSeen.fCurExp += pLeader->lastSeen.fExpStep;
						}
						UpdateSelectedLeaderVisualInfo();
						break;
					}

					case CLeaderInfo::SLastSeen::ST_RANK:
					{
						pLeader->lastSeen.nWaitTime = WAIT_TIME;
						pLeader->lastSeen.eState = CLeaderInfo::SLastSeen::ST_LOST;
						pLeader->lastSeen.nCurRank = pLeader->nRank;
						UpdateSelectedLeaderVisualInfo();
						break;
					}

					case CLeaderInfo::SLastSeen::ST_LOST:
					{
						pLeader->lastSeen.nWaitTime = WAIT_TIME;
						pLeader->lastSeen.eState = CLeaderInfo::SLastSeen::ST_KILLED;
						pLeader->lastSeen.nCurLost = pLeader->nLost;
						UpdateSelectedLeaderVisualInfo();
						break;
					}

					case CLeaderInfo::SLastSeen::ST_KILLED:
					{
						pLeader->lastSeen.nWaitTime = WAIT_TIME;
						pLeader->lastSeen.eState = CLeaderInfo::SLastSeen::ST_DONE;
						pLeader->lastSeen.nCurKilled = pLeader->nKilled;
						UpdateSelectedLeaderVisualInfo();
						break;
					}
				}
			}
		}
	}
	return bResult;
}

void CInterfaceArmyScreen::UpdateSelectedLeaderVisualInfo()
{
	if ( pSelection )
	{
		UpdateLeaderVisualInfo( pSelection->pLeader );
	}
}

void CInterfaceArmyScreen::UpdateLeaderVisualInfo( const CLeaderInfo *pLeader )
{
	if ( !pLeader )
		return;

	IScenarioTracker *pST = Singleton<IScenarioTracker>();

	int nTempRank = pLeader->lastSeen.nCurRank;
	while ( nTempRank < pLeader->nRank )
	{
		float fNextLevExp = pST->GetReinforcementXPForLevel( pLeader->eType, nTempRank + 1 );
		if ( pLeader->lastSeen.fCurExp >= fNextLevExp )
			nTempRank++;
		else
			break;
	}

	float fCurLevExp = pST->GetReinforcementXPForLevel( pLeader->eType, nTempRank );
	float fNextLevExp = pST->GetReinforcementXPForLevel( pLeader->eType, nTempRank + 1 );
	float fDeltaExp = fNextLevExp - fCurLevExp;
	float fExpAtLevel = pLeader->lastSeen.fCurExp - fCurLevExp;
	if ( fDeltaExp > 0.0f )
		fExpAtLevel = Clamp( fExpAtLevel / fDeltaExp, 0.0f, 1.0f );
	else
		fExpAtLevel = fExpAtLevel > 0.0f ? 1.0f : 0.0f;

	if ( pLeaderNameView )
		pLeaderNameView->SetText( pLeaderNameView->GetDBText() + pLeader->wszName );

	if ( pLeaderExpBar )
		pLeaderExpBar->SetPosition( fExpAtLevel );

	std::wstring wszRank = pST->GetLeaderRankName( nTempRank );
	if ( pLeader->lastSeen.bRankChanged && pLeader->lastSeen.nCurRank == pLeader->nRank )
		wszRank = wszChangedValueTag + wszRank;
	if ( pLeaderRankView )
		pLeaderRankView->SetText( pLeaderRankView->GetDBText() + wszRank );

	std::wstring wszRankNumber = NStr::ToUnicode( std::to_string(  pLeader->nRank + 1 ) );
	std::vector< std::pair<std::wstring, std::wstring> > params;
	params.push_back( std::pair<std::wstring, std::wstring>( DYNAMIC_TAG_RANK_NUMBER, wszRankNumber ) );
	SetDynamicTextView( pLeaderRankLabel, params );
	
	std::wstring wszKilled = NStr::ToUnicode( std::to_string( pLeader->lastSeen.nCurKilled ) );
	if ( pLeader->lastSeen.bKilledChanged && pLeader->lastSeen.nCurKilled == pLeader->nKilled )
		wszKilled = wszChangedValueTag + wszKilled;
	if ( pLeaderKilledView )
		pLeaderKilledView->SetText( pLeaderKilledView->GetDBText() + wszKilled );

	std::wstring wszLost = NStr::ToUnicode( std::to_string( pLeader->lastSeen.nCurLost ) );
	if ( pLeader->lastSeen.bLostChanged && pLeader->lastSeen.nCurLost == pLeader->nLost )
		wszLost = wszChangedValueTag + wszLost;
	if ( pLeaderLostView )
		pLeaderLostView->SetText( pLeaderLostView->GetDBText() + wszLost );
}

bool CInterfaceArmyScreen::IsMainScreenActive() const
{
	if ( pAutoAssignDlg && pAutoAssignDlg->IsVisible() ) 
		return false;
	if ( pUndoPromotionsDlg && pUndoPromotionsDlg->IsVisible() )
		return false;
	if ( pUndoAllPromotionsDlg && pUndoAllPromotionsDlg->IsVisible() )
		return false;
	return true;
}

// CICArmyScreen

void CICArmyScreen::PreCreate()
{
}

void CICArmyScreen::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICArmyScreen::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x171BB441, CInterfaceArmyScreen )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_ARMY_SCREEN, CICArmyScreen )
REGISTER_SAVELOAD_CLASS_NM( GAMEX, 0x171C4C40, CReinfData, CInterfaceArmyScreen )
REGISTER_SAVELOAD_CLASS_NM( GAMEX, 0x171C4C41, CReinfViewer, CInterfaceArmyScreen )
REGISTER_SAVELOAD_CLASS_NM( GAMEX, 0x171C5440, CLeaderInfo, CInterfaceArmyScreen )


