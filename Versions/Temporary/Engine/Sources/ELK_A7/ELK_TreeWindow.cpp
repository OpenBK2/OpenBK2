#include "StdAfx.h"
#include "resource.h"
#include "ELK_TreeWindow.h"
#include "SpellChecker.h"
#include "../System/FileUtils.h"
#include "../Misc/StrProc.h"
#include "ProgressDialog.h"
#include "WMDefines.h"
#include "..\System\FilePath.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CELKTreeWindow::CELKTreeWindow() : pwndFormWindow( 0 ), bCollapseDeselected( true ), bNextFilterChanged( true ), bPreviousFilterChanged( true ), cachedNextItem( 0 ), cachedPreviousItem( 0 ), bCachedNextItemExists( true ), bCachedPreviousItemExists( true )
{
}


CELKTreeWindow::~CELKTreeWindow()
{
}


BEGIN_MESSAGE_MAP(CELKTreeWindow, CTreeDockWindow)
	ON_WM_CREATE()
	ON_NOTIFY(TVN_SELCHANGED, IDC_EMBEDDED_CONTROL, OnSelChanged)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(IDC_BT_STATE_0, OnBtState0)
	ON_COMMAND(IDC_BT_STATE_1, OnBtState1)
	ON_COMMAND(IDC_BT_STATE_2, OnBtState2)
	ON_COMMAND(IDC_BT_STATE_3, OnBtState3)
	ON_COMMAND(ID_VIEW_TREE_STATISTIC, OnViewTreeStatistic)
END_MESSAGE_MAP()



int CELKTreeWindow::OnCreate( LPCREATESTRUCT lpCreateStruct ) 
{
	if ( CTreeDockWindow::OnCreate( lpCreateStruct ) == -1 )
	{
		return -1;
	}

	InitImageList();
	wndTree.SetImageList( &imageListNormal, TVSIL_NORMAL );
	
	return 0;
}


void CELKTreeWindow::InitImageList()
{
	CBitmap bmp;

	// normal tree images
	imageListNormal.Create( 16, 16, true, IMAGE_COUNT, IMAGE_COUNT );
	NI_ASSERT( imageListNormal.m_hImageList != 0, StrFmt( _T( "CELKTreeWindow::InitImageLists(), can't create normal image list" ) ) );

	bmp.LoadBitmap( IDB_ELK_TREE_NORMAL_IMAGE_LIST );
	imageListNormal.Add( &bmp, RGB( 255,255,255 ) );
	bmp.DeleteObject();
}


int CELKTreeWindow::FillFolder( HTREEITEM parentItem, const string &rszFolderName, const string &rszInitialItemPath,  int nInitialELKElement, CProgressDialog *pwndProgressDialog )
{
	int nTextState = SELKTextProperty::STATE_APPROVED;
	
	HTREEITEM folderItem = TVI_LAST;
	HTREEITEM fileItem = 0;
	for ( NFile::CFileIterator fileIterator( ( rszFolderName + string( "*.*" ) ).c_str() ); !fileIterator.IsEnd(); ++fileIterator )
	{
		if ( fileIterator.IsDirectory() && !fileIterator.IsDots() )
		{
			if ( folderItem = wndTree.InsertItem( fileIterator.GetFileName().c_str(), IMAGE_FOLDER_NOT_TRANSLATED, IMAGE_FOLDER_NOT_TRANSLATED, parentItem, folderItem ) )
			{
				int nLocalTextState = FillFolder( folderItem, fileIterator.GetFullName() + string( "\\" ), rszInitialItemPath, nInitialELKElement, pwndProgressDialog );
				if ( wndTree.GetChildCount( folderItem, false, false ) == 0 )
				{
					wndTree.DeleteItem( folderItem );
					folderItem = TVI_LAST;
				}
				else
				{
					wndTree.SetItemImage( folderItem, IMAGE_FOLDER_NOT_TRANSLATED + nLocalTextState, IMAGE_FOLDER_NOT_TRANSLATED + nLocalTextState );
					wndTree.SetItemData( folderItem, MAKELONG( nLocalTextState, false ) );
					if ( nLocalTextState < nTextState )
					{
						nTextState = nLocalTextState;
					}

					if ( ( !rszInitialItemPath.empty() ) && ( nInitialELKElement >= 0 ) )
					{
						string szPath;
						GetItemPathInternal( folderItem, &szPath, false );
						if ( ( rszInitialItemPath == szPath ) && ( nInitialELKElement == GetELKElementNumberInternal( folderItem ) ) )
						{
							SelectItem( folderItem );
						}
					}
				}
			}
		}
		else if ( !fileIterator.IsDirectory() && !fileIterator.IsDots() )
		{
			string szFileExt = NFile::GetFileExt( fileIterator.GetFileName() );
			NStr::ToLower( &szFileExt );
			if ( szFileExt == CELK::ELK_EXTENTION )
			{
				string szFileTitle = NFile::GetFileTitle( fileIterator.GetFileName() );
				if ( fileItem = wndTree.InsertItem( szFileTitle.c_str(), IMAGE_TEXT_NOT_TRANSLATED, IMAGE_TEXT_NOT_TRANSLATED, parentItem, fileItem ) )
				{
					string szFileName = fileIterator.GetFullName().substr( 0, fileIterator.GetFullName().rfind( '.' ) );
					bool bTranslated = false;
					int nLocalTextState = CELK::GetState( szFileName, &bTranslated );
					if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
					{
						pwndProgressDialog->IterateProgressPosition();
					}

					wndTree.SetItemImage( fileItem, IMAGE_TEXT_NOT_TRANSLATED + nLocalTextState, IMAGE_TEXT_NOT_TRANSLATED + nLocalTextState );
					wndTree.SetItemData( fileItem, MAKELONG( nLocalTextState, bTranslated ) );
					if ( nLocalTextState < nTextState )
					{
						nTextState = nLocalTextState;
					}

					if ( ( !rszInitialItemPath.empty() ) && ( nInitialELKElement >= 0 ) )
					{
						string szPath;
						GetItemPathInternal( fileItem, &szPath, false );
						if ( ( rszInitialItemPath == szPath ) && ( nInitialELKElement == GetELKElementNumberInternal( fileItem ) ) )
						{
							SelectItem( fileItem );
						}
					}
				}
			}
		}
	}
	
	return nTextState;
}


