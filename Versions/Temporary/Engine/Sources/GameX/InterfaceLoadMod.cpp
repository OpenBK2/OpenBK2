#include "stdafx.h"
#include "InterfaceLoadMod.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "DBGameRoot.h"
#include "ScenarioTracker.h"
#include "Misc/StrProc.h"
#include "System/Text.h"
#include "Main/MODs.h"

#include "GameX_export.h"

const int NO_MOD = -1;

// CInterfaceLoadMod

CInterfaceLoadMod::CInterfaceLoadMod() : 
	CInterfaceScreenBase( "LoadMod", "load_mod" ),
	nInitialMod( NO_MOD )
{
}

bool CInterfaceLoadMod::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );
	
	MakeInterior();

	return true;
}

void CInterfaceLoadMod::MakeInterior()
{
	pMainWnd = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	pTopPanel = GetChildChecked<IWindow>( pMainWnd, "TopPanel", true );
	pBottomPanel = GetChildChecked<IWindow>( pMainWnd, "BottomPanel", true );
	pModsListPanel = GetChildChecked<IWindow>( pMainWnd, "LeftPanel", true );
	pModDescPanel = GetChildChecked<IWindow>( pMainWnd, "RightPanel", true );

	pModsListCont = GetChildChecked<IScrollableContainer>( pModsListPanel, "ModsListCont", true );
	pModsListItemTemplate = GetChildChecked<IWindow>( pModsListPanel, "ModsListItem", true );
	
	if ( pModsListItemTemplate )
		pModsListItemTemplate->ShowWindow( false );

	pModDescCont = GetChildChecked<IScrollableContainer>( pModDescPanel, "ModDescCont", true );
	pModDescView = GetChildChecked<ITextView>( pModDescPanel, "ModDescView", true );
	pModNameView = GetChildChecked<ITextView>( pModDescPanel, "ModDescNameView", true );
	
	if ( pModDescCont )
	{
		if ( pModNameView )
			pModDescCont->PushBack( pModNameView, false );
		if ( pModDescView )
			pModDescCont->PushBack( pModDescView, false );
	}
	
	pDefaultBtn = GetChildChecked<IButton>( pBottomPanel, "DefaultBtn", true );
	pAcceptBtn = GetChildChecked<IButton>( pBottomPanel, "AcceptBtn", true );

	std::vector<NMOD::SMOD> gameMODs;
	NMOD::GetAllMODs( &gameMODs );
	for ( int i = 0; i < gameMODs.size(); ++i )
	{
		NMOD::SMOD &gameMOD = gameMODs[i];
		AddMod( gameMOD.wszName, gameMOD.wszDesc, gameMOD.szFullFolderPath );
	}
	
	if ( IScreen *pScreen = GetScreen() )
	{
		wszModDefaultName = pScreen->GetTextEntry( "T_MOD_DEFAULT_NAME" );
		wszModDefaultDesc = pScreen->GetTextEntry( "T_MOD_DEFAULT_DESC" );
	}

	NMOD::SMOD modDesc;
	NMOD::GetAttachedMOD( &modDesc );

	IWindow *pWnd = 0;
	if ( !modDesc.szFullFolderPath.empty() )
	{
		for ( int i = 0; i < mods.size(); ++i )
		{
			SMod &mod = mods[i];
			if ( mod.szFullFolderPath == modDesc.szFullFolderPath )
			{
				nInitialMod = i;
				pWnd = mod.pWnd;
				break;
			}
		}
	}
	
	if ( pModsListCont )
		pModsListCont->Select( pWnd );

	UpdateSelection();
}

void CInterfaceLoadMod::AddMod( const std::wstring &wszName, const std::wstring &wszDesc, const NFile::CFilePath &szFullFolderPath )
{
	SMod mod;
	mod.pWnd = AddWindowCopy( pModsListCont, pModsListItemTemplate );
	mod.pNameView = GetChildChecked<ITextView>( mod.pWnd, "NameView", true );
	mod.pCheckedWnd = GetChildChecked<IWindow>( mod.pWnd, "Checked", true );
	mod.wszName = wszName;
	mod.wszDesc = wszDesc;
	mod.szFullFolderPath = szFullFolderPath;
	
	if ( mod.pWnd )
		mod.pWnd->ShowWindow( true );
	if ( mod.pNameView )
		mod.pNameView->SetText( mod.pNameView->GetDBText() + mod.wszName );
	if ( mod.pCheckedWnd )
		mod.pCheckedWnd->ShowWindow( false );
	if ( pModsListCont )
		pModsListCont->PushBack( mod.pWnd, true );

	mods.push_back( mod );
}

