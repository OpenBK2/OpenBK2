#include "StdAfx.h"
#include "InterfaceMissionObjectives.h"
#include "GameXClassIDs.h"
#include "../UI/SceneClassIDs.h"
#include "../SceneB2/Scene.h"
#include "../Misc/STrProc.h"
#include "InterfaceState.h"
#include "ScenarioTracker.h"
#include "../B2_M1_World/MissionObjectiveStates.h"
#include "../System/Text.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int N_OBJECTIVES_ROW_COUNT = 8;
const int MISSION_BRIEFING_ID = 1000000;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceMissionObjectives::CReactions::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "select_objective" )
	{
		int i = NStr::ToInt( szSender.substr( string( "ObjectiveItemButton" ).size() ) );
		NI_VERIFY( i >= 0, "Incorrect window name", return true );
		pInterface->UpdateObjectives( i );
		return true;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CInterfaceMissionObjectives::CReactions::Check( const string &szCheckName ) const
{
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CInterfaceMissionObjectives
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CInterfaceMissionObjectives::CInterfaceMissionObjectives() :
	CInterfaceScreenBase( "MissionObjectives", "mission_objectives" ),
	bIsModal( true ),
	bCameraBack( false )
{
	AddObserver( "menu_close", MsgBack );
	AddObserver( "set_modality", MsgSetModality );
	AddObserver( "mission_objectives_changed", MsgMissionObjectivesChanged );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CInterfaceMissionObjectives::~CInterfaceMissionObjectives()
{
	pReactions = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceMissionObjectives::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	// load screen
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	pReactions = new CReactions( pScreen, this );
	AddUIScreen( pScreen, "MissionObjectives", pReactions );
	
	pMainWnd = GetChildChecked<IWindow>( pScreen, "Objectives", true );

	if ( pMainWnd )
	{
		int nTop, nHeight;
		pMainWnd->GetPlacement( 0, &nTop, 0, &nHeight );
		fMainInitialTop = nTop;
		fMainInitialHeight = nHeight;
	}

	pObjectivesPanel = GetChildChecked<IWindow>( pMainWnd, "ObjectivesPanel", true );
	pHeader = GetChildChecked<ITextView>( pMainWnd, "Header", true );
	pDescPanel = GetChildChecked<IWindow>( pMainWnd, "DescPanel", true );
	pDescScrollableWnd = GetChildChecked<IScrollableContainer>( pMainWnd, "DescScrollable", true );
	
	if ( pObjectivesPanel )
	{
		int nHeight;
		pObjectivesPanel->GetPlacement( 0, 0, 0, &nHeight );
		fListInitialHeight = nHeight;
	}

	pObjectiveItem = GetChildChecked<IWindow>( pObjectivesPanel, "ObjectiveItem", true );
	pNextObjective = GetChildChecked<IWindow>( pObjectivesPanel, "NextObjective", true );
	
	if ( pObjectiveItem )
		pObjectiveItem->ShowWindow( false );
	if ( pNextObjective )
		pNextObjective->ShowWindow( false );

	pDescName = GetChildChecked<ITextView>( pMainWnd, "DescName", true );
	pDescBrief = GetChildChecked<ITextView>( pMainWnd, "DescBrief", true );
	pDescFull = GetChildChecked<ITextView>( pMainWnd, "DescFull", true );
	pDescStatus = GetChildChecked<IButton>( pMainWnd, "DescStatus", true );

	if ( pDescScrollableWnd )
	{
		if ( pDescBrief )
			pDescScrollableWnd->PushBack( pDescBrief, false );
		if ( pDescFull )
			pDescScrollableWnd->PushBack( pDescFull, false );
	}
	
	if ( pScreen )
	{
		wszObjectivesSummary = pScreen->GetTextEntry( "T_OBJECTIVES_SUMMARY" );
		wszObjectivesSummaryTooltip = pScreen->GetTextEntry( "T_OBJECTIVES_SUMMARY_TOOLTIP" );
	}

	const NDb::SMapInfo *pMapInfo = Singleton<IScenarioTracker>()->GetCurrentMission();
	if ( pMapInfo )
	{
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pMapInfo->,LocalizedDescription) )
			wszMissionBriefing = GET_TEXT_PRE(pMapInfo->,LocalizedDescription);
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pMapInfo->,LocalizedName) )
			wszMissionName = GET_TEXT_PRE(pMapInfo->,LocalizedName);
	}

	bIsModal = true;
	nPrevSelectionID = -1;

	MakeObjectives();
	UpdateKnownObjectives();
	UpdateObjectives( -1 );
	
	NInput::PostEvent( "blink_objective_button", 0, 0 );
	
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceMissionObjectives::MsgBack( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "minimap_hide_objectives" );
	
	if ( bCameraBack )
		NInput::PostEvent( "notifications_camera_back", 0, 0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceMissionObjectives::MsgSetModality( const SGameMessage &msg )
{
	bIsModal = (msg.nParam1 != 0);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceMissionObjectives::MsgMissionObjectivesChanged( const SGameMessage &msg )
{
	UpdateKnownObjectives();
	int nNewSelectionIndex = -1;
	for ( int i = 0; i < objectives.size(); ++i )
	{
		SObjective &objective = objectives[i];
		if ( objective.nID == nPrevSelectionID )
		{
			nNewSelectionIndex = i;
			break;
		}
	}
	UpdateObjectives( nNewSelectionIndex );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceMissionObjectives::MakeObjectives()
{
	objectives.resize( N_OBJECTIVES_ROW_COUNT );

	if ( pHeader )
		pHeader->SetText( pHeader->GetDBText() + wszMissionName );

	if ( pObjectivesPanel )
	{
		if ( pObjectiveItem )
			pObjectiveItem->ShowWindow( false );

		if ( pNextObjective )
			pNextObjective->ShowWindow( false );

		if ( pObjectiveItem && pNextObjective )
		{
			int nStartX, nStartY;
			pObjectiveItem->GetPlacement( &nStartX, &nStartY, 0, 0 );
			int nDeltaX, nDeltaY;
			pNextObjective->GetPlacement( &nDeltaX, &nDeltaY, 0, 0 );
			nDeltaX -= nStartX;
			nDeltaY -= nStartY;
			fItemHeight = nDeltaY;
			
			for ( int i = 0; i < N_OBJECTIVES_ROW_COUNT; ++i )
			{
				IWindow *pWnd = AddWindowCopy( pObjectivesPanel, pObjectiveItem );
				if ( !pWnd )
					continue;

				pWnd->ShowWindow( true );
				pWnd->SetPlacement( nStartX, nStartY + nDeltaY * i, 0, 0, EWPF_POS_X | EWPF_POS_Y );

				SObjective &objective = objectives[i];
				objective.pWnd = pWnd;
				objective.pText = GetChildChecked<ITextView>( pWnd, "ObjectiveItemDesc", true );
				objective.pStatus = GetChildChecked<IButton>( pWnd, "ObjectiveItemStatus", true );
				objective.pButton = GetChildChecked<IButton>( pWnd, "ObjectiveItemButton", true );
				if ( objective.pButton )
					objective.pButton->SetName( StrFmt( "ObjectiveItemButton%d", i ) );
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceMissionObjectives::UpdateKnownObjectives()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const NDb::SMapInfo *pDBInfo = pST->GetCurrentMission();
	if ( !pDBInfo )
		return;

	for ( int i = 0; i < objectives.size(); ++i )
	{
		SObjective &objective = objectives[i];
		objective.nID = -1;
	}

	int nIndex = 0;
	
	// special "objective" - mission briefing
	SObjective &objective = objectives[nIndex];
	nIndex++;
	objective.nID = MISSION_BRIEFING_ID;
	objective.bAutoSelect = false;
	objective.wszHeader = wszObjectivesSummary;
	objective.wszDescBrief = L"";
	objective.wszDescFull = wszMissionBriefing;
	objective.nButtonIndex = 0;
	if ( objective.pButton )
		objective.pButton->SetTooltip( wszObjectivesSummaryTooltip );

	const int known_count = pST->GetKnownObjectiveCount();
	for ( int i = 0; i < known_count; ++i )
	{
		int nID = pST->GetKnownObjectiveID( i );
		const NDb::SMissionObjective *pDBObjective = pDBInfo->objectives[nID];
		NI_VERIFY( pDBObjective, "No objective", continue );
		
		NI_VERIFY( nIndex < N_OBJECTIVES_ROW_COUNT, "Designers: too many objectives", break );
		
		SObjective &objective = objectives[nIndex];
		nIndex++;
		objective.nID = nID;
		objective.bAutoSelect = (pST->GetObjectiveState( nID ) == EMOS_RECEIVED);
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBObjective->,Header) )
			objective.wszHeader = GET_TEXT_PRE(pDBObjective->,Header);
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBObjective->,Briefing) )
			objective.wszDescBrief = GET_TEXT_PRE(pDBObjective->,Briefing);
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pDBObjective->,Description) )
			objective.wszDescFull = GET_TEXT_PRE(pDBObjective->,Description);
		objective.nButtonIndex = GetButtonIndex( pST->GetObjectiveState( nID ) );
		if ( objective.pButton )
			objective.pButton->SetTooltip( objective.pButton->GetDBTooltipStr() );
	}
	
	float fDeltaHeight = fItemHeight * (N_OBJECTIVES_ROW_COUNT - nIndex);
	if ( pMainWnd )
		pMainWnd->SetPlacement( 0, fMainInitialTop + fDeltaHeight * 0.5f, 0, fMainInitialHeight - fDeltaHeight, EWPF_POS_Y | EWPF_SIZE_Y );
	if ( pObjectivesPanel )
		pObjectivesPanel->SetPlacement( 0, 0, 0, fListInitialHeight - fDeltaHeight, EWPF_SIZE_Y );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceMissionObjectives::UpdateObjectives( int _nSelected )
{
	int nSelected = _nSelected;
	
	// Принудительно выберем элемент, если ничего не поселекчено
	if ( nSelected == -1 )
	{
		for ( int i = 0; i < objectives.size(); ++i )
		{
			SObjective &objective = objectives[i];
			if ( objective.nID != -1 && objective.bAutoSelect )
			{
				nSelected = i;
				break;
			}
		}
	}
	if ( nSelected == -1 )
	{
		for ( int i = 0; i < objectives.size(); ++i )
		{
			SObjective &objective = objectives[i];
			if ( objective.nID != -1 )
			{
				nSelected = i;
				break;
			}
		}
	}

	for ( int i = 0; i < objectives.size(); ++i )
	{
		SObjective &objective = objectives[i];

		const bool bIsObjectiveVisible = (objective.nID >= 0) && (i < N_OBJECTIVES_ROW_COUNT);

		if ( objective.pWnd )
			objective.pWnd->ShowWindow( bIsObjectiveVisible );

		if ( !bIsObjectiveVisible )
			continue;

		if ( objective.pText )
			objective.pText->SetText( objective.pText->GetDBText() + objective.wszHeader );

		if ( objective.pStatus )
		{
			objective.pStatus->SetStateWithVisual( objective.nButtonIndex );
			objective.pStatus->ShowWindow( objective.nID != MISSION_BRIEFING_ID );
		}

		if ( objective.pButton )
			objective.pButton->SetStateWithVisual( (nSelected == i) ? 1 : 0 );
	}

	const bool bIsSelectedObjective = (nSelected >= 0 && objectives[nSelected].nID >= 0);
	if ( pDescPanel )
		pDescPanel->ShowWindow( bIsSelectedObjective );
	
	if ( bIsSelectedObjective )
	{
		SObjective &objective = objectives[nSelected];

		if ( pDescName )
			pDescName->SetText( pDescName->GetDBText() + objective.wszHeader );

		int nSizeY = 0;
		if ( pDescBrief )
		{
			pDescBrief->SetText( pDescBrief->GetDBText() + objective.wszDescBrief );
			pDescBrief->GetPlacement( 0, 0, 0, &nSizeY );
		}

		if ( pDescFull )
		{
			pDescFull->SetText( pDescFull->GetDBText() + objective.wszDescFull );
			pDescFull->SetPlacement( 0, nSizeY, 0, 0, EWPF_POS_Y );
		}
		if ( pDescScrollableWnd )
			pDescScrollableWnd->Update();

		if ( pDescStatus )
		{
			pDescStatus->SetStateWithVisual( objective.nButtonIndex );
			pDescStatus->ShowWindow( objective.nID != MISSION_BRIEFING_ID );
		}
	}
	
	nPrevSelectionID = (nSelected >= 0) ? objectives[nSelected].nID : -1;
	
	NInput::PostEvent( "set_modality", 0, 0 );
	NInput::PostEvent( "minimap_show_objectives", nPrevSelectionID, 0 );
	NInput::PostEvent( "set_modality", 1, 0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CInterfaceMissionObjectives::GetButtonIndex( enum EMissionObjectiveState eState )
{
	switch ( eState )
	{
		case EMOS_RECEIVED:
			return 0;
		case EMOS_COMPLETED:
			return 1;
		case EMOS_FAILED:
			return 2;
	};
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceMissionObjectives::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CInterfaceMissionObjectives::IsModal()
{
	return bIsModal;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CInterfaceMissionObjectives::SetStartID( int nID )
{
	if ( nID >= 0 )
	{
		for ( int i = 0; i < objectives.size(); ++i )
		{
			SObjective &objective = objectives[i];
			if ( objective.nID == nID )
			{
				UpdateObjectives( i );
				bCameraBack = true;
				break;
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CInterfaceMissionObjectives::operator&( IBinSaver &saver )
{
	saver.Add( 1, (CInterfaceScreenBase *)this );
	
	saver.Add( 2, &pMainWnd );
	saver.Add( 3, &pDescPanel );

	saver.Add( 11, &pReactions );
	saver.Add( 12, &bIsModal );

	saver.Add( 13, &nPrevSelectionID );
	saver.Add( 14, &pDescScrollableWnd );
	saver.Add( 15, &pDescName );
	saver.Add( 16, &pDescBrief );
	saver.Add( 17, &pDescFull );
	saver.Add( 18, &pDescStatus );
	saver.Add( 19, &objectives );
	saver.Add( 20, &pObjectivesPanel );
	saver.Add( 21, &pNextObjective );
	saver.Add( 22, &pHeader );
	saver.Add( 23, &wszObjectivesSummary );
	saver.Add( 24, &wszMissionName );
	saver.Add( 25, &wszMissionBriefing );
	saver.Add( 26, &fMainInitialTop );
	saver.Add( 27, &fMainInitialHeight );
	saver.Add( 28, &fListInitialHeight );
	saver.Add( 29, &fItemHeight );

	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CICMissionObjectives
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CICMissionObjectives::PreCreate()
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CICMissionObjectives::PostCreate( IInterface *pInterface )
{
	pInterface->SetStartID( nID );
	NMainLoop::PushInterface( pInterface );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CICMissionObjectives::Configure( const char *pszConfig )
{
	if ( pszConfig[0] == 0 )
		nID = -1;
	else
		nID = NStr::ToInt( pszConfig );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x170CEC41, CInterfaceMissionObjectives );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MISSION_OBJECTIVES, CICMissionObjectives );
REGISTER_SAVELOAD_CLASS_NM( 0x170CEC42, CReactions, CInterfaceMissionObjectives );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