void CELKTreeWindow::ClearTree()
{
	wndTree.DeleteAllItems();
	rootFolderMap.clear();
	rootNameMap.clear();
	rootNumberMap.clear();
}


// Функтор для EnumerateFiles, вычисляет количество файлов
class CGetFilesCount
{
	int *pFilesCount;

public:

	CGetFilesCount( int *_pFilesCount ) : pFilesCount( _pFilesCount ) {}

	void operator() ( const NFile::CFileIterator &rFileIterator )
	{
		if ( !rFileIterator.IsDirectory() )
		{
			++( *pFilesCount );
		}
	}
};


void CELKTreeWindow::FillTree( const class CELK &rELK, const string &rszInitialItemPath, int nInitialELKElement, CProgressDialog *pwndProgressDialog )
{
	for ( int nElementIndex = 0; nElementIndex < rELK.elementList.size(); ++nElementIndex )
	{
		const SELKElement &rELKElement = rELK.elementList[nElementIndex];
		string szBaseName = StrFmt( _T( "%s, VERSION: %s" ), rELKElement.description.szName.c_str(), rELKElement.szVersion.c_str() );

		string szDataBaseFolder;
		rELKElement.GetDataBaseFolder( &szDataBaseFolder );

		if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
		{
			pwndProgressDialog->ShowWindow( SW_SHOW );
			pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Opening ELK: %s..." ), rELKElement.szPath.c_str() ) );
			
			int nFilesCount = 0;
			NFile::EnumerateFiles( szDataBaseFolder.c_str(), StrFmt( _T( "*%s" ), CELK::ELK_EXTENTION ), CGetFilesCount( &nFilesCount ), true );

			pwndProgressDialog->SetProgressRange( 0, nFilesCount );
			pwndProgressDialog->SetProgressPosition( 0 );
		}

		HTREEITEM baseItem = wndTree.InsertItem( szBaseName.c_str(), IMAGE_ROOT_NOT_TRANSLATED_EXPANDED, IMAGE_ROOT_NOT_TRANSLATED_EXPANDED, TVI_ROOT );
		if ( baseItem != 0 )
		{
			
			rootFolderMap[reinterpret_cast<LONG>( baseItem )] = szDataBaseFolder;
			rootNameMap[reinterpret_cast<LONG>( baseItem )] = rELKElement.szPath.substr( rELKElement.szPath.rfind( '\\' ) + 1 );
			rootNumberMap[reinterpret_cast<LONG>( baseItem )] = nElementIndex;
			int nTextState = FillFolder( baseItem, szDataBaseFolder, rszInitialItemPath, nInitialELKElement, pwndProgressDialog );
			wndTree.SetItemImage( baseItem, IMAGE_ROOT_NOT_TRANSLATED_EXPANDED + nTextState, IMAGE_ROOT_NOT_TRANSLATED_EXPANDED + nTextState );
			if ( ( !rszInitialItemPath.empty() ) && ( nInitialELKElement >= 0 ) )
			{
				string szPath;
				GetItemPathInternal( baseItem, &szPath, false );
				if ( ( rszInitialItemPath == szPath ) && ( nInitialELKElement == GetELKElementNumberInternal( baseItem ) ) )
				{
					SelectItem( baseItem );
				}
			}
		}
	}
}


