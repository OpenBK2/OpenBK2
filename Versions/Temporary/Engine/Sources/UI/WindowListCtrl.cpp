#include "stdafx.h"
#include "windowlistctrl.h"
#include "InterfaceConsts.h"

REGISTER_SAVELOAD_CLASS( 0x11075B86, CWindowListCtrl)
REGISTER_SAVELOAD_CLASS( 0x11075B85, CWindowListItem )
REGISTER_SAVELOAD_CLASS( 0x11075B84, CWindowListHeader )


//	CWindowListHeader


int CWindowListHeader::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 4, &sorters );
	saver.Add( 5, &nSelectedColumn );
	saver.Add( 6, &bResizable );
	saver.Add( 7, &nResizingIndex );
	saver.Add( 8, &pSortIconDown );
	saver.Add( 9, &pSortIconUp );
	saver.Add( 10, &pInstance );
	saver.Add( 11, &pShared );
	saver.Add( 12, &columnSizes );
	saver.Add( 13, &subitems );
	saver.Add( 14, &pHeaderNotify );

	return 0;
}

void CWindowListHeader::AfterLoad()
{
	CWindow::AfterLoad();
	Init();
}

void CWindowListHeader::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowListHeader *pDesc( checked_cast<const NDb::SWindowListHeader*>( _pDesc ) );
	pInstance = pDesc->Duplicate();
	pShared = checked_cast_ptr<const NDb::SWindowListHeaderShared*>( pDesc->pShared );

	CWindow::InitByDesc( pDesc );

	// optional fields
	pSortIconDown = CUIFactory::MakeWindow( pShared->pSortIconDown );
	pSortIconUp = CUIFactory::MakeWindow( pShared->pSortIconUp );

	MakeInterior();
}

void CWindowListHeader::MakeInterior()
{
	int nXSoFar = 0;
	const int nColumns = pShared->subHeaderSamples.size();
	columnSizes.resize( nColumns );
	subitems.resize( nColumns );
	//
	for ( int i = 0; i < nColumns; ++i )
	{
		// CRAP
		const NDb::SWindow *pSample = pShared->subHeaderSamples[i];
		NI_ASSERT( pSample, "Window not found" );
		IWindow *pChild = static_cast<IWindow*>( CUIFactory::MakeWindow( pSample ) );
		pChild->GetPlacement( 0, 0, &columnSizes[i], 0 );
		subitems[i] = pChild;
		AddChild( pChild, false );
		pChild->ShowWindow( true );
		pChild->SetPlacement( nXSoFar, 0, 0, 0, EWPF_POS_X );
		nXSoFar += columnSizes[i];
	}
	SetPlacement( 0, 0, nXSoFar, 0, EWPF_SIZE_X );

	// ensure that it is enough sorters
	sorters.resize( nColumns );
	for ( int i = 0; i < nColumns; ++i )
	{
		if ( sorters[i] == 0 )
		{
			sorters[i] = MakeObject<IWindowSorter>( SCRNE_UI_LIST_SORTER_ALPHABET );
			sorters[i]->SetColumn( i );
		}
	}
}

void CWindowListHeader::ResizeColumn( const int nPos, const int nResizingIndex )
{
	int nPrevColumnsSize = 0;
	for ( int i = 0; i < nResizingIndex; ++i )
		nPrevColumnsSize += columnSizes[i];

	if ( nPos < nPrevColumnsSize )
	{
		// don't allow to resize
		const bool bAdjust = columnSizes[nResizingIndex] != 0;
		if ( bAdjust )
		{
			columnSizes[nResizingIndex] = 0;
			AdjustColums();
		}
	}
	else
	{
		columnSizes[nResizingIndex] = nPos - nPrevColumnsSize;
		AdjustColums();
	}
}

