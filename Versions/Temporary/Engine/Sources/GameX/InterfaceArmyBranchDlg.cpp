#include "stdafx.h"
#include "InterfaceArmyBranchDlg.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "DBGameRoot.h"
#include "ScenarioTracker.h"
#include "Misc/StrProc.h"
#include "System/Text.h"

#include "GameX_export.h"

// CInterfaceArmyBranchDlg

CInterfaceArmyBranchDlg::CInterfaceArmyBranchDlg() : 
	CInterfaceScreenBase( "ArmyBranchDlg", "army_branch_dlg" )
{
}

bool CInterfaceArmyBranchDlg::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );
	
	MakeInterior();

	return true;
}

void CInterfaceArmyBranchDlg::SetParams( const std::vector<CInterfaceArmyBranchDlg::SBranch> &_branches )
{
	branches = _branches;
	
	nSelectedBranch = branches.empty() ? -1 : 0;
	nLeftBranch = 0;
	
	UpdateInterior();
}

void CInterfaceArmyBranchDlg::MakeInterior()
{
	pMainWnd = GetChildChecked<IWindow>( GetScreen(), "ArmyBranchPanel", true );
	
	pLeftBtn = GetChildChecked<IButton>( pMainWnd, "LeftBtn", true );
	pRightBtn = GetChildChecked<IButton>( pMainWnd, "RightBtn", true );

	pBranchNameView = GetChildChecked<ITextView>( pMainWnd, "BranchNameView", true );
	pDescCont = GetChildChecked<IScrollableContainer>( pMainWnd, "DescCont", true );
	pDescView = GetChildChecked<ITextView>( pMainWnd, "DescView", true );
	
	if ( pDescView && pDescCont )
			pDescCont->PushBack( pDescView, false );
			
	IWindow *pBranchesBlock = GetChildChecked<IWindow>( pMainWnd, "BranchesBlock", true );
	if ( pBranchesBlock )
	{
		for ( int i = 0; ; ++i )
		{
			IWindow *pWnd = pBranchesBlock->GetChild( StrFmt( "Branch%02d", i + 1 ), true );
			if ( !pWnd )
				break;
				
			SVisSlot visSlot;
			visSlot.pWnd = pWnd;
			visSlot.pBtn = GetChildChecked<IButton>( pWnd, "BranchBtn", true );
			visSlot.szBtnName = StrFmt( "BranchBtn%02d", i + 1 );
			if ( visSlot.pBtn )
				visSlot.pBtn->SetName( visSlot.szBtnName );
			visSlot.pIconWnd = GetChildChecked<IWindow>( pWnd, "Icon", true );
			visSlot.pSelectionWnd = GetChildChecked<IWindow>( pWnd, "Selection", true );
			
			if ( visSlot.pBtn )
				visSlot.pBtn->ShowWindow( false );
			if ( visSlot.pSelectionWnd )
				visSlot.pSelectionWnd->ShowWindow( false );
			
			visSlots.push_back( visSlot );
		}
	}
	
/*	for ( int i = 0; i < 12; ++i )
	{
		SBranch branch;
		branch.wszName = NStr::ToUnicode( StrFmt( "Name%d", i ) );
		branch.wszDesc = NStr::ToUnicode( StrFmt( "Desc%d", i ) );

		branches.push_back( branch );
	}
	nSelectedBranch = branches.empty() ? -1 : 0;*/

	nSelectedBranch = -1;
	nLeftBranch = 0;

	UpdateInterior();
}

void CInterfaceArmyBranchDlg::UpdateInterior()
{
	if ( pLeftBtn )
	{
		pLeftBtn->ShowWindow( branches.size() > visSlots.size() );
		pLeftBtn->Enable( nLeftBranch > 0 );
	}
	if ( pRightBtn )
	{
		pRightBtn->ShowWindow( branches.size() > visSlots.size() );
		pRightBtn->Enable( nLeftBranch + visSlots.size() < branches.size() );
	}
		
	for ( int i = 0; i < visSlots.size(); ++i )
	{
		SVisSlot &visSlot = visSlots[i];
		if ( nLeftBranch + i >= branches.size() )
		{
			if ( visSlot.pBtn )
				visSlot.pBtn->ShowWindow( false );
		}
		else
		{
			SBranch &branch = branches[nLeftBranch + i];
			
			if ( visSlot.pBtn )
				visSlot.pBtn->ShowWindow( true );
			if ( visSlot.pIconWnd )
				visSlot.pIconWnd->SetTexture( branch.pIconTexture );
			if ( visSlot.pSelectionWnd )
				visSlot.pSelectionWnd->ShowWindow( (nLeftBranch + i == nSelectedBranch) );
		}
	}

	if ( nSelectedBranch >= 0 && nSelectedBranch < branches.size() )
	{
		SBranch &branch = branches[nSelectedBranch];
		
		if ( pBranchNameView )
			pBranchNameView->SetText( pBranchNameView->GetDBText() + branch.wszName );

		if ( pDescView )
			pDescView->SetText( pDescView->GetDBText() + branch.wszDesc );
		if ( pDescCont )
			pDescCont->Update();
	}
}

bool CInterfaceArmyBranchDlg::OnClose()
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );

	return true;
}

bool CInterfaceArmyBranchDlg::OnLeft()
{
	if ( nLeftBranch > 0 )
	{
		--nLeftBranch;
		if ( nSelectedBranch >= nLeftBranch + visSlots.size() )
			nSelectedBranch = nLeftBranch + visSlots.size() - 1;

		UpdateInterior();
	}
	
	return true;
}

bool CInterfaceArmyBranchDlg::OnRight()
{
	if ( nLeftBranch + visSlots.size() < branches.size() )
	{
		++nLeftBranch;
		if ( nSelectedBranch < nLeftBranch )
			nSelectedBranch = nLeftBranch;

		UpdateInterior();
	}

	return true;
}

bool CInterfaceArmyBranchDlg::OnSelect( const std::string &szSender )
{
	for ( int i = 0; i < visSlots.size(); ++i )
	{
		SVisSlot &visSlot = visSlots[i];
		if ( visSlot.szBtnName == szSender )
		{
			nSelectedBranch = nLeftBranch + i;
			UpdateInterior();
			break;
		}
	}

	return true;
}

bool CInterfaceArmyBranchDlg::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "dialog_army_branch_close" )
		return OnClose();
	if ( szReaction == "dialog_army_branch_left" )
		return OnLeft();
	if ( szReaction == "dialog_army_branch_right" )
		return OnRight();
	if ( szReaction == "dialog_army_branch_select" )
		return OnSelect( szSender );

	return false;
}

int CInterfaceArmyBranchDlg::Check( const std::string &szCheckName ) const
{
	return 0;
}

// CICArmyBranchDlg

CICArmyBranchDlg::CICArmyBranchDlg( const std::vector<CInterfaceArmyBranchDlg::SBranch> &_branches )
{
	branches = _branches;
}

void CICArmyBranchDlg::PreCreate()
{
}

void CICArmyBranchDlg::PostCreate( IInterface *pInterface )
{
	pInterface->SetParams( branches );
	NMainLoop::PushInterface( pInterface );
}

void CICArmyBranchDlg::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x17267B81, CInterfaceArmyBranchDlg )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_ARMY_BRANCH_DLG, CICArmyBranchDlg )


