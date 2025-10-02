#include "stdafx.h"
#include "ResourceDefines.h"

#include "ControlAlgorithms.h"
#include "..\Misc\StrProc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace NCA
{
	bool IsParent( HTREEITEM hItem, HTREEITEM hParentItem, const SECTreeCtrl *pTreeControl )
	{
		NI_ASSERT( pTreeControl != 0, "IsParent(): Invalid parameter: pTreeControl == 0" );
		if ( pTreeControl == 0 )
		{
			return false;
		}
		HTREEITEM hLocalParentItem = hItem;
		while ( hParentItem != 0 )
		{
			if ( hLocalParentItem == hParentItem )
			{
				return true;
			}
			hLocalParentItem = pTreeControl->GetParentItem( hLocalParentItem );
		}
		return false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void AddFullName( string *pszFullName, HTREEITEM hItem, const string &rszTreeItemDivider, const SECTreeCtrl *pTreeControl )
	{
		string szItemText = pTreeControl->GetItemText( hItem );
		if ( pszFullName->empty() )
		{
			( *pszFullName ) = szItemText;
		}
		else
		{
			( *pszFullName ) = szItemText + rszTreeItemDivider + ( *pszFullName );
		}
		if ( HTREEITEM hParentItem = pTreeControl->GetParentItem( hItem ) )
		{
			AddFullName( pszFullName, hParentItem, rszTreeItemDivider, pTreeControl );
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CreateFullName( string *pszFullName, HTREEITEM hItem, const string &rszTreeItemDivider, const SECTreeCtrl *pTreeControl )
	{
		NI_ASSERT( pszFullName != 0, "CreateFullName(): Invalid parameter: pszFullName == 0" );
		NI_ASSERT( pTreeControl != 0, "CreateFullName(): Invalid parameter: pTreeControl == 0" );
		if ( ( pszFullName == 0 ) || ( pTreeControl == 0 ) )
		{
			return;
		}
		pszFullName->clear();
		AddFullName( pszFullName, hItem, rszTreeItemDivider, pTreeControl );
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool IsTopSelection( HTREEITEM hItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl )
	{
		NI_ASSERT( pTreeControl != 0, "IsTopTreeSelection(): Invalid parameter: pTreeControl == 0" );
		if ( pTreeControl == 0 )
		{
			return false;
		}
		//
		HTREEITEM hParentItem = pTreeControl->GetParentItem( hItem );
		while ( hParentItem != 0 )
		{
			if ( ( hParentItem != hItemToSkip ) && ( ( pTreeControl->GetItemState( hParentItem, TVIS_SELECTED  ) & TVIS_SELECTED ) > 0 ) )
			{
				return false;
			}
			hParentItem = pTreeControl->GetParentItem( hParentItem );
		}
		return true;
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int AddFocused( CHTREEITEMList *pSelection, HTREEITEM hParentItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl )
	{
		HTREEITEM hSelectedItem = pTreeControl->GetSelectedItem();
		if ( ( hSelectedItem != 0 ) && ( hSelectedItem != hItemToSkip ) )
		{
			if ( hParentItem != 0 )
			{
				if ( IsParent( hSelectedItem, hParentItem, pTreeControl ) )
				{
					pSelection->push_back( hSelectedItem );
					return 1;
				}
			}
			else
			{
				pSelection->push_back( hSelectedItem );
				return 1;
			}
		}
		return 0;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int AddSelected( CHTREEITEMList *pSelection, HTREEITEM hParentItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl )
	{
		int nCount = 0;
		HTREEITEM hSelectedItem = pTreeControl->GetFirstSelectedItem();
		while ( hSelectedItem != 0 )
		{
			if ( hSelectedItem != hItemToSkip )
			{
				if ( hParentItem != 0 )
				{
					if ( IsParent( hSelectedItem, hParentItem, pTreeControl ) )
					{
						pSelection->push_back( hSelectedItem );
						++nCount;
					}
				}
				else
				{
					pSelection->push_back( hSelectedItem );
					++nCount;
				}
			}
			hSelectedItem = pTreeControl->GetNextSelectedItem( hSelectedItem );
		}
		return nCount;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int AddComplete( CHTREEITEMList *pSelection, HTREEITEM hParentItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl )
	{
		int nCount = 0;
		if ( hParentItem == 0 )
		{
			hParentItem = TVI_ROOT;
		}
		else
		{
			if ( hParentItem != hItemToSkip )
			{
				pSelection->push_back( hParentItem );
				++nCount;
			}
		}
		HTREEITEM hSiblingItem = pTreeControl->GetChildItem( hParentItem );
		while ( hSiblingItem != 0 )
		{
			if ( hSiblingItem != hItemToSkip )
			{
				if ( pTreeControl->ItemHasChildren( hSiblingItem ) )
				{
					nCount += AddComplete( pSelection, hSiblingItem, hItemToSkip, pTreeControl );
				}
				else
				{
					pSelection->push_back( hSiblingItem );
					++nCount;
				}
			}
			hSiblingItem = pTreeControl->GetNextSiblingItem( hSiblingItem );
		}
		return nCount;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int AddCompleteFocused( CHTREEITEMList *pSelection, HTREEITEM hParentItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl )
	{
		HTREEITEM hSelectedItem = pTreeControl->GetSelectedItem();
		if ( ( hSelectedItem != 0 ) && ( hSelectedItem != hItemToSkip ) )
		{
			if ( hParentItem != 0 )
			{
				if ( IsParent( hSelectedItem, hParentItem, pTreeControl ) )
				{
					return AddComplete( pSelection, hSelectedItem, hItemToSkip, pTreeControl );
				}
			}
			else
			{
				return AddComplete( pSelection, hSelectedItem, hItemToSkip, pTreeControl );
			}
		}
		return 0;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int AddCompleteSelected( CHTREEITEMList *pSelection, HTREEITEM hParentItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl )
	{
		int nCount = 0;
		HTREEITEM hSelectedItem = pTreeControl->GetFirstSelectedItem();
		while ( hSelectedItem != 0 )
		{
			if ( ( hSelectedItem != hItemToSkip ) && IsTopSelection( hSelectedItem, hItemToSkip, pTreeControl ) ) 
			{
				if ( hParentItem != 0 )
				{
					if ( IsParent( hSelectedItem, hParentItem, pTreeControl ) )
					{
						nCount += AddComplete( pSelection, hSelectedItem, hItemToSkip, pTreeControl );
					}
				}
				else
				{
					nCount += AddComplete( pSelection, hSelectedItem, hItemToSkip, pTreeControl );
				}
			}
			hSelectedItem = pTreeControl->GetNextSelectedItem( hSelectedItem );
		}
		return nCount;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int CreateSelection( CHTREEITEMList *pSelection, ESelectionType eSelectionType, HTREEITEM hParentItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl )
	{
		NI_ASSERT( pTreeControl != 0, "CreateSelection(): Invalid parameter: pSelection == 0" );
		NI_ASSERT( pSelection != 0, "CreateSelection(): Invalid parameter: pTreeControl == 0" );
		if ( ( pSelection == 0 ) || ( pTreeControl == 0 ) )
		{
			return 0;
		}
		pSelection->clear();
		switch ( eSelectionType )
		{
			case ST_FOCUS:
			{
				return AddFocused( pSelection, hParentItem, hItemToSkip, pTreeControl );
			}
			case ST_SELECT:
			default:
			{
				return AddSelected( pSelection, hParentItem, hItemToSkip, pTreeControl );
			}
			case ST_COMPLETE:
			{
				return AddComplete( pSelection, hParentItem, hItemToSkip, pTreeControl );
				break;
			}
			case ST_COMPLETE_FOCUS:
			{
				return AddCompleteFocused( pSelection, hParentItem, hItemToSkip, pTreeControl );
				break;
			}
			case ST_COMPLETE_SELECT:
			{
				return AddCompleteSelected( pSelection, hParentItem, hItemToSkip, pTreeControl );
				break;
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SelectAll( HTREEITEM hParentItem, HTREEITEM hItemToSkip, SECTreeCtrl *pTreeControl )
	{
		NI_ASSERT( pTreeControl != 0, "CreateSelection(): Invalid parameter: pSelection == 0" );
		if ( pTreeControl == 0 )
		{
			return;
		}
		if ( hParentItem == 0 )
		{
			hParentItem = TVI_ROOT;
		}
		if ( hParentItem != TVI_ROOT )
		{
			pTreeControl->SetItemState( hParentItem, TVIS_SELECTED, TVIS_SELECTED );
		}
		if ( hParentItem == hItemToSkip )
		{
			return;
		}
		if ( pTreeControl->GetItemState( hParentItem, TVIS_EXPANDED | TVIS_EXPANDPARTIAL ) > 0 )
		{
			HTREEITEM hSiblingItem = pTreeControl->GetChildItem( hParentItem );
			while ( hSiblingItem != 0 )
			{
				if ( hSiblingItem != hItemToSkip )
				{
					if ( pTreeControl->ItemHasChildren( hSiblingItem ) )
					{
						SelectAll( hSiblingItem, hItemToSkip, pTreeControl );
					}
					else
					{
						pTreeControl->SetItemState( hSiblingItem, TVIS_SELECTED, TVIS_SELECTED );
					}
				}
				hSiblingItem = pTreeControl->GetNextSiblingItem( hSiblingItem );
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void FillWindowsClipboard( const CHTREEITEMList &rSelection,
														 const string &rszClipboardPrefix,
														 const string &rszClipboardPostfix,
														 const string &rszItemPrefix,
														 const string &rszItemPostfix,
														 const string &rszTreeItemDivider,
														 const string &rszColumnDivider,
														 const SECTreeCtrl *pTreeControl )
	{
		NI_ASSERT( pTreeControl != 0, "CreateSelection(): Invalid parameter: pSelection == 0" );
		if ( pTreeControl == 0 )
		{
			return;
		}
		string szWindowsClipboardText = rszClipboardPrefix;
		const int nColumnCount = pTreeControl->GetColumnCount();
		for ( CHTREEITEMList::const_iterator itSelection = rSelection.begin(); itSelection != rSelection.end(); ++itSelection )
		{
			if ( !szWindowsClipboardText.empty() )
			{
				szWindowsClipboardText += "\r\n";
			}
			//
			string szFullName;
			CreateFullName( &szFullName, *itSelection, rszTreeItemDivider, pTreeControl );
			szWindowsClipboardText += rszItemPrefix + szFullName;
			//
			if ( nColumnCount > 1 )
			{
				for ( int nColumnIndex = 1; nColumnIndex < nColumnCount; ++nColumnIndex )
				{
					szFullName = pTreeControl->GetItemText( *itSelection, nColumnIndex );
					szWindowsClipboardText += rszColumnDivider + szFullName;
				}
			}
			szWindowsClipboardText += rszItemPostfix;
		}
		if ( !szWindowsClipboardText.empty() && !rszClipboardPostfix.empty() )
		{
			if ( !szWindowsClipboardText.empty() )
			{
				szWindowsClipboardText += "\r\n";
			}
			szWindowsClipboardText += rszClipboardPostfix;
		}
		if ( ::OpenClipboard( pTreeControl->m_hWnd ) && ::EmptyClipboard() )
		{
			if ( !szWindowsClipboardText.empty() )
			{
				wstring wszWindowsClipboardText;
				NStr::ToUnicode( &wszWindowsClipboardText, szWindowsClipboardText );
				HGLOBAL hglbCopy = ::GlobalAlloc( GMEM_MOVEABLE, ( wszWindowsClipboardText.size() + 1 ) * sizeof( wchar_t ) ); 
				if ( hglbCopy != 0 ) 
				{ 
					// Lock the handle and copy the text to the buffer. 
					LPVOID lptstrCopy = ::GlobalLock( hglbCopy ); 
					memset( lptstrCopy, 0, ( wszWindowsClipboardText.size() + 1 ) * sizeof( wchar_t ) ); 
					memcpy( lptstrCopy, wszWindowsClipboardText.c_str(), wszWindowsClipboardText.size() * sizeof( wchar_t ) ); 
					::GlobalUnlock( hglbCopy ); 
					::SetClipboardData( CF_UNICODETEXT, hglbCopy );
				}
			}
			::CloseClipboard();
		}
	}
	//
	bool TranslateAccelerators( bool bModal, UINT nMessage, WPARAM wParam, LPARAM lParam )
	{
		if ( HACCEL hAcceleratorTable = ::LoadAccelerators( ::AfxGetResourceHandle(), bModal ? MAKEINTRESOURCE( IDA_MODAL ) : MAKEINTRESOURCE( IDA_MAIN ) ) )
		{
			acceleratorMessage.hwnd = ::AfxGetMainWnd()->m_hWnd;
			acceleratorMessage.message = nMessage;
			acceleratorMessage.wParam = wParam;
			acceleratorMessage.lParam = lParam;
			acceleratorMessage.time = ::GetTickCount();
			acceleratorMessage.pt.x = 0;
			acceleratorMessage.pt.y = 0;
			return ::TranslateAccelerator( ::AfxGetMainWnd()->m_hWnd, hAcceleratorTable, &acceleratorMessage );
		}
		return false;
	}
};

// basement storage  