void CWindowListHeader::AdjustColums()
{
	int nW;
	GetPlacement( 0, 0, &nW, 0 );
	int nXSoFar = 0;
	for ( int i = 0; i < columnSizes.size(); ++i )
	{
		const int nNext = columnSizes[i] + nXSoFar;
		
		if ( nXSoFar > nW )
		{
			columnSizes[i] = 0;
		}
		else if ( nNext > nW )
			columnSizes[i] -= nNext - nW;
		else if ( i == columnSizes.size() -1 ) // last column takes all avalable space
		{
			columnSizes[i] = nW - nXSoFar;
		}
		IWindow *pChild = subitems[i];
		if ( pChild )
			pChild->SetPlacement( nXSoFar, 0, columnSizes[i], 0, EWPF_POS_X | EWPF_SIZE_X );

		nXSoFar = nNext;
	}
	
	if ( pHeaderNotify )
		pHeaderNotify->ColumnsResized( columnSizes );
}

bool CWindowListHeader::OnButtonDblClk( const CVec2 &vPos, const int nButton )
{
	const bool bRes = CWindow::OnButtonDblClk( vPos, nButton );
	if ( (MSTATE_BUTTON1 & nButton) &&
				IsInside( vPos ) && 
				bResizable && 
				pHeaderNotify )
	{
		const int nColumn = GetResizeColumnIndex( vPos ).first;
		if ( -1 != nColumn )
			pHeaderNotify->SetOptimalWidth( nColumn );
	}
	return bRes;
}

bool CWindowListHeader::OnMouseMove( const CVec2 &vPos, const int nButton )
{
	if ( -1 != nResizingIndex )
	{
		if ( MSTATE_BUTTON1 & nButton )
		{
			CTRect<float> rc;
			FillWindowRect( &rc );
			ResizeColumn( vPos.x - rc.left, nResizingIndex );
		}
		else
			nResizingIndex = -1;
	}
	CWindow::OnMouseMove( vPos, nButton );
	
	if ( MSTATE_FREE == nButton )
	{
		//CRAP{ before new scene
		/*
		pair<int, bool> res = GetResizeColumnIndex( vPos );
		if ( IsInside( vPos ) && bResizable && -1 != res.first )
		{
			if ( res.second )
				Singleton<ICursor>()->SetMode( SInterfaceConsts::CURSOR_MODE_LIST_HEADER_RESIZE_2() );
			else
				Singleton<ICursor>()->SetMode( SInterfaceConsts::CURSOR_MODE_LIST_HEADER_RESIZE() );
		}
		else 
			Singleton<ICursor>()->SetMode( SInterfaceConsts::CURSOR_MODE_NORMAL_UI() );
			*/
		//CRAP}
	}
	return false;
}

std::pair<int,bool> CWindowListHeader::GetResizeColumnIndex( const CVec2 &vPos ) const
{
	CTRect<float> rc;
	FillWindowRect( &rc );
	// check if button is down near the border between headers
	const int nX = vPos.x - rc.left;
	int nXSoFar = 0;
	for ( int i = 0; i < columnSizes.size() - 1; ++i )
	{
		nXSoFar += columnSizes[i];
		const int nNextX = nXSoFar + columnSizes[i+1];
		if ( abs(nXSoFar - nX ) <= CInterfaceConsts::LIST_CONTROL_HEADER_RESIZE_OFFSET() )
		{
			if ( nXSoFar < nX && nNextX < nX && i+1 < columnSizes.size() ) // next column is very narrow or 0
				return std::pair<int,bool>( i+1, true );
			return std::pair<int,bool>( i, false );
		}
	}
	return std::pair<int,bool>( -1, false );
}

