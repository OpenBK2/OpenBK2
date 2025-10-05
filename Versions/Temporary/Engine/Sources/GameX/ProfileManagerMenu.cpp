#include "stdafx.h"
#include "ProfileManagerMenu.h"
#include "InterfaceMisc.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "Main/Profiles.h"
#include "Misc/StrProc.h"

// CInterfaceProfileManager

CInterfaceProfileManager::CInterfaceProfileManager() :
	CInterfaceScreenBase( "ProfileManager", "profile_manager_menu" ),
	bNoUpdateSelection( false )
{
	AddObserver( "message_box_ok", &CInterfaceProfileManager::MsgOk );
	AddObserver( "message_box_cancel", &CInterfaceProfileManager::MsgCancel );
}

bool CInterfaceProfileManager::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	AddScreen( this );

	//Get elements
	pMain = GetChildChecked<IWindow>( pScreen, "Main", false );
	
	pCenterPanel = GetChildChecked<IWindow>( pMain, "CenterPanel", false );

	pProfileListTemplate = GetChildChecked<IScrollableContainer>( pCenterPanel, "ProfileList", true );
	if ( pProfileListTemplate )
		pProfileListTemplate->ShowWindow( false );
	pItemTemplate = GetChildChecked<IWindow>( pProfileListTemplate, "ListItem", true );
	pNameEditLine = GetChildChecked<IEditLine>( pCenterPanel, "ProfileName", true );

	pCreateBtn = GetChildChecked<IButton>( pMain, "ButtonCreate", true );
	pSelectBtn = GetChildChecked<IButton>( pMain, "ButtonSelect", true );
	pDeleteBtn = GetChildChecked<IButton>( pMain, "ButtonDelete", true );

	//Fill profile list
	pProfileList = 0;
	FillScreenList();
	
	eState = EST_DEFAULT;
	
	if ( pMain )
		SetMainWindowTexture( pMain, InterfaceState()->GetMenuBackgroundTexture() );

	return true;
}

bool CInterfaceProfileManager::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "reaction_on_back" )
	{
		MsgBack( szSender );
		return true;
	}
	if ( szReaction == "reaction_on_select" )
	{
		MsgSelect( szSender );
		return true;
	}
	if ( szReaction == "reaction_on_create" )
	{
		MsgCreate( szSender );
		return true;
	}
	if ( szReaction == "reaction_on_delete" )
	{
		MsgDelete( szSender );
		return true;
	}
	if ( szReaction == "react_on_selection_change" )
	{
		MsgSelectionChange( szSender );
		return true;
	}
	if ( szReaction == "edit_changed" )
		return OnEditChanged();

	return false;
}

int CInterfaceProfileManager::Check( const std::string &szCheckName ) const
{
	return 0;
}

void CInterfaceProfileManager::FillScreenList()
{
	std::vector<std::wstring> tmpList;

	if ( pProfileList ) 
	{
		if ( pCenterPanel ) 
			pCenterPanel->RemoveChild( pProfileList );
		pProfileList = 0;
	}
	pProfileList = dynamic_cast<IScrollableContainer*>( AddWindowCopy( pCenterPanel, pProfileListTemplate ) );
	if ( pProfileList )
		pProfileList->ShowWindow( true );
	profiles.clear();

	NProfile::GetAllProfiles( &tmpList );
	std::wstring wszSelectedItem = NProfile::GetCurrentProfileName();
	nSelectedItem = -1;
	for ( int i = 0; i < tmpList.size(); ++i )
	{
		SProfileEntry item;
		item.wszName = tmpList[i];
		//Create screen item
		IWindow *pElement = AddWindowCopy( pProfileList, pItemTemplate );
		NI_ASSERT( pElement, "Could not create Profile list entry" );
		if ( pElement )
			pElement->ShowWindow( true );
		pElement->SetName( StrFmt("Profile%d", i ) );
		ITextView *pName = GetChildChecked<ITextView>( pElement, "NameText", true );
		if ( pName )
			pName->SetText( pName->GetDBText() + item.wszName );

		IWindow *pIndic = GetChildChecked<IWindow>( pElement, "Indicator", true );		//Get "currently selected" indicator
		if ( item.wszName == wszSelectedItem ) //Select one
		{			
			nSelectedItem = i;
			if ( pIndic )
				pIndic->ShowWindow( true );
		} 
		else
		{
			if ( pIndic )
				pIndic->ShowWindow( false );
		}

		item.pWindow = pElement;
		pProfileList->PushBack( pElement, true );

		profiles.push_back( item );
	}

	if ( nSelectedItem != -1 ) 
	{
		if ( pProfileList )
			pProfileList->Select( profiles[ nSelectedItem ].pWindow.GetPtr() );
	}
}

void CInterfaceProfileManager::MsgBack( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
}

void CInterfaceProfileManager::MsgSelect( const std::string &szSender )
{
	if ( nSelectedItem == -1 )		// Not selected, do nothing
		return;

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );

	std::wstring wszCurrentProfile = NProfile::GetCurrentProfileName();
	if ( profiles[nSelectedItem].wszName != wszCurrentProfile )
		NProfile::ChangeProfile( profiles[nSelectedItem].wszName );
}

