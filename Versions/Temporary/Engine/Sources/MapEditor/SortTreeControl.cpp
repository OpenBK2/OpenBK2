#include "stdafx.h"

#include "SortTreeControl.h"
#include "../libdb/Manipulator.h"
#include "..\Misc\StrProc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CSortTreeControl, SECTreeCtrl)
END_MESSAGE_MAP()


HTREEITEM CSortTreeControl::GetTreeItem( const string &rszName )
{
	if ( !rszName.empty() )
	{
		string szName = rszName;
		if ( IsIgnoreCase() )
		{
			NStr::ToLower( &szName );
		}
		CTreeItemMap::const_iterator posTreeItem = treeItemMap.find( szName );
		if ( posTreeItem != treeItemMap.end() )
		{
			//DebugTrace("CSortTreeControl::GetTreeItem( %s ) Cached!", szName.c_str() );
			return posTreeItem->second;
		}
	}
	return 0;
}


void CSortTreeControl::ClearNameCache( const string &rszName )
{
	string szName = rszName;
	if ( IsIgnoreCase() )
	{
		NStr::ToLower( &szName );
	}
	if ( !szName.empty() )
	{
		for ( CTreeItemMap::iterator itTreeItem = treeItemMap.begin(); itTreeItem != treeItemMap.end(); )
		{
			if ( itTreeItem->first.compare( 0, szName.size(), szName ) == 0 )
			{
				//DebugTrace("CSortTreeControl::ClearNameCache( %s )", itTreeItem->first.c_str() );
				treeItemMap.erase( itTreeItem++ );
			}
			else
			{
				++itTreeItem;
			}
		}
	}
	else
	{
		treeItemMap.clear();
	}
}


void CSortTreeControl::SetNameCache( const string &rszName, HTREEITEM hItem )
{
	if ( !rszName.empty() &&
			 ( hItem != 0 ) && 
			 ( hItem != TVI_ROOT ) && 
			 ( hItem != TVI_FIRST ) && 
			 ( hItem != TVI_LAST ) )
	{	
		string szName = rszName;
		if ( IsIgnoreCase() )
		{
			NStr::ToLower( &szName );
		}
		//DebugTrace("CSortTreeControl::SetNameCache( %s )", rszName.c_str() );
		treeItemMap[szName] = hItem;
	}
}


void CSortTreeControl::RemoveTreeItemFromMaps( HTREEITEM hItem )
{
	string szHashName;
	if ( GetTreeItemName( hItem, &szHashName ) && !szHashName.empty() )
	{
		if ( IsIgnoreCase() )
		{
			NStr::ToLower( &szHashName );
		}
		CTreeItemMap::iterator posTreeItem = treeItemMap.find( szHashName );
		if ( posTreeItem != treeItemMap.end() )
		{
			treeItemMap.erase( posTreeItem );
		}
		//
		CTreeItemMap::iterator posClipboardTreeItem = clipboardTreeItemMap.find( szHashName );
		if ( posClipboardTreeItem != clipboardTreeItemMap.end() )
		{
			clipboardTreeItemMap.erase( posClipboardTreeItem );
		}
	}
	RemoveTreeItemColorFromCache( hItem );
	RemoveTreeItemReadOnlyFromCache( hItem ); 
	HTREEITEM hChildItem = GetChildItem( hItem );
	while ( hChildItem )
	{
		RemoveTreeItemFromMaps( hChildItem );
		hChildItem = GetNextSiblingItem( hChildItem );
	}
}


void CSortTreeControl::SortTree( HTREEITEM hParentItem, PFNTVCOMPARE pfnCompare, LPARAM lParam )
{
	/**
	if ( ItemHasChildren( hParentItem ) )
	{
		HTREEITEM hItem = GetChildItem( hParentItem );
		while ( hItem != 0 )
		{
			SortPCTree( hItem );
			hItem = GetNextSiblingItem( hItem );
		}
		TVSORTCB tvs;
		tvs.hParent = hParentItem;
		tvs.lpfnCompare = PCMainTreeControlCompareFunc;
		tvs.lParam = reinterpret_cast<LPARAM>( this );
		SortChildrenCB( &tvs );			
	}
	/**/
	TVSORTCB tvs;
	tvs.hParent = hParentItem;
	tvs.lpfnCompare = pfnCompare;
	tvs.lParam = lParam;
	SortChildrenCB( &tvs );
}