void CWindowListHeader::UpdateSortButton()
{
	if ( pSortIconDown )
		pSortIconDown->SetParent( 0 );
	if ( pSortIconUp )
		pSortIconUp->SetParent( 0 );
		
	if ( -1 != nSelectedColumn )
	{
		CWindow *pSelectedHeader = dynamic_cast<CWindow*>( subitems[nSelectedColumn].GetPtr() );

		if ( pSelectedHeader )
		{
			if ( sorters[nSelectedColumn]->IsAscending() )
			{
				if ( pSortIconDown )
					pSelectedHeader->AddChild( pSortIconDown, false );
			}
			else
			{
				if ( pSortIconUp )
					pSelectedHeader->AddChild( pSortIconUp, false );
			}
			CTRect<float> rc;
			FillWindowRect( &rc );
			pSelectedHeader->Reposition( rc );
		}
	}
}

void CWindowListHeader::Resort( const int nColumn, const bool bAscending )
{
	nSelectedColumn = nColumn;
	sorters[nColumn]->SetDirection( bAscending );
	UpdateSortButton();
	if ( pHeaderNotify )
		pHeaderNotify->ColumnResort( sorters[nColumn] );
}

bool CWindowListHeader::OnButtonDown( const CVec2 &vPos, const int nButton )
{
	if ( (nButton & MSTATE_BUTTON1) && bResizable && IsInside( vPos ) )
	{
		nResizingIndex = GetResizeColumnIndex( vPos ).first;
		if ( -1 != nResizingIndex )
			return true; // don't need to send this message deeper.
	}

	return CWindow::OnButtonDown( vPos, nButton );
}

void CWindowListHeader::Init()
{
	for ( int i = 0; i < subitems.size(); ++i )
	{
		IButton *pItem = dynamic_cast<IButton*>( subitems[i].GetPtr() );
		pItem->SetNotifySink( this );
	}
	UpdateSortButton();
}

void CWindowListHeader::Reposition( const CTRect<float> &parentRect )
{
	CWindow::Reposition( parentRect );
}

void CWindowListHeader::Released( class CWindow *pWho )
{
	int nColumn = 0;
	for ( int i = 0; i < subitems.size(); ++i )
	{
		if ( pWho == subitems[i] )
		{
			nColumn = i;
			break;
		}
	}
	if ( nColumn == nSelectedColumn )
		sorters[nColumn]->SetDirection( !sorters[nColumn]->IsAscending() );
	nSelectedColumn = nColumn;
	UpdateSortButton();

	if ( pHeaderNotify )
		pHeaderNotify->ColumnResort( sorters[nColumn] );
}

void CWindowListHeader::SetColumnSize( const int nColumn, const int nWidth )
{
	columnSizes[nColumn] = nWidth;
	AdjustColums();
}

bool CWindowListHeader::OnButtonUp( const CVec2 &vPos, const int nButton )
{
	if ( (MSTATE_BUTTON1 & nButton) && -1 != nResizingIndex )
		nResizingIndex = -1;
	return CWindow::OnButtonUp( vPos, nButton );
}


//	CWindowListItem


void CWindowListItem::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{ 
	const NDb::SWindowListItem *pDesc( checked_cast<const NDb::SWindowListItem*>( _pDesc ) );
	pInstance = pDesc->Duplicate();
	pShared = checked_cast_ptr<const NDb::SWindowListItemShared*>( pDesc->pShared );

	CWindow::InitByDesc( pDesc ); 
}

int CWindowListItem::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &pInstance );
	saver.Add( 3, &pShared );
	saver.Add( 4, &subitems );
	saver.Add( 5, &pUserData );
	return 0;
}

void CWindowListItem::MakeInterior( const int nColumns )
{
	NI_ASSERT( pShared->subItemSamples.size() >= nColumns, "Not enough subitems samples" );
	
	subitems.resize( nColumns >= 0 ? nColumns : pShared->subItemSamples.size() );
	for ( int i = 0; i < subitems.size(); ++i )
	{
		const NDb::SWindow *pSubItemDesc = pShared->subItemSamples[i];
		CWindow *pSubItem = checked_cast<CWindow*>( CUIFactory::MakeWindow( pSubItemDesc ) );
		subitems[i] = pSubItem;
		AddChild( pSubItem, nColumns > 0 );
		pSubItem->ShowWindow( true );
	}
}