void CELKTreeWindow::GetItemPathInternal( HTREEITEM item, string *pszItemPath, bool bFull )
{
	NI_ASSERT( pszItemPath != 0, StrFmt( _T( "CELKTreeWindow::GetItemPath() wrong parameter: pszItemPath %x" ), pszItemPath ) );

	if ( pszItemPath )
	{
		pszItemPath->clear();
		HTREEITEM childItem = wndTree.GetChildItem( item );
		while ( HTREEITEM parentItem = wndTree.GetParentItem( item ) )
		{
			string szLocalPath = wndTree.GetItemText( item );
			( *pszItemPath ) = szLocalPath + string( "\\" ) + ( *pszItemPath );
			item = parentItem;
		}
		if ( bFull )
		{
			( *pszItemPath ) = rootFolderMap[reinterpret_cast<LONG>( item )] + ( *pszItemPath );
		}
		if ( !childItem )
		{
			( *pszItemPath ) = pszItemPath->substr( 0, pszItemPath->rfind( '\\' ) );
		}
	}
}


int CELKTreeWindow::GetELKElementNumberInternal( HTREEITEM item )
{
	while ( HTREEITEM parentItem = wndTree.GetParentItem( item ) )
	{
		item = parentItem;
	}
	hash_map<LONG, int>::const_iterator itRootNumber = rootNumberMap.find( reinterpret_cast<LONG>( item ) );
	if ( itRootNumber != rootNumberMap.end() )
	{
		return itRootNumber->second;
	}
	return ( -1 );
}


void CELKTreeWindow::GetELKElementNameInternal( HTREEITEM item, string *pszName )
{
	NI_ASSERT( pszName != 0, StrFmt( _T( "CELKTreeWindow::GetELKElementNameInternal() wrong parameter: pszName %x" ), pszName ) );
	if ( pszName )
	{
		pszName->clear();
		while ( HTREEITEM parentItem = wndTree.GetParentItem( item ) )
		{
			item = parentItem;
		}
		hash_map<LONG, string>::const_iterator itRootName = rootNameMap.find( reinterpret_cast<LONG>( item ) );
		if ( itRootName != rootNameMap.end() )
		{
			( *pszName ) = itRootName->second;
		}
	}
}


void CELKTreeWindow::GetELKElementPathInternal( HTREEITEM item, string *pszPath )
{
	NI_ASSERT( pszPath != 0, StrFmt( _T( "CELKTreeWindow::GetELKElementPathInternal() wrong parameter: pszPath %x" ), pszPath ) );
	if ( pszPath )
	{
		pszPath->clear();
		while ( HTREEITEM parentItem = wndTree.GetParentItem( item ) )
		{
			item = parentItem;
		}
		hash_map<LONG, string>::const_iterator itRootFolder = rootFolderMap.find( reinterpret_cast<LONG>( item ) );
		if ( itRootFolder != rootFolderMap.end() )
		{
			( *pszPath ) = itRootFolder->second;
		}
	}
}


void CELKTreeWindow::GetXLSPathInternal( HTREEITEM item, string *pszItemPath )
{
	NI_ASSERT( pszItemPath != 0, StrFmt( _T( "CELKTreeWindow::GetXLSPathInternal() wrong parameter: pszItemPath %x" ), pszItemPath ) );

	if ( pszItemPath )
	{
		pszItemPath->clear();
		HTREEITEM childItem = wndTree.GetChildItem( item );
		while ( HTREEITEM parentItem = wndTree.GetParentItem( item ) )
		{
			string szLocalPath = wndTree.GetItemText( item );
			( *pszItemPath ) = szLocalPath + string( "\\" ) + ( *pszItemPath );
			item = parentItem;
		}
		( *pszItemPath ) = rootNameMap[reinterpret_cast<LONG>( item )] + string( "\\" ) + ( *pszItemPath );
		if ( !childItem )
		{
			( *pszItemPath ) = pszItemPath->substr( 0, pszItemPath->rfind( '\\' ) );
		}
	}
}