void CInterfaceProfileManager::MsgCreate( const std::string &szSender )
{
	std::wstring wszName;
	if ( pNameEditLine )
		wszName = pNameEditLine->GetText();
		
	if ( FindProfileIndex() >= 0 ) // sanity check
	{
		NI_ASSERT( 0, "Programmers: profile already exists" );
		return;
	}

	if ( NProfile::AddProfile( wszName ) )
		FillScreenList();
		
	NProfile::ChangeProfile( wszName );

	nSelectedItem = FindProfileIndex();
	IWindow *pWnd = (nSelectedItem >= 0) ? profiles[nSelectedItem].pWindow : 0;
	if ( pProfileList )
		pProfileList->Select( pWnd );
	
	UpdateButtons();

	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
}

void CInterfaceProfileManager::MsgDelete( const std::string &szSender )
{
	std::wstring wszName;

	if ( nSelectedItem < 0 || nSelectedItem >= profiles.size() ) 
		return;			//Wrong selection

	wszName = profiles[ nSelectedItem ].wszName;

	if ( NProfile::GetCurrentProfileName() == wszName )						// Cannot delete currently selected profile
	{
		// Display warning
		NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
			CICMessageBox::MakeConfigString( "MessageBoxWindowOk",
			InterfaceState()->GetTextEntry( "T_PROFILE_MANAGER_DELETE_CURRENT" ) ).c_str() );
		return;
	}

	eState = EST_DELETE_QUESTION;
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOkCancel",
		InterfaceState()->GetTextEntry( "T_PROFILE_MANAGER_DELETE_QUESTION" ) ).c_str() );
}

void CInterfaceProfileManager::MsgSelectionChange( const std::string &szSender )
{
	if ( bNoUpdateSelection )
		return;

	IWindow *pSelection = pProfileList ? pProfileList->GetSelectedItem() : 0;

	nSelectedItem = -1;
	if ( !pSelection )
	{
		if ( pNameEditLine )
			pNameEditLine->SetText( L"" );			//No selection, empty editline
	}
	else
	{
		for ( int i = 0; i < profiles.size(); ++i )
		{
			if ( pSelection == profiles[i].pWindow ) 
			{
				nSelectedItem = i;
				if ( pNameEditLine )
					pNameEditLine->SetText( profiles[i].wszName.c_str() );
			}
		}
	}

	UpdateButtons();
}

bool CInterfaceProfileManager::OnEditChanged()
{
	bNoUpdateSelection = true;

	nSelectedItem = FindProfileIndex();
	IWindow *pWnd = (nSelectedItem >= 0) ? profiles[nSelectedItem].pWindow : 0;
	if ( pProfileList )
		pProfileList->Select( pWnd );

	UpdateButtons();

	bNoUpdateSelection = false;

	return true;
}

int CInterfaceProfileManager::FindProfileIndex() const
{
	std::wstring wszEditName;
	if ( pNameEditLine )
		wszEditName = pNameEditLine->GetText();

	for ( int i = 0; i < profiles.size(); ++i )
	{
		const SProfileEntry &profile = profiles[i];
		if ( IsSameProfileName( profile.wszName, wszEditName ) ) // Profile exists
			return i;
	}

	return -1;
}

void CInterfaceProfileManager::UpdateButtons()
{
	std::wstring wszEditName;
	if ( pNameEditLine )
		wszEditName = pNameEditLine->GetText();

	int nIndex = FindProfileIndex();
	std::wstring wszCurrentProfile = NProfile::GetCurrentProfileName();
	
	// Can create only a profile with non-empty, non-existent name
	bool bCanCreate = !wszEditName.empty() && nIndex == -1;
	bool bCanSelect = nIndex >= 0;
	// can't delete currently selected profile
	bool bCanDelete = bCanSelect;

	if ( pCreateBtn )
		pCreateBtn->Enable( bCanCreate );
	if ( pSelectBtn )
		pSelectBtn->Enable( bCanSelect );
	if ( pDeleteBtn )
		pDeleteBtn->Enable( bCanDelete );
}

bool CInterfaceProfileManager::IsSameProfileName( const std::wstring &wszName1, const std::wstring &wszName2 ) const
{
	std::string szName1 = NStr::ToMBCS( wszName1 );
	std::string szName2 = NStr::ToMBCS( wszName2 );
	NStr::ToLower( &szName1 );
	NStr::ToLower( &szName2 );
	return szName1 == szName2;
}

void CInterfaceProfileManager::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
	
	PauseIntermission( !bFocus );
}

bool CInterfaceProfileManager::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

bool CInterfaceProfileManager::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );
	
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return bResult;
}

void CInterfaceProfileManager::MsgOk( const SGameMessage &msg )
{
	switch ( eState )
	{
		case EST_DELETE_QUESTION:
		{
			std::wstring wszName = profiles[ nSelectedItem ].wszName;
			if ( NProfile::RemoveProfile( wszName ) ) 
			{
				FillScreenList();
			}
			break;
		}
			
		default:
			break;
	}
	eState = EST_DEFAULT;
}

void CInterfaceProfileManager::MsgCancel( const SGameMessage &msg )
{
	eState = EST_DEFAULT;
}

// CICProfileManager

void CICProfileManager::PreCreate()
{
}

void CICProfileManager::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICProfileManager::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x19113481, CInterfaceProfileManager )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_PROFILE_MENU, CICProfileManager )


