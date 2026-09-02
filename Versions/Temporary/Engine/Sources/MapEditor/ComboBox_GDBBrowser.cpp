#include "stdafx.h"
#include "WMDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "PC_Dialog.h"

#include "ComboBox_GDBBrowser.h"

#include <cstdint>

CComboBoxGDBBrowser::CComboBoxGDBBrowser( int _nGDBBrowserID ) : pwndParent( 0 ), nControlID( 0 ), bEnableEdit( true ), nGDBBrowserID( _nGDBBrowserID )
{
	ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
}


CComboBoxGDBBrowser::~CComboBoxGDBBrowser()
{
	for ( int nTabIndex = 0; nTabIndex < tabList.size(); ++nTabIndex )
	{
		if ( tabList[nTabIndex] )
		{
			delete ( tabList[nTabIndex] );
			tabList[nTabIndex] = 0;
		}
	}
}


void CComboBoxGDBBrowser::MoveWindow( const CRect &rRect )
{
	if ( IsWindow( wndComboBox.m_hWnd ) )
	{
		CRect comboBoxRect; 
		//wndComboBox.GetWindowRect( &comboBoxRect ); 	
		wndComboBox.MoveWindow( rRect.left, rRect.top, rRect.Width(), rRect.Height() ); 	
		wndComboBox.GetWindowRect( &comboBoxRect ); 	
		//wndComboBox.MoveWindow( rRect.left, rRect.top, rRect.Width(), comboBoxRect.Height() );
		for ( int nTabIndex = 0; nTabIndex < tabList.size(); ++nTabIndex )
		{
			if ( ( tabList[nTabIndex] != 0 ) && ::IsWindow( tabList[nTabIndex]->m_hWnd ) )
			{
				tabList[nTabIndex]->MoveWindow( rRect.left, rRect.top + comboBoxRect.Height(), rRect.Width(), rRect.Height() - comboBoxRect.Height() );
			}
		}
	}
}


bool CComboBoxGDBBrowser::Create( CWnd *_pwndParent, unsigned _nControlID )
{
	pwndParent = _pwndParent;
	nControlID = _nControlID;
	//
	const uint32_t dwStyle =  WS_CHILD | WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DISABLENOSCROLL | CBS_DROPDOWNLIST | CBS_SORT;
	const bool bResult = wndComboBox.Create( dwStyle, CRect( 0, 0, 0, 0 ), pwndParent, nControlID );
	wndComboBox.SetExtendedUI( true );
	wndComboBox.SetFont( CFont::FromHandle( (HFONT)GetStockObject( DEFAULT_GUI_FONT ) ) );
	InitImageLists();
	return bResult;
}


void CComboBoxGDBBrowser::SwitchTabs()
{
	for ( int nTabIndex = 0; nTabIndex < tabList.size(); ++nTabIndex )
	{
		if ( ( tabList[nTabIndex] != 0 ) && ::IsWindow( tabList[nTabIndex]->m_hWnd ) )
		{
			tabList[nTabIndex]->ShowWindow( SW_HIDE );
		}
	}
	CTreeGDBBrowserBase *pwndActiveTab = 0;
	if ( GetActiveTab( &pwndActiveTab ) )
	{
		pwndActiveTab->ShowWindow( SW_SHOW );
	}
}


bool CComboBoxGDBBrowser::ActivateTab( CTreeGDBBrowserBase* pwndActiveTab )
{
	if ( pwndActiveTab != 0 )
	{
		for ( int nTabIndex = 0; nTabIndex < tabList.size(); ++nTabIndex )
		{
			if ( tabList[nTabIndex] == pwndActiveTab )
			{
				const int nStringCount = wndComboBox.GetCount();
				for ( int nStringIndex = 0; nStringIndex < nStringCount; ++nStringIndex )
				{
					if ( wndComboBox.GetItemData( nStringIndex ) == nTabIndex )
					{
						wndComboBox.SetCurSel( nStringIndex );
						pwndParent->SendMessage( WM_GDB_BROWSER, MAKEWPARAM( GDB_BROWSER_CBN_SELCHANGE, 0 ), reinterpret_cast<LPARAM>( this ) );
						return true;
					}
				}
				break;
			}
		}
	}
	return false;
}


CTreeGDBBrowserBase* CComboBoxGDBBrowser::GetTab( int nTabIndex )
{
	if ( ( nTabIndex >=0 ) && ( nTabIndex < tabList.size() ) )
	{
		return tabList[nTabIndex];
	}
	return 0;
}


CTreeGDBBrowserBase* CComboBoxGDBBrowser::GetTab( const std::string &rszObjectTypeName )
{
	for ( int nItemIndex = 0; nItemIndex < wndComboBox.GetCount(); ++nItemIndex )
	{
		CString strName;
		wndComboBox.GetLBText( nItemIndex, strName );
		std::string szName = strName;
		if ( szName == rszObjectTypeName )
		{
			const int nTabIndex = wndComboBox.GetItemData( nItemIndex );
			return GetTab( nTabIndex );
		}
	}
	return 0;
}


bool CComboBoxGDBBrowser::GetActiveTab( CTreeGDBBrowserBase** ppwndActiveTab )
{
	const int nSelectedString = wndComboBox.GetCurSel();
	if ( nSelectedString != CB_ERR )
	{
		int nSelectedTabIndex = wndComboBox.GetItemData( nSelectedString );
		if ( ( nSelectedTabIndex >= 0 ) && ( nSelectedTabIndex < tabList.size() ) )
		{
			if ( ( tabList[nSelectedTabIndex] != 0 ) && ::IsWindow( tabList[nSelectedTabIndex]->m_hWnd ) )
			{
				if ( ppwndActiveTab != 0 )
				{
					( *ppwndActiveTab ) = tabList[nSelectedTabIndex];
				}
				return true;
			}
		}
	}
	return false;
}