void CELKTreeWindow::GetItemPath( string *pszItemPath, bool bFull )
{
	NI_ASSERT( pszItemPath != 0, StrFmt( _T( "CELKTreeWindow::GetItemPath() wrong parameter: pszItemPath %x" ), pszItemPath ) );

	if ( pszItemPath )
	{
		if ( HTREEITEM item = wndTree.GetFirstSelectedItem() )
		{
			GetItemPathInternal( item, pszItemPath, bFull );
		}
	}
}
	


int CELKTreeWindow::GetELKElementNumber()
{
	if ( HTREEITEM item = wndTree.GetFirstSelectedItem() )
	{
		return GetELKElementNumberInternal( item );
	}
	else
	{
		return ( -1 );
	}
}


int CELKTreeWindow::UpdateFolderState( HTREEITEM item )
{
	int nPreviousState = LOWORD( wndTree.GetItemData( item ) );
	int nNewState = SELKTextProperty::STATE_APPROVED;
	HTREEITEM childItem = wndTree.GetChildItem( item );
	while ( childItem )
	{
		int nChildState = LOWORD( wndTree.GetItemData( childItem ) );
		if ( nChildState < nNewState )
		{
			nNewState = nChildState;
		}
		childItem = wndTree.GetNextSiblingItem( childItem );
	}
	if ( nNewState != nPreviousState )
	{
		wndTree.SetItemData( item, MAKELONG( nNewState, false ) );
		HTREEITEM parentItem = wndTree.GetParentItem( item );
		if ( parentItem )
		{
			wndTree.SetItemImage( item, IMAGE_FOLDER_NOT_TRANSLATED + nNewState, IMAGE_FOLDER_NOT_TRANSLATED + nNewState );
			UpdateFolderState( parentItem );
		}
		else
		{
			wndTree.SetItemImage( item, IMAGE_ROOT_NOT_TRANSLATED_EXPANDED + nNewState, IMAGE_ROOT_NOT_TRANSLATED_EXPANDED + nNewState );
		}
	}
	return nPreviousState;
}


void CELKTreeWindow::OnSelChanged( NMHDR* pNMHDR, LRESULT* pResult ) 
{
	if ( HTREEITEM item = wndTree.GetFirstSelectedItem() )
	{
		HTREEITEM childItem = wndTree.GetChildItem( item );
		if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
		{
			if ( childItem == 0 )
			{
				pwndMainFrame->SendMessage( WM_ELK_TREE_NOTIFY, ETN_TEXT_SELECTED, LOWORD( wndTree.GetItemData( item ) ) );
			}
			else
			{
				pwndMainFrame->SendMessage( WM_ELK_TREE_NOTIFY, ETN_FOLDER_SELECTED, LOWORD( wndTree.GetItemData( item ) ) );
			}
		}
	}
}


void CELKTreeWindow::OnContextMenu( CWnd *pwnd, CPoint point )
{
	if ( pwndFormWindow )
	{
		if ( HTREEITEM item = wndTree.GetFirstSelectedItem() )
		{
			int nState = LOWORD( wndTree.GetItemData( item ) );
			CMenu statesMenu;
			statesMenu.LoadMenu( IDM_BT_POPUP_MENUS );
			CMenu *pMenu = statesMenu.GetSubMenu( 0 );
			if ( pMenu )
			{
				if ( CWnd* pWnd = pwndFormWindow->GetDlgItem( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON ) )
				{
					pMenu->EnableMenuItem( IDC_BT_STATE_0, pWnd->IsWindowEnabled() ? MF_ENABLED : MF_GRAYED );
				}
				if ( CWnd* pWnd = pwndFormWindow->GetDlgItem( IDC_IV_OUTDATED_RADIO_BUTTON ) )
				{
					pMenu->EnableMenuItem( IDC_BT_STATE_1, pWnd->IsWindowEnabled() ? MF_ENABLED : MF_GRAYED );
				}
				if ( CWnd* pWnd = pwndFormWindow->GetDlgItem( IDC_IV_TRANSLATED_RADIO_BUTTON ) )
				{
					pMenu->EnableMenuItem( IDC_BT_STATE_2, pWnd->IsWindowEnabled() ? MF_ENABLED : MF_GRAYED );
				}
				if ( CWnd* pWnd = pwndFormWindow->GetDlgItem( IDC_IV_APPROVED_RADIO_BUTTON ) )
				{
					pMenu->EnableMenuItem( IDC_BT_STATE_3, pWnd->IsWindowEnabled() ? MF_ENABLED : MF_GRAYED );
				}
				pMenu->CheckMenuRadioItem( IDC_BT_STATE_0, IDC_BT_STATE_3, IDC_BT_STATE_0 + nState, MF_BYCOMMAND );
				CPoint point;
				GetCursorPos( &point );
				pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, this, 0 );
			}
			statesMenu.DestroyMenu();
		}
	}
}


