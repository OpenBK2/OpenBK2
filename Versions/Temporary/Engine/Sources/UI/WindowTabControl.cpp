#include "StdAfx.h"
#include ".\windowtabcontrol.h"
#include "WindowMSButton.h"
#include "WindowScrollableContainer.h"
#include "ButtonGroup.h"


// CWindowTabControl


REGISTER_SAVELOAD_CLASS(0x11075B8B, CWindowTabControl);

int CWindowTabControl::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 4, &pHeadersContainer );
	saver.Add( 7, &nActiveTab );
	saver.Add( 8, &pInstance );
	saver.Add( 9, &tabNames );
	saver.Add( 10, &pShared );
	saver.Add( 11, &nTabNumber );
	return 0;
}

void CWindowTabControl::AfterLoad()
{
	CWindow::AfterLoad();
	if ( pHeadersContainer )
		Init();
}

void CWindowTabControl::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowTabControl *pDesc( checked_cast<const NDb::SWindowTabControl*>( _pDesc ) );
	pInstance = pDesc->Duplicate();

	CWindow::InitByDesc( _pDesc );
	pShared = checked_cast_ptr<const NDb::SWindowTabControlShared *>( pDesc->pShared );

	pHeadersContainer = dynamic_cast<CWindowScrollableContainer*>( CUIFactory::MakeWindow( pShared->pHeadersContainer ) );
	if ( pHeadersContainer )
		AddChild( pHeadersContainer, false );

	for ( int i = 0; i < pDesc->tabs.size(); ++i )
	{
		const NDb::SWindowTabControl::STab &tab = pDesc->tabs[i];
		CWindow *pTab = CUIFactory::MakeWindow( tab.pTabContainer );
		AddTab( NStr::ToUnicode( tab.szButtonName ), pTab );
		if ( i > 0 )
			pTab->ShowWindow( false );
	}
	SetActive( 0 );

	//CRAP{ UTILL TEXT IS READY
	/*
	CPtr<IText> pText = MakeObject<IText>( MAIN_TEXT );
	for ( int i = 0; i < pDesc->tabs.size(); ++i )
	{
		const NDb::SWindowTabControl::STab &tab = pDesc->tabs[i];
		CWindow *pTab = CUIFactory::MakeWindow( tab.pTabContainer );
		pText->SetKey( tab.szButtonName );
		AddTab( pText->GetText(), pTab );
	}
	*/
	//CRAP}
}

int CWindowTabControl::GetNTabs() const
{
	return nTabNumber;
}

void CWindowTabControl::SetActive( const int nTab, const bool bUpdateButtons )
{
	if ( nTab != nActiveTab )
	{
		// hide current nActiveTab window and show new tab window
		if ( -1 != nActiveTab )
			ShowTab( nActiveTab, false, false );
		nActiveTab = nTab;
		ShowTab( nActiveTab, true, bUpdateButtons );
	}
}

void CWindowTabControl::ShowTab( const int nTab, const bool bShow, const bool bUpdateButtons )
{
	CWindow * pWindow = dynamic_cast<CWindow*>( GetChild( CreateTabName( nTab, 0 ), false ) );
	if ( pWindow )
		pWindow->ShowWindow( bShow );

	if ( /*bUpdateButtons && bShow && pButtonSample &&*/ pHeadersContainer )
	{
		CWindowMSButton *pButton = dynamic_cast<CWindowMSButton*>( pHeadersContainer->GetItem( nTab ) );
		if ( pButton )
		{
			CButtonGroup * pGroup = pButton->GetButtonGroup();
			NI_ASSERT( pGroup != 0, "the button must have button group in tab control header" );
			//pButton->SetState( bShow ? 1 : 0 );
			pGroup->SetActive( pButton );
			pHeadersContainer->EnsureElementVisible( pButton );
		}
	}
}

void CWindowTabControl::Reposition( const CTRect<float> &rcParent )
{
	CWindow::Reposition( rcParent );
/*
	if ( !pContainerSample )
		pContainerSample = checked_cast<CWindow*>( GetChild( "ContainerSample" ) );
	if ( !pButtonSample )
		pButtonSample = dynamic_cast<CWindowMSButton*>( GetChild( "ButtonSample" ) );
*/
	if ( !pHeadersContainer )
		pHeadersContainer = dynamic_cast<CWindowScrollableContainer*>( GetChild( "HeadersContainer", false ) );
}

void CWindowTabControl::Init()
{
	// find existing tabs
	nTabNumber = 0;
	while( true )
	{
		IWindow *pContainer = GetChild( CreateTabName( nTabNumber, 0 ), false );
		if ( pContainer )
		{
			if ( pHeadersContainer /* && pButtonSample */ )
			{
				IWindow *pButton = pHeadersContainer->GetItem( nTabNumber );
				if ( pButton )
					dynamic_cast<IButton*>( pButton )->SetNotifySink( this );
			}
			++nTabNumber;
		}
		else
			break;
	}
}

const string &CWindowTabControl::CreateTabName( const int nTab, CWindow *pTab )
{
	tabNames.resize( Max( tabNames.size(), nTab + 1 ) );
	if ( tabNames[nTab].empty() || pTab )
	{
		if ( pTab )
		{
			NI_ASSERT( GetChild( pTab->GetName(), true ) == 0, StrFmt( "Tab \"%s\" already exists", pTab->GetName().c_str() ) );
			tabNames[nTab] = pTab->GetName();
		}
		else
		{
			tabNames[nTab] = StrFmt( "%i", nTab );
		}
	}
	return tabNames[nTab];
}

void CWindowTabControl::AddTab( const wstring &szButtonName, CWindow *pTab )
{
	// if we have buttons, add button
	

	if ( pHeadersContainer )
	{
		CWindowMSButton *pButton = dynamic_cast<CWindowMSButton*>( CUIFactory::MakeWindow( pShared->pButtonSample ));
		if ( pButton )
		{
			pHeadersContainer->PushBack( pButton, false );
			pButton->Init();
			pButton->SetTextString( szButtonName );			
			pButton->ShowWindow( true );
			pButton->SetPlacement( 0, 0, pButton->GetOptimalWidth(), 0, EWPF_SIZE_X );
			pButton->SetNotifySink( this );
			pButton->SetButtonGroup( 1 );
		}
	}

	// add container
	// NI_ASSERT( pContainerSample != 0, "no container sample" );
	
	pTab->SetName( CreateTabName( nTabNumber, pTab ) );
	AddChild( pTab, false );
	++nTabNumber;
}

void CWindowTabControl::AddTab( const wstring &szButtonName )
{
	// add container
	CWindow * pNewTab = checked_cast<CWindow*>( CUIFactory::MakeWindow( pShared->pContainerSample ) );
	NI_ASSERT( pNewTab != 0, "cannot create new tab" );
	AddTab( szButtonName, pNewTab );
	pNewTab->Init();
}

void CWindowTabControl::AddElement( const int nTab, IWindow *pWnd )
{
	CWindow * pWindow = dynamic_cast<CWindow*>( GetChild( CreateTabName( nTab, 0 ), false ) );
	if ( pWindow )
		pWindow->AddChild( pWnd, true );
}

void CWindowTabControl::StateChanged( class CWindow *pWho )
{
	// switch tab
	const int nTabNumber = pHeadersContainer->GetItemNumber( pWho );
	SetActive( nTabNumber );
	/*for ( int i = 0; i < nTabNumber; ++i )
	{
	if ( pWho->GetName() ==  )
	{
	SetActive( i, false );
	return;
	}
	}*/
}