HTREEITEM CSortTreeControl::InsertTreeItem( LPCTSTR lpszItem, int nImage, int nSelectedImage, HTREEITEM hParent , HTREEITEM hInsertAfter )
{
	if ( HTREEITEM hItem = InsertItem( lpszItem, nImage, nSelectedImage, hParent, hInsertAfter ) )
	{
		SetItemData( hItem, reinterpret_cast<DWORD>( hItem ) );
		//
		string szHashName;
		if ( GetTreeItemName( hItem, &szHashName ) )
		{
			SetNameCache( szHashName, hItem );
		}
		//
		return hItem;
	}
	return 0;
}


bool CSortTreeControl::RenameTreeItem( const string &rszDestination, const string &rszSource )
{
	string szDestination = rszDestination;
	string szSource = rszSource;
	if ( IsIgnoreCase() )
	{
		NStr::ToLower( &szDestination );
		NStr::ToLower( &szSource );
	}
	CTreeItemMap newElements;
	for ( CTreeItemMap::iterator itTreeItem = treeItemMap.begin(); itTreeItem != treeItemMap.end(); )
	{
		if ( itTreeItem->first.compare( 0, szSource.size(), szSource ) == 0 )
		{
			newElements[szDestination + itTreeItem->first.substr( szSource.size() )] = itTreeItem->second;
			treeItemMap.erase( itTreeItem++ );
		}
		else
			++itTreeItem;
	}
	treeItemMap.insert( newElements.begin(), newElements.end() );
	return true;
}


bool CSortTreeControl::DeleteAllTreeItems()
{
	treeItemMap.clear();
	treeItemColorMap.clear();
	treeItemReadOnlyMap.clear();
	clipboardTreeItemMap.clear();
	return DeleteAllItems();
}


bool CSortTreeControl::DeleteTreeItem( HTREEITEM hItem )
{
	RemoveTreeItemFromMaps( hItem );
	return DeleteItem( hItem );
}


bool CSortTreeControl::IsTopSelection( HTREEITEM hItem, HTREEITEM hItemToSkip )
{
	HTREEITEM hParentItem = GetParentItem( hItem );
	while ( hParentItem != 0 )
	{
		if ( ( hParentItem != hItemToSkip ) && ( ( GetItemState( hParentItem, TVIS_SELECTED  ) & TVIS_SELECTED ) > 0 ) )
		{
			return false;
		}
		hParentItem = GetParentItem( hParentItem );
	}
	return true;
}


void CSortTreeControl::FillWindowsClipboard( HTREEITEM hItem, string *pszWindowClipboardText )
{
	HTREEITEM hChildItem = GetChildItem( hItem );
	if ( hChildItem )
	{
		while ( hChildItem )
		{
			FillWindowsClipboard( hChildItem, pszWindowClipboardText );
			hChildItem = GetNextSiblingItem( hChildItem );
		}
	}
	else
	{
		if ( pszWindowClipboardText )
		{
			string szClipboardPrefix;
			GetClipboardPrefix( &szClipboardPrefix );
			string szHashName;
			GetTreeItemName( hItem, &szHashName );
			const int nColumnCount = GetColumnCount();
			//
			if ( szClipboardPrefix.empty() )
			{
				( *pszWindowClipboardText ) += StrFmt( "%s", szHashName.c_str() );
			}
			else
			{
				( *pszWindowClipboardText ) += StrFmt( "%s\t%s", szClipboardPrefix.c_str(), szHashName.c_str() );
			}
			//
			if ( nColumnCount > 1 )
			{
				for ( int nColumnIndex = 1; nColumnIndex < nColumnCount; ++nColumnIndex )
				{
					const string szText = GetItemText( hItem, nColumnIndex );
					( *pszWindowClipboardText ) += "\t" + szText;
				}
			}
			//
			( *pszWindowClipboardText ) += "\r\n";
		}
	}
}