void CELKTreeWindow::SelectItem( HTREEITEM item )
{
	if ( item )
	{
		wndTree.EnsureVisible( item, true );
		wndTree.SelectItem( item );
	}
}


void CELKTreeWindow::DeselectItem( HTREEITEM item )
{
	if ( item && bCollapseDeselected )
	{
		wndTree.CollapseCompletely( wndTree.GetRootItem( item ), false );
	}
}


HTREEITEM CELKTreeWindow::GetNextItemInternal( HTREEITEM item )
{
	if ( HTREEITEM nextItem = item )
	{
		if ( wndTree.GetChildItem( nextItem ) )
		{
			while ( wndTree.GetChildItem( nextItem ) )
			{
				nextItem = wndTree.GetChildItem( nextItem );
			}
			return nextItem;
		}
		HTREEITEM parentItem = item;
		nextItem = wndTree.GetNextSiblingItem( nextItem );
		while ( !nextItem )
		{
			parentItem = wndTree.GetParentItem( parentItem );
			if ( parentItem )
			{
				nextItem = wndTree.GetNextSiblingItem( parentItem );
			}
			else
			{
				return 0;
			}
		}
		while ( wndTree.GetChildItem( nextItem ) )
		{
			nextItem = wndTree.GetChildItem( nextItem );
		}
		return nextItem;
	}
	return 0;
}


HTREEITEM CELKTreeWindow::GetPreviousItemInternal( HTREEITEM item )
{
	if ( HTREEITEM nextItem = item )
	{
		if ( wndTree.GetChildItem( nextItem ) )
		{
			while ( wndTree.GetChildItem( nextItem ) )
			{
				nextItem = wndTree.GetChildItem( nextItem );
			}
			return nextItem;
		}
		HTREEITEM parentItem = item;
		nextItem = wndTree.GetPrevSiblingItem( nextItem );
		while ( !nextItem )
		{
			parentItem = wndTree.GetParentItem( parentItem );
			if ( parentItem )
			{
				nextItem = wndTree.GetPrevSiblingItem( parentItem );
			}
			else
			{
				return 0;
			}
		}
		while ( wndTree.GetChildItem( nextItem ) )
		{
			nextItem = wndTree.GetChildItem( nextItem );
			while ( wndTree.GetNextSiblingItem( nextItem ) )
			{
				nextItem = wndTree.GetNextSiblingItem( nextItem );
			}
		}
		return nextItem;
	}
	return 0;
}


HTREEITEM CELKTreeWindow::GetFirstItemInternal()
{
	HTREEITEM item = wndTree.GetRootItem( 0 );
	if ( item )
	{
		while ( wndTree.GetChildItem( item ) )
		{
			item = wndTree.GetChildItem( item );
		}
		return item;
	}
	return 0;
}


HTREEITEM CELKTreeWindow::GetLastItemInternal()
{
	HTREEITEM item = wndTree.GetRootItem( 0 );
	if ( item )
	{
		while ( wndTree.GetNextSiblingItem( item ) )
		{
			item = wndTree.GetNextSiblingItem( item );
		}
		while ( wndTree.GetChildItem( item ) )
		{
			item = wndTree.GetChildItem( item );
			while ( wndTree.GetNextSiblingItem( item ) )
			{
				item = wndTree.GetNextSiblingItem( item );
			}
		}
		return item;
	}
	return 0;
}


int CELKTreeWindow::GetItemsCountInternal()
{
	int nItemsCount = 0;
	if ( HTREEITEM item = GetFirstItemInternal() )
	{
		++nItemsCount;
		while ( item = GetNextItemInternal( item ) )
		{
			++nItemsCount;
		}
	}
	return nItemsCount;
}



