#include "stdafx.h"

#include "pc_dialog.h"
#include "Tree_GDBBrowserBase.h"
#include "Tree_GDBBrowser_InputState.h"

#define RELAX_RADIUS_2 16


CTreeGDBBrowserInputState::CTreeGDBBrowserInputState() 
	:	pTargetWindow( 0 ), bEnabled( false ), bLeave( false ), bCopy( false ), sourceItem( 0 ), targetItem( 0 ), sourcePoint( 0, 0 ), hDefaultCursor( 0 )
{
	hMoveCursor = ::LoadCursor( ::AfxGetInstanceHandle(), MAKEINTRESOURCE( IDC_DRAG_AND_DROP_MOVE ) );
	hCopyCursor = ::LoadCursor( ::AfxGetInstanceHandle(), MAKEINTRESOURCE( IDC_DRAG_AND_DROP_COPY ) );
	hErrorCursor = ::LoadCursor( 0, IDC_NO );
}


void CTreeGDBBrowserInputState::SetTargetWindow(  CTreeGDBBrowserBase *_pTargetWindow )
{
	pTargetWindow = _pTargetWindow;
}


void CTreeGDBBrowserInputState::Enter()
{
	hDefaultCursor = 0;
	bEnabled = false;
	bLeave = false;
	bCopy = false;
	sourceItem = 0;
	targetItem = 0;
	sourcePoint.x = 0;
	sourcePoint.y = 0;
}


void CTreeGDBBrowserInputState::Leave()
{
	if ( bEnabled )
	{
		::ReleaseCapture();
	}

	hDefaultCursor = 0;
	bEnabled = false;
	bLeave = false;
	bCopy = false;
	sourceItem = 0;
	targetItem = 0;
	sourcePoint.x = 0;
	sourcePoint.y = 0;
}


void CTreeGDBBrowserInputState::BeginDrag( unsigned nFlags )
{
	if ( NGlobal::GetVar( "enable_drag_and_drop", 0 ) != 1 )
	{
		// do not drag
		return;
	}
	if ( pTargetWindow->IsEditEnabled() )
	{
		bEnabled = true;
		::SetCapture( pTargetWindow->m_hWnd );
		if ( nFlags & MK_CONTROL )
		{
			hDefaultCursor = ::SetCursor( hCopyCursor );
		}
		else
		{
			hDefaultCursor = ::SetCursor( hMoveCursor );
		}
	}
}


void CTreeGDBBrowserInputState::ContinueDrag( const CTPoint<int> &rMousePoint, unsigned nFlags )
{
	CRect clientRect;
	pTargetWindow->GetClientRect( &clientRect );
	if ( ( rMousePoint.x < clientRect.left ) ||
			 ( rMousePoint.x > clientRect.right ) ||
			 ( rMousePoint.y < clientRect.top ) ||
			 ( rMousePoint.y > clientRect.bottom ) )
	{
		if ( !bLeave )
		{
			::SetCursor( hErrorCursor );
			bLeave = true;
		}
	}
	else
	{
		if ( bLeave )
		{
			if ( nFlags & MK_CONTROL )
			{
				::SetCursor( hCopyCursor );
				bCopy = true;
			}
			else
			{
				::SetCursor( hMoveCursor );
				bCopy = false;
			}
			bLeave = false;
		}
		else
		{
			if ( nFlags & MK_CONTROL )
			{
				if( !bCopy )
				{
					::SetCursor( hCopyCursor );
					bCopy = true;
				}
			}
			else
			{
				if( bCopy )
				{
					::SetCursor( hMoveCursor );
					bCopy = false;
				}
			}
		}
	}
}


void CTreeGDBBrowserInputState::EndDrag( bool bSuccess )
{
	if ( bSuccess )
	{
		if ( targetItem != 0 )
		{
			CTreeGDBBrowserBase::CTreeOperationList dragTreeOperations;
			HTREEITEM hSelectedItem = pTargetWindow->GetFirstSelectedItem();
			while ( hSelectedItem != 0 )
			{
				if ( hSelectedItem != targetItem )
				{
					if ( pTargetWindow->IsTopSelection( hSelectedItem, targetItem ) )
					{
						CTreeGDBBrowserBase::CTreeOperationList::iterator posNewDragTreeOperation = dragTreeOperations.insert( dragTreeOperations.end(), CTreeGDBBrowserBase::STreeOperation() );
						//
						if ( bCopy )
						{
							posNewDragTreeOperation->nType = CTreeGDBBrowserBase::STreeOperation::TYPE_COPY;
						}
						else
						{
							posNewDragTreeOperation->nType = CTreeGDBBrowserBase::STreeOperation::TYPE_RENAME;
						}
						posNewDragTreeOperation->hDestination = targetItem;
						posNewDragTreeOperation->hSource = hSelectedItem;
					}
				}
				hSelectedItem = pTargetWindow->GetNextSelectedItem( hSelectedItem );
			}
			pTargetWindow->ExecuteTreeOperations( dragTreeOperations );
			pTargetWindow->RedrawWindow();
			pTargetWindow->hLabelEditSortTimerItem = TVI_ROOT;
			pTargetWindow->SetLabelEditSortTimer();
			pTargetWindow->UpdateSelectionManipulator( true );
		}
	}
	::SetCursor( hDefaultCursor );
	if ( targetItem )
	{
		pTargetWindow->SetItemState( targetItem, 0, TVIS_DROPHILITED );
	}
	Leave(); // ReleaseCapture();
}