bool CComboBoxGDBBrowser::GetActiveTabName( std::string *pszName )
{
	const int nSelectedString = wndComboBox.GetCurSel();
	if ( nSelectedString != CB_ERR )
	{
		CString strLBText;
		wndComboBox.GetLBText( nSelectedString, strLBText );
		if ( pszName )
		{
			( *pszName ) = strLBText;
		}
		return true;
	}
	return false;
}


bool CComboBoxGDBBrowser::ShowWindow( int nCmdShow )
{
	wndComboBox.ShowWindow( nCmdShow );
	CTreeGDBBrowserBase *pwndActiveTab = 0;
	if ( GetActiveTab( &pwndActiveTab ) )
	{
		pwndActiveTab->ShowWindow( nCmdShow );
		if ( nCmdShow == SW_SHOW )
		{
			pwndActiveTab->RedrawWindow();
		}
	}
	return true;
}


bool CComboBoxGDBBrowser::IsFocused()
{
	HWND hWnd = ::GetFocus();
	if ( wndComboBox.m_hWnd == hWnd  )
	{
		return true;
	}
	CTreeGDBBrowserBase* pwndActiveTab = 0;
	GetActiveTab( &pwndActiveTab );
	if ( pwndActiveTab && ( pwndActiveTab->m_hWnd == hWnd ) )
	{
		return true;
	}
	return false;
}


void CComboBoxGDBBrowser::InitImageLists()
{
	{
		CBitmap bmp;
		typesImageList.Create( 16, 16, ILC_MASK, CTreeGDBBrowserBase::GDBO_COUNT * 2, CTreeGDBBrowserBase::GDBO_COUNT * 2 );
		NI_ASSERT( typesImageList.m_hImageList != 0, "C3DTabGDBBrowser::InitImageLists(), can't create typesImageList" );

		bmp.LoadBitmap( IDB_TABGDBB_TREE_TYPES_IMAGE_LIST );
		typesImageList.Add( &bmp, RGB( 255, 0, 255 ) );
		bmp.DeleteObject();
	}
	{
		CBitmap bmp;
		headerImageList.Create( 16, 16, ILC_MASK, TABGDBB_TREE_COLUMN_COUNT, TABGDBB_TREE_COLUMN_COUNT );
		NI_ASSERT( headerImageList.m_hImageList != 0, "C3DTabGDBBrowser::InitImageLists(), can't create headerImageList" );

		bmp.LoadBitmap( IDB_TABGDBB_TREE_HEADER_IMAGE_LIST );
		headerImageList.Add( &bmp, RGB( 255, 0, 255 ) );
		bmp.DeleteObject();
	}
}


void CComboBoxGDBBrowser::RemoveAllTabs()
{
	wndComboBox.ResetContent();
	//
	for ( int nTabIndex = 0; nTabIndex < tabList.size(); ++nTabIndex )
	{
		if ( tabList[nTabIndex] != 0 )
		{
			if ( ::IsWindow( tabList[nTabIndex]->m_hWnd ) )
			{
				tabList[nTabIndex]->ShowWindow( SW_HIDE );
				tabList[nTabIndex]->DestroyWindow();
			}
			delete ( tabList[nTabIndex] );
			tabList[nTabIndex] = 0;
		}
	}
	tabList.clear();
}


void CComboBoxGDBBrowser::EnableEdit( bool bEnable )
{
	bEnableEdit = bEnable;
	for ( int nTabIndex = 0; nTabIndex < tabList.size(); ++nTabIndex )
	{
		if ( tabList[nTabIndex] != 0 )
		{
			tabList[nTabIndex]->EnableEdit( bEnableEdit );
		}
	}
}


bool CComboBoxGDBBrowser::GetObjectSet( SObjectSet *pObjectSet )
{
	CTreeGDBBrowserBase *pwndActiveTab = 0;
	if ( GetActiveTab( &pwndActiveTab ) )
	{
		if ( pwndActiveTab->IsTreeCreated() )
		{
			return pwndActiveTab->GetCurrentObjectSet( pObjectSet );
		}
	}
	return false;
}


bool CComboBoxGDBBrowser::GetSelectionSet( SSelectionSet *pSelectionSet )
{
	CTreeGDBBrowserBase *pwndActiveTab = 0;
	if ( GetActiveTab( &pwndActiveTab ) )
	{
		if ( pwndActiveTab->IsTreeCreated() )
		{
			return pwndActiveTab->GetCurrentSelectionSet( pSelectionSet );
		}
	}
	return false;
}


bool CComboBoxGDBBrowser::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_OS_GET_OBJECTSET:
			return GetObjectSet( reinterpret_cast<SObjectSet*>( dwData ) );
		case ID_OS_GET_SELECTION:
			return GetSelectionSet( reinterpret_cast<SSelectionSet*>( dwData ) );
		default:
			return false;
	}
}


bool CComboBoxGDBBrowser::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "C3DTabGDBBrowser::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "C3DTabGDBBrowser::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_OS_GET_OBJECTSET:
		case ID_OS_GET_SELECTION:
		{
			CTreeGDBBrowserBase *pwndActiveTab = 0;
			( *pbEnable ) = GetActiveTab( &pwndActiveTab );
			( *pbCheck ) = false;
			return true;
		}
		default:
			return false;
	}
}
/**/

// basement storage  