bool CELKTreeWindow::GetFirstItem( const SSimpleFilter *pELKFilter )
{
	if ( HTREEITEM item = wndTree.GetFirstSelectedItem() )
	{
		DeselectItem( item );
	}

	HTREEITEM item = GetFirstItemInternal();
	if ( item != 0 )
	{
		while (  item != 0 )
		{
			if ( pELKFilter )
			{
				LPARAM itemData = wndTree.GetItemData( item );
				int nItemState = LOWORD( itemData );
				bool bTranslated = HIWORD( itemData );
				string szItemPath;
				GetItemPathInternal( item, &szItemPath, false );
				if ( pELKFilter->Check( szItemPath, bTranslated, nItemState ) )
				{
					break;
				}
			}
			else
			{
				break;
			}
			item = GetNextItemInternal( item );
		}
		if ( item )
		{
			SelectItem( item );
			return true;
		}
	}
	return false;
}


bool CELKTreeWindow::GetLastItem( const SSimpleFilter *pELKFilter )
{
	if ( HTREEITEM item = wndTree.GetFirstSelectedItem() )
	{
		DeselectItem( item );
	}

	HTREEITEM item = GetLastItemInternal();
	if ( item != 0 )
	{
		while (  item != 0 )
		{
			if ( pELKFilter )
			{
				LPARAM itemData = wndTree.GetItemData( item );
				int nItemState = LOWORD( itemData );
				bool bTranslated = HIWORD( itemData );
				string szItemPath;
				GetItemPathInternal( item, &szItemPath, false );
				if ( pELKFilter->Check( szItemPath, bTranslated, nItemState ) )
				{
					break;
				}
			}
			else
			{
				break;
			}
			item = GetPreviousItemInternal( item );
		}
		if ( item )
		{
			SelectItem( item );
			return true;
		}
	}
	return false;
}