void CTreeGDBBrowserInputState::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pTargetWindow )
	{
		if ( nFlags & MK_LBUTTON )
		{
			if ( bEnabled )
			{
				unsigned hitTestFlags = 0;
				HTREEITEM newTargetItem = pTargetWindow->HitTest( CPoint( rMousePoint.x, rMousePoint.y ), &hitTestFlags );
				//bool bErrorTarget = true;
				if ( ( newTargetItem != 0 ) &&
						 ( ( ( hitTestFlags & TVHT_ONITEMICON ) != 0 ) || ( ( hitTestFlags & TVHT_ONITEMLABEL ) != 0 ) ) &&
						 ( newTargetItem != targetItem ) &&
						 ( pTargetWindow->GetItemState( newTargetItem, TVIS_SELECTED ) != TVIS_SELECTED ) &&
						 ( pTargetWindow->IsTopSelection( newTargetItem, targetItem ) ) &&
						 ( pTargetWindow->GetTreeItemType( newTargetItem ) == CTreeGDBBrowserBase::GDBO_FOLDER ) )
				{
					if ( targetItem )
					{
						pTargetWindow->SetItemState( targetItem, 0, TVIS_DROPHILITED );
					}
					targetItem = newTargetItem;
					pTargetWindow->SetItemState( targetItem, TVIS_DROPHILITED, TVIS_DROPHILITED );
					//bErrorTarget = false;
				}
				ContinueDrag( rMousePoint, nFlags );
			}
			else if ( sourceItem )
			{
				if ( ( sqr( sourcePoint.x - rMousePoint.x ) + sqr( sourcePoint.y - rMousePoint.y ) ) > RELAX_RADIUS_2 )
				{
					BeginDrag( nFlags );
				}
			}
		}
	}
}


void CTreeGDBBrowserInputState::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pTargetWindow )
	{
		unsigned hitTestFlags = 0;
		sourceItem = pTargetWindow->HitTest( CPoint( rMousePoint.x, rMousePoint.y ), &hitTestFlags );
		if ( sourceItem )
		{
			if ( ( ( hitTestFlags & TVHT_ONITEMICON ) == 0 ) &&
					 ( ( hitTestFlags & TVHT_ONITEMLABEL ) == 0 ) )
			{
				Enter();
			}
			else
			{
				sourcePoint = rMousePoint;
			}
		}
		else if ( ( ( hitTestFlags & TVHT_COLUMNHEADING ) != 0 ) ||
							( ( hitTestFlags & TVHT_COLUMNSEP ) != 0 ) ||
							( ( hitTestFlags & TVIF_EX_STATEEX ) != 0 ) )			
		{
			pTargetWindow->SaveHeaderWidthInternal();
		}
	}
}


void CTreeGDBBrowserInputState::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pTargetWindow )
	{
		if ( bEnabled )
		{
			EndDrag( true );
		}
	}
}


void CTreeGDBBrowserInputState::OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pTargetWindow )
	{
		unsigned hitTestFlags = 0;
		sourceItem = pTargetWindow->HitTest( CPoint( rMousePoint.x, rMousePoint.y ), &hitTestFlags );
		if ( sourceItem )
		{
			if ( ( ( hitTestFlags & TVHT_ONITEMICON ) != 0 ) ||
					 ( ( hitTestFlags & TVHT_ONITEMLABEL ) != 0 ) )
			{
				pTargetWindow->Load();
			}
		}
	}
}


void CTreeGDBBrowserInputState::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( pTargetWindow )
	{
		if ( nFlags & MK_LBUTTON )
		{
			if ( sourceItem )
			{
				BeginDrag( nFlags );
			}
		}
	}
}


void CTreeGDBBrowserInputState::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( pTargetWindow )
	{
		if ( bEnabled )
		{
			if ( nChar == VK_ESCAPE )
			{
				EndDrag( false );
			}
			else if ( nChar == VK_CONTROL )
			{
				POINT point;
				::GetCursorPos( &point );
				pTargetWindow->ScreenToClient( &point );
				ContinueDrag( CTPoint<int>( point.x, point.y ), MK_CONTROL );
			}
		}
		else
		{
			/**
			if ( nChar == VK_DELETE )
			{
				if ( pTargetWindow->IsEditEnabled() && ( pTargetWindow->GetSelectedCount() > 0 ) && pTargetWindow->IsNotEditLabel() )
				{
					pTargetWindow->Delete();
				}
			}
			else if ( nChar == VK_INSERT )
			{
				if ( pTargetWindow->IsEditEnabled() && pTargetWindow->IsNotEditLabel() )
				{
					pTargetWindow->New();
				}
			}
			/**/
			if ( ( nChar == VK_SPACE ) || ( nChar == VK_RETURN ) )
			{
				if ( pTargetWindow->IsEditEnabled() && ( pTargetWindow->GetSelectedCount() == 1 )  && pTargetWindow->IsNotEditLabel() )
				{
					pTargetWindow->Rename();
				}
			}
		}
	}
}


void CTreeGDBBrowserInputState::OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( pTargetWindow )
	{
		if ( bEnabled )
		{
			if ( nChar == VK_CONTROL )
			{
				POINT point;
				::GetCursorPos( &point );
				pTargetWindow->ScreenToClient( &point );
				ContinueDrag( CTPoint<int>( point.x, point.y ), 0 );
			}
		}
	}
}


void CTreeGDBBrowserInputState::OnContextMenu( const CTPoint<int> &rMousePoint )
{
	if ( pTargetWindow )
	{
		if ( !bEnabled )
		{
			pTargetWindow->ShowContextMenu( rMousePoint );
		}
	}
}


// basement storage  