void CSortTreeControl::FillClipboard( bool _bClipboardCut )
{
	clipboardTreeItemMap.clear();
	HTREEITEM hSelectedItem = GetFirstSelectedItem();
	string szWindowClipboardText;
	while ( hSelectedItem != 0 )
	{
		string szHashName;
		GetTreeItemName( hSelectedItem, &szHashName );
		if ( IsIgnoreCase() )
		{
			NStr::ToLower( &szHashName );
		}
		//
		if ( IsTopSelection( hSelectedItem, 0 ) )
		{
			clipboardTreeItemMap[szHashName] = hSelectedItem;
			FillWindowsClipboard( hSelectedItem, &szWindowClipboardText );
		}
		hSelectedItem = GetNextSelectedItem( hSelectedItem );
	}
	bClipboardCut = _bClipboardCut;
	// Записываем данные в Clipboard
  if ( OpenClipboard() && EmptyClipboard() )
  {
		HGLOBAL hglbCopy = ::GlobalAlloc( GMEM_MOVEABLE, ( szWindowClipboardText.size() + 1 ) * sizeof( TCHAR ) ); 
		if ( hglbCopy != 0 ) 
		{ 
			// Lock the handle and copy the text to the buffer. 
			LPVOID lptstrCopy = ::GlobalLock( hglbCopy ); 
			memset( lptstrCopy, 0, ( szWindowClipboardText.size() + 1 ) * sizeof( TCHAR ) ); 
			memcpy( lptstrCopy, szWindowClipboardText.c_str(), szWindowClipboardText.size() * sizeof( TCHAR ) ); 
			::GlobalUnlock( hglbCopy ); 
			::SetClipboardData( CF_TEXT, hglbCopy );
		}
    CloseClipboard();
	}
}


bool CSortTreeControl::IsClipboardItem( HTREEITEM hItem ) const
{
	if ( ( hItem == 0 ) || ( hItem == TVI_ROOT ) )
	{
		return false;
	}
	for ( CTreeItemMap::const_iterator itClipboardTreeItem = clipboardTreeItemMap.begin(); itClipboardTreeItem != clipboardTreeItemMap.end(); ++itClipboardTreeItem )
	{
		HTREEITEM hParentItem = hItem;
		while ( hParentItem )
		{
			if ( hParentItem == itClipboardTreeItem->second )
			{
				return true;
			}
			hParentItem = GetParentItem( hParentItem );
		}
	}
	return false;
}


void CSortTreeControl::SetTreeItemColor( HTREEITEM hItem, COLORREF color )
{
	treeItemColorMap[hItem] = color;
}


bool CSortTreeControl::GetTreeItemColor( HTREEITEM hItem, COLORREF *pColor )
{
	//NI_ASSERT( pColor != 0, "CSortTreeControl::GetTreeItemColor(): pColor == 0" );
	CTreeItemColorMap::iterator posTreeItemColor = treeItemColorMap.find( hItem );
	if ( posTreeItemColor != treeItemColorMap.end() )
	{
		//DebugTrace("CSortTreeControl::GetTreeItemColor() Cached!" );
		( *pColor ) = posTreeItemColor->second;
		return true;
	}
	return false;
}


void CSortTreeControl::RemoveTreeItemColorFromCache( HTREEITEM hItem )
{
	CTreeItemColorMap::iterator posTreeItemColor = treeItemColorMap.find( hItem );
	if ( posTreeItemColor != treeItemColorMap.end() )
	{
		treeItemColorMap.erase( posTreeItemColor );
	}
}


void CSortTreeControl::SetTreeItemReadOnly( HTREEITEM hItem, bool bReadOnly )
{
	treeItemReadOnlyMap[hItem] = bReadOnly;
}


bool CSortTreeControl::GetTreeItemReadOnly( HTREEITEM hItem, bool *pbReadOnly )
{
	//NI_ASSERT( pbReadOnly != 0, "CSortTreeControl::GetTreeItemReadOnly(): pbReadOnly == 0" );
	CTreeItemBoolMap::iterator posTreeItemReadOnly = treeItemReadOnlyMap.find( hItem );
	if ( posTreeItemReadOnly != treeItemReadOnlyMap.end() )
	{
		//DebugTrace("CSortTreeControl::GetTreeItemReadOnly() Cached!" );
		( *pbReadOnly ) = posTreeItemReadOnly->second;
		return true;
	}
	return false;
}


void CSortTreeControl::RemoveTreeItemReadOnlyFromCache( HTREEITEM hItem )
{
	CTreeItemBoolMap::iterator posTreeItemReadOnly = treeItemReadOnlyMap.find( hItem );
	if ( posTreeItemReadOnly != treeItemReadOnlyMap.end() )
	{
		treeItemReadOnlyMap.erase( posTreeItemReadOnly );
	}
}

// basement storage  