bool CELKTreeWindow::GetNextItem( const SSimpleFilter *pELKFilter )
{
	if ( HTREEITEM item = wndTree.GetFirstSelectedItem() )
	{
		DeselectItem( item );
		while ( item = GetNextItemInternal( item ) )
		{
			if ( pELKFilter )
			{
				LPARAM itemData = wndTree.GetItemData( item );
				int nItemState = LOWORD( itemData );
				bool bTranslated = HIWORD( itemData );
				string szItemPath;
				GetItemPathInternal( item, &szItemPath, false );
				if ( pELKFilter->Check( szItemPath, bTranslated, nItemState ) )
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		if ( item != 0 )
		{
			SelectItem( item );
			return true;
		}
	}
	return false;
}


bool CELKTreeWindow::GetPreviousItem( const SSimpleFilter *pELKFilter )
{
	if ( HTREEITEM item = wndTree.GetFirstSelectedItem() )
	{
		DeselectItem( item );

		while ( item = GetPreviousItemInternal( item ) )
		{
			if ( pELKFilter )
			{
				LPARAM itemData = wndTree.GetItemData( item );
				int nItemState = LOWORD( itemData );
				bool bTranslated = HIWORD( itemData );
				string szItemPath;
				GetItemPathInternal( item, &szItemPath, false );
				if ( pELKFilter->Check( szItemPath, bTranslated, nItemState ) )
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		if ( item != 0 )
		{
			SelectItem( item );
			return true;
		}
	}
	return false;
}


bool CELKTreeWindow::IsNextItemExists( const SSimpleFilter *pELKFilter )
{
	HTREEITEM item = wndTree.GetFirstSelectedItem();
	if ( item != 0 )
	{
		if ( ( !bNextFilterChanged ) && ( cachedNextItem == item ) )
		{
			return bCachedNextItemExists;		
		}
		bNextFilterChanged = false;
		cachedNextItem = item;
		while ( item = GetNextItemInternal( item ) )
		{
			if ( pELKFilter )
			{
				LPARAM itemData = wndTree.GetItemData( item );
				int nItemState = LOWORD( itemData );
				bool bTranslated = HIWORD( itemData );
				string szItemPath;
				GetItemPathInternal( item, &szItemPath, false );
				if ( pELKFilter->Check( szItemPath, bTranslated, nItemState ) )
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
	}
	bCachedNextItemExists = ( item != 0 );
	return bCachedNextItemExists;
}


bool CELKTreeWindow::IsPreviousItemExists( const SSimpleFilter *pELKFilter )
{
	HTREEITEM item = wndTree.GetFirstSelectedItem();
	if ( item != 0 )
	{
		if ( ( !bPreviousFilterChanged ) && ( cachedPreviousItem == item ) )
		{
			return bCachedPreviousItemExists;		
		}
		bPreviousFilterChanged = false;
		cachedPreviousItem = item;
		while ( item = GetPreviousItemInternal( item ) )
		{
			if ( pELKFilter )
			{
				LPARAM itemData = wndTree.GetItemData( item );
				int nItemState = LOWORD( itemData );
				bool bTranslated = HIWORD( itemData );
				string szItemPath;
				GetItemPathInternal( item, &szItemPath, false );
				if ( pELKFilter->Check( szItemPath, bTranslated, nItemState ) )
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
	}
	bCachedPreviousItemExists = ( item != 0 );
	return bCachedPreviousItemExists;
}


int CELKTreeWindow::GetItemsCount(  const SSimpleFilter *pELKFilter )
{
	int nItemsCount = 0;
	if ( HTREEITEM item = GetFirstItemInternal() )
	{
		if ( pELKFilter )
		{
			LPARAM itemData = wndTree.GetItemData( item );
			int nItemState = LOWORD( itemData );
			bool bTranslated = HIWORD( itemData );
			string szItemPath;
			GetItemPathInternal( item, &szItemPath, false );
			if ( pELKFilter->Check( szItemPath, bTranslated, nItemState ) )
			{
				++nItemsCount;
			}
		}
		else
		{
			++nItemsCount;
		}
		while ( item = GetNextItemInternal( item ) )
		{
			if ( pELKFilter )
			{
				LPARAM itemData = wndTree.GetItemData( item );
				int nItemState = LOWORD( itemData );
				bool bTranslated = HIWORD( itemData );
				string szItemPath;
				GetItemPathInternal( item, &szItemPath, false );
				if ( pELKFilter->Check( szItemPath, bTranslated, nItemState ) )
				{
					++nItemsCount;
				}
			}
			else
			{
				++nItemsCount;
			}
		}
	}
	return nItemsCount;
}



void CELKTreeWindow::SetCollapseItem( bool bCollapseItem )
{
	bCollapseDeselected = bCollapseItem;
}


bool CELKTreeWindow::FindItem( const class CELK &rELK, SMainFrameParams::SSearchParam *pSearchParam, int nCodePage )
{
	if ( pSearchParam )
	{
		if ( HTREEITEM item = wndTree.GetFirstSelectedItem() )
		{
			CString strFindText = pSearchParam->strFindText;
			CString strWordDelimeters( CSpellChecker::SPELLING_WORD_DELIMITERS );
			
			if ( !pSearchParam->bFindMatchCase )
			{
				strFindText.MakeLower();
			}

			HTREEITEM newItem = item;
			if ( wndTree.GetChildCount( item ) > 0 )
			{
				pSearchParam->bFindDown ? ( newItem = GetNextItemInternal( newItem ) ) : ( newItem = GetPreviousItemInternal( newItem ) );
			}
			CString strText;
			while ( newItem )
			{
				if ( pSearchParam->nWindowType >= SMainFrameParams::SSearchParam::WT_COUNT )
				{
					pSearchParam->bFindDown ? ( newItem = GetNextItemInternal( newItem ) ) : ( newItem = GetPreviousItemInternal( newItem ) );
					pSearchParam->nWindowType = SMainFrameParams::SSearchParam::WT_ORIGINAL;
					pSearchParam->nPosition = 0;
					strText.Empty();
				}
				if ( newItem == 0 )
				{
					return false;
				}
				
				string szItemPath;
				GetItemPathInternal( newItem, &szItemPath, true );
				
				if ( strText.IsEmpty() )
				{
					if ( pSearchParam->nWindowType == SMainFrameParams::SSearchParam::WT_ORIGINAL )
					{
						CELK::GetOriginalText( szItemPath, &strText, nCodePage, false );
					}
					else if ( pSearchParam->nWindowType == SMainFrameParams::SSearchParam::WT_DESCRIPTION )
					{
						CELK::GetDescription( szItemPath, &strText, nCodePage, false );
					}
					else
					{
						CELK::GetTranslatedText( szItemPath, &strText, nCodePage, false );
					}
					if ( !pSearchParam->bFindMatchCase )
					{
						strText.MakeLower();
					}
				}
				if ( !strText.IsEmpty() )
				{
					pSearchParam->nPosition = strText.Find( strFindText, pSearchParam->nPosition );
					if ( pSearchParam->nPosition >= 0 )
					{
						bool bValid = true;
						if ( pSearchParam->bFindWholeWord )
						{
							if ( pSearchParam->nPosition > 0 )
							{
								if ( strWordDelimeters.Find( strText[pSearchParam->nPosition - 1] ) < 0 )
								{
									bValid = false;
								}
							}
							if ( bValid )
							{
								if ( ( pSearchParam->nPosition + strFindText.GetLength() ) < strText.GetLength() )
								{
									if ( strWordDelimeters.Find( strText[pSearchParam->nPosition + strFindText.GetLength()] ) < 0 )
									{
										bValid = false;
									}
								}
							}
						}
						if ( bValid )
						{
							if ( newItem != item )
							{
								DeselectItem( item );
								SelectItem( newItem );
							}
							return true;
						}
					}
				}
				strText.Empty();
				pSearchParam->nWindowType += 1;
				pSearchParam->nPosition = 0;
			}			
		}
	}
	return false;
}


void CELKTreeWindow::UpdateSelectedText( CELK *pELK, int nState )
{
	if ( pELK )
	{
		string szPath;
		if ( HTREEITEM item = wndTree.GetFirstSelectedItem() )
		{
			GetItemPathInternal( item, &szPath, true );

			bool bTranslated = false;
			int nPreviousState = CELK::SetState( szPath, nState, &bTranslated );
			if ( nPreviousState != nState )
			{
				wndTree.SetItemImage( item, IMAGE_TEXT_NOT_TRANSLATED + nState, IMAGE_TEXT_NOT_TRANSLATED + nState );
				wndTree.SetItemData( item, MAKELONG( nState, bTranslated ) );
				item = wndTree.GetParentItem( item );
				if ( item )
				{
					UpdateFolderState( item );
				}
			}
		}
	}
}


bool CELKTreeWindow::IsItemParent( HTREEITEM item, HTREEITEM parentItem )
{
	if ( item != parentItem )
	{
		while ( item != 0 )
		{
			if ( item == parentItem )
			{
				return true;
			}
			item = wndTree.GetParentItem( item );
		}
	}
	return false;
}


void CELKTreeWindow::UpdateSelectedFolder( CELK *pELK, int nState )
{
	if ( pELK )
	{
		if ( HTREEITEM selectedItem = wndTree.GetFirstSelectedItem() )
		{
			HTREEITEM item = GetNextItemInternal( selectedItem );

			while ( IsItemParent( item, selectedItem ) )
			{
				string szPath;
				GetItemPathInternal( item, &szPath, true );
				bool bTranslated = false;
				int nPreviousState = CELK::SetState( szPath, nState, &bTranslated );
				if ( nPreviousState != nState )
				{
					wndTree.SetItemImage( item, IMAGE_TEXT_NOT_TRANSLATED + nState, IMAGE_TEXT_NOT_TRANSLATED + nState );
					wndTree.SetItemData( item, MAKELONG( nState, bTranslated ) );
					
					HTREEITEM parentItem = wndTree.GetParentItem( item );
					if ( parentItem )
					{
						UpdateFolderState( parentItem );
					}
				}
				
				item = GetNextItemInternal( item );
			}
		}
	}
}


void CELKTreeWindow::OnBtState0() 
{
	if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
	{
		pwndMainFrame->SendMessage( WM_ELK_TREE_NOTIFY, ETN_STATE_CHANGED, SELKTextProperty::STATE_NOT_TRANSLATED );
	}
}


void CELKTreeWindow::OnBtState1() 
{
	if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
	{
		pwndMainFrame->SendMessage( WM_ELK_TREE_NOTIFY, ETN_STATE_CHANGED, SELKTextProperty::STATE_OUTDATED );
	}
}


void CELKTreeWindow::OnBtState2() 
{
	if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
	{
		pwndMainFrame->SendMessage( WM_ELK_TREE_NOTIFY, ETN_STATE_CHANGED, SELKTextProperty::STATE_TRANSLATED );
	}
}


void CELKTreeWindow::OnBtState3() 
{
	if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
	{
		pwndMainFrame->SendMessage( WM_ELK_TREE_NOTIFY, ETN_STATE_CHANGED, SELKTextProperty::STATE_APPROVED );
	}
}


void CELKTreeWindow::OnViewTreeStatistic()
{
	pwndMainFrame->SendMessage( WM_ELK_TREE_NOTIFY, ETN_SHOW_TREE_STATISTIC, 0 );
}