void CWindowListItem::SetColumnSizes( const std::vector<int> &sizes )
{
	int nPosSoFar = 0;
	for ( int i = 0; i < sizes.size(); ++i )
	{
		IWindow *pChild = subitems[i];
		NI_ASSERT( pChild != 0, StrFmt( "cannot find subitem %i", i ) );
		pChild->SetPlacement( nPosSoFar, 0, sizes[i], 0, EWPF_POS_X | EWPF_SIZE_X );
		nPosSoFar += sizes[i];
	}
	SetPlacement( 0, 0, nPosSoFar, 0, EWPF_SIZE_X );
}

IWindow *CWindowListItem::GetSubItem( const int nSubItem )
{
	NI_ASSERT( nSubItem >= 0 && nSubItem < subitems.size(), "Index out of range" );
	return subitems[nSubItem];
}


//	CWindowListCtrl


int CWindowListCtrl::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindowScrollableContainerBase*>( this ) );
	saver.Add( 2, &pHeader );

	saver.Add( 6, &items );
	saver.Add( 7, &nSortColumn );
	saver.Add( 8, &bSelectable );
	saver.Add( 9, &pInstance );
	saver.Add( 10, &pShared );
	saver.Add( 11, &pViewer );
	saver.Add( 12, &pSelectNotify );
	saver.Add( 13, &pFocusNotify );

	return 0;
}

void CWindowListCtrl::AfterLoad()
{
	CWindow::AfterLoad();
	if ( pHeader )
		pHeader->SetNotify( this );
}

void CWindowListCtrl::SetFocus( const bool bFocus )
{
	CWindowScrollableContainerBase::SetFocus( bFocus );
	
	if ( pFocusNotify )
		pFocusNotify->OnFocus( bFocus );
}

void CWindowListCtrl::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowListCtrl *pDesc( checked_cast<const NDb::SWindowListCtrl*>( _pDesc ) );
	pInstance = pDesc->Duplicate();
	pShared = checked_cast_ptr<const NDb::SWindowListSharedData *>( pDesc->pShared );

	CWindowScrollableContainerBase::InitByDesc( pDesc );
	
	// optional field
	pHeader = dynamic_cast<CWindowListHeader*>( CUIFactory::MakeWindow( pShared->pListHeader ) );
	AddChild( pHeader, false );
}

void CWindowListCtrl::Reposition( const CTRect<float> &parentRect )
{
	CWindowScrollableContainerBase::Reposition( parentRect );
}

void CWindowListCtrl::Init()
{
	CWindowScrollableContainerBase::Init();
	
	if ( pHeader )
	{
		pHeader->Init();
		pHeader->SetNotify( this );
	}
	items.clear();
}

IListControlItem* CWindowListCtrl::AddItemInternal()
{
	CWindowListItem *pItem = checked_cast<CWindowListItem*>( CUIFactory::MakeWindow( pShared->pListItem ) );
	pItem->ShowWindow( true );
	PushBack( pItem, bSelectable );
	items.push_back( pItem );
	if ( pHeader )
	{
		pItem->MakeInterior( pHeader->GetNColumns() );
		pItem->SetColumnSizes( pHeader->GetSizes() );
	}
	else
	{
		pItem->MakeInterior( -1 );
	}
	return pItem;
}

IListControlItem* CWindowListCtrl::AddItem()
{
	return AddItemInternal();
}

IListControlItem* CWindowListCtrl::AddItem( CObjectBase *pData )
{
	IListControlItem *pItem = AddItemInternal();
	pItem->SetUserData( pData);
	if ( pViewer )
		pViewer->MakeInterior( pItem, pData );
	return pItem;
}

void CWindowListCtrl::RemoveItem( IListControlItem *pItem )
{
	if ( pItem )
	{
		IWindow *pItemWnd = dynamic_cast<IWindow*>( pItem );
		items.remove( pItemWnd );
		Remove( pItemWnd );
	}
}