const CInterfaceLoadMod::SMod* CInterfaceLoadMod::FindSelected() const
{
	IWindow *pSelectedWnd = 0;
	if ( pModsListCont )
		pSelectedWnd = pModsListCont->GetSelectedItem();
	if ( !pSelectedWnd )
		return 0;
		
	for ( int i = 0; i < mods.size(); ++i )
	{
		const SMod &mod = mods[i];
		if ( mod.pWnd == pSelectedWnd )
			return &mod;
	}
	
	return 0;
}

bool CInterfaceLoadMod::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "menu_back" )
		return OnBack();
	if ( szReaction == "menu_default" )
		return OnDefault();
	if ( szReaction == "menu_accept" )
		return OnAccept();
	if ( szReaction == "menu_select" )
		return OnSelect();
	if ( szReaction == "menu_dbl_click" )
		return OnDblClick();

	return false;
}

int CInterfaceLoadMod::Check( const std::string &szCheckName ) const
{
	return 0;
}

bool CInterfaceLoadMod::OnBack()
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );

	return true;
}

bool CInterfaceLoadMod::OnDefault()
{
	if ( pModsListCont )
		pModsListCont->Select( 0 );

	UpdateSelection();

	return true;
}

bool CInterfaceLoadMod::OnAccept()
{
	const SMod *pMod = FindSelected();
	SelectMod( pMod );

//	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
//	NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );

	return true;
}

bool CInterfaceLoadMod::OnSelect()
{
	UpdateSelection();
	
	return true;
}

bool CInterfaceLoadMod::OnDblClick()
{
	// do nothing

	return true;
}

void CInterfaceLoadMod::SelectMod( const SMod *pMod )
{
	if ( pMod )
	{
		if ( NMOD::DoesMODAttached(pMod->szFullFolderPath) == false )
		{
			NMOD::AttachMOD( pMod->szFullFolderPath );
			NGlobal::ProcessCommand( L"menu" );
		}
	}
	else
	{
		NMOD::DetachAllMODs();
		NGlobal::ProcessCommand( L"menu" );
	}
}

void CInterfaceLoadMod::UpdateSelection()
{
	if ( pPrevCheckedWnd )
	{
		pPrevCheckedWnd->ShowWindow( false );
		pPrevCheckedWnd = 0;
	}

	const SMod *pMod = FindSelected();
		
	if ( !pMod || !pMod->pWnd )
	{
		if ( pModNameView )
			pModNameView->SetText( pModNameView->GetDBText() + wszModDefaultName );
		if ( pModDescView )
			pModDescView->SetText( pModDescView->GetDBText() + wszModDefaultDesc );
		if ( pModDescCont )
			pModDescCont->Update();
			
		if ( pDefaultBtn )
			pDefaultBtn->Enable( false );
		if ( pAcceptBtn )
			pAcceptBtn->Enable( IsChanged() );

		return;
	}
	
	if ( pModNameView )
		pModNameView->SetText( pModNameView->GetDBText() + pMod->wszName );
	if ( pModDescView )
		pModDescView->SetText( pModDescView->GetDBText() + pMod->wszDesc );

	if ( pModDescCont )
		pModDescCont->Update();

	if ( pDefaultBtn )
		pDefaultBtn->Enable( true );
	if ( pAcceptBtn )
		pAcceptBtn->Enable( IsChanged() );
		
	pPrevCheckedWnd = pMod->pCheckedWnd;
	if ( pPrevCheckedWnd )
		pPrevCheckedWnd->ShowWindow( true );
}

bool CInterfaceLoadMod::IsChanged() const
{
	const SMod *pMod = FindSelected();
	if ( !pMod )
		return nInitialMod != NO_MOD;

	if ( nInitialMod < 0 || nInitialMod >= mods.size() )
		return true;

	return pMod != &mods[nInitialMod];
}

bool CInterfaceLoadMod::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive );
}

// CICLoadMod

void CICLoadMod::PreCreate()
{
}

void CICLoadMod::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICLoadMod::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x17263C01, CInterfaceLoadMod )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_LOAD_MOD, CICLoadMod )