void CWindowListCtrl::ColumnsResized( const std::vector<int> &sizes )
{
	for ( CItems::iterator it = items.begin(); it != items.end(); ++it )
	{
		IWindow *pItem = *it;
		NI_ASSERT( pItem && pItem->IsRefValid(), "item deleted, but not trough list control" );
		if ( pItem )
		{
			CWindowListItem *pI = dynamic_cast<CWindowListItem*>( pItem );
			pI->SetColumnSizes( sizes );
		}
	}
}

void CWindowListCtrl::SetSorter( IWindowSorter *pSorter, const int nColumn )
{
	pHeader->SetSorter( pSorter, nColumn );
}

void CWindowListCtrl::SetViewer( IDataViewer *_pViewer )
{
	pViewer = _pViewer;
}

void CWindowListCtrl::Resort( const int nColumn, const bool bAscending )
{
	pHeader->Resort( nColumn, bAscending );
}

void CWindowListCtrl::ColumnResort( IWindowSorter *pSorter )
{
	// call base class resort
	CWindowScrollableContainerBase::Resort( pSorter );
}

void CWindowListCtrl::SetOptimalWidth( const int nColumn )
{
	int nMaxOptimalWidth = 0;
	for ( CItems::const_iterator it = items.begin(); it != items.end(); ++it )
	{
		IListControlItem *pI = dynamic_cast_ptr<IListControlItem*>( *it );
		CWindow *pSubItem = dynamic_cast<CWindow*>( pI->GetSubItem( nColumn ) );
		
		nMaxOptimalWidth = Max( pSubItem->GetOptimalWidth(), nMaxOptimalWidth  );
	}
	pHeader->SetColumnSize( nColumn, nMaxOptimalWidth );
}

void CWindowListCtrl::Select( IWindow *pElement )
{
	CWindowScrollableContainerBase::Select( pElement );
	
	if ( pSelectNotify )
	{
		CDynamicCast<IListControlItem> pItem = pElement;
		CObjectBase *pData = pItem ? pItem->GetUserData() : 0;
		pSelectNotify->OnSelectData( pData );
	}

	//Play reaction
	if ( GetSelectedItem() )
		RunAnimationAndCommands( pInstance->onSelection, "", true, true );
}

void CWindowListCtrl::RemoveAllElements()
{
	RemoveItems();
	items.clear();
}

int CWindowListCtrl::GetItemCount() const
{
	return items.size();
}

IListControlItem* CWindowListCtrl::GetSelectedListItem() const
{
	return dynamic_cast<IListControlItem*>( CWindowScrollableContainerBase::GetSelectedItem() );
}

void CWindowListCtrl::SelectItem( CObjectBase *pData )
{	
	IWindow *pElement = 0;
	if ( pData )
	{
		for ( CItems::iterator it = items.begin(); it != items.end(); ++it )
		{
			IListControlItem *pWnd = dynamic_cast_ptr<IListControlItem*>( *it );
			if ( pWnd && pWnd->GetUserData() == pData )
			{
				pElement = pWnd;
				break;
			}
		}
	}

	CWindowScrollableContainerBase::Select( pElement );

	if ( pElement )
		EnsureElementVisible( pElement );
}

int CWindowListCtrl::GetRowHeight() const
{
	int nHeight = 0;
	if ( pShared )
	{
		if ( const NDb::SWindowListItem *pItem = pShared->pListItem )
		{
			CParam<CVec2> size = pItem->placement.size;
			if ( const NDb::SWindowShared *pItemShared = dynamic_cast_ptr<const NDb::SWindowShared*>( pItem->pShared ) )
				size.Merge( pItemShared->placement.size.Get() );
			nHeight += size.Get().y;
		}
		nHeight += pShared->nInterval;
	}
	return nHeight;
}

