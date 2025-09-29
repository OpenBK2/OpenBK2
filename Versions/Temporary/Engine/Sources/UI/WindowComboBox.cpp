#include "StdAfx.h"
#include "WindowComboBox.h"
#include "WindowMSButton.h"
#include "WindowListCtrl.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS(0x17122340, CWindowComboBox);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CWindowComboBox
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// При клике на иконку вызывается скроллируемый список.
// Список может располагаться за пределами исходного окна. Стандартный оконный механизм не позволяет
// обрабатывать такое окно как дочернее для исходного - нормальный путь для мышиных сообщений 
// проходит исключительно внутри границ всех родительских окон. Поэтому список делается дочерним для 
// корневого с заданным приоритетом (надо задавать выше чем у всех "обычных" окон, но не тултипов), 
// получает фокус и убирается при его потере.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWindowComboBox::CWindowComboBox() :
	nSelected( -1 ),
	bSuppressPopupList( false ),
	nMaxVisibleRows( 0 )
{
	AddObserver( "window_combo_box_close", OnEscape );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowComboBox *pDesc( checked_cast<const NDb::SWindowComboBox*>( _pDesc ) );
	pInstance = pDesc->Duplicate();
	nMaxVisibleRows = pInstance->nMaxVisibleRows;

	CWindow::InitByDesc( _pDesc );
	pShared = checked_cast_ptr<const NDb::SWindowComboBoxShared*>( pDesc->pShared ) ;
	
	// required field
	CWindow *pLineWnd = CUIFactory::MakeWindow( pShared->pLine );
	pLine = dynamic_cast<CWindowListItem*>( pLineWnd );	
	NI_ASSERT( pLine, "Window not found" );
	if ( pLine )
	{
		pLine->MakeInterior( -1 );
		pLine->SetClickNotify( this );
	}
	AddChild( pLineWnd, false );

	// required field
	pIcon = dynamic_cast<CWindowMSButton*>( CUIFactory::MakeWindow( pShared->pIcon ) );
	NI_ASSERT( pIcon, "Window not found" );
	AddChild( pIcon, false );
	if ( pIcon )
		pIcon->SetNotifySink( this );

	// required field
	pList = dynamic_cast<CWindowListCtrl*>( CUIFactory::MakeWindow( pShared->pList ) );
	NI_ASSERT( pList, "Window not found" );
	ShowList( false );
	if ( pList )
	{
		pList->GetPlacement( &vListPos.x, &vListPos.y, 0, &nListBaseHeight );
		pList->SetSelectNotify( this );
		pList->SetClickNotify( this );
		pList->SetFocusNotify( this );

		nRowHeight = pList->GetRowHeight();
		nListBaseHeight = pList->GetBaseHeight();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::Init()
{
	CWindow::Init();

	if ( pList )
		pList->Init();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::Reposition( const CTRect<float> &parentRect )
{
	CWindow::Reposition( parentRect );
	
	if ( pList )
	{
		CTRect<float> rect = GetWindowRect();
		pList->SetPlacement( rect.x1 + vListPos.x, rect.y1 + vListPos.y, 0, 0, EWPF_POS_X | EWPF_POS_Y );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::ResizeList()
{
	if ( pList && nMaxVisibleRows > 0 )
	{
		NI_ASSERT( GetItemCount() <= nMaxVisibleRows, "Too many items at combo box" );
		pList->SetPlacement( 0, 0, 0, nListBaseHeight + nRowHeight * min( nMaxVisibleRows, GetItemCount() ), EWPF_SIZE_Y );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CWindowComboBox::OnButtonDown( const CVec2 &vPos, const int nButton )
{
	bSuppressPopupList = false;
	bool bResult = CWindow::OnButtonDown( vPos, nButton );
	bSuppressPopupList = false;
	return bResult;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::AddItem( CObjectBase *pData )
{
	if ( !pList )
		return;

	IListControlItem *pItem = pList->AddItem( pData );
	items.push_back( pItem );
	
	ResizeList();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::RemoveItem( CObjectBase *pData )
{
	if ( !pList )
		return;
	
	CItems::iterator pos = find( items.begin(), items.end(), pData );
	if ( pos == items.end() )
		return;
		
	pList->RemoveItem( *pos );
	items.erase( pos );
	
	ResizeList();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::RemoveAllItems()
{
	Select( -1 );
	items.clear();
	pList->RemoveAllElements();	
	
	ResizeList();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IListControlItem* CWindowComboBox::GetItem( int nIndex ) const
{
	NI_VERIFY( 0 <= nIndex && nIndex < items.size(), "Index out of range", return 0 );
	return items[nIndex];
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::Select( int nIndex )
{
	nSelected = nIndex;

	if ( nSelected >= 0 )
	{
		if ( pViewer )
			pViewer->MakeInterior( pLine, items[nSelected]->GetUserData() );
	}
	else
	{
		if ( pViewer )
			pViewer->MakeInterior( pLine, 0 );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::SetViewer( IDataViewer *_pViewer )
{
	pViewer = _pViewer;
	if ( pList )
		pList->SetViewer( pViewer );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::SetLine( CObjectBase *pData ) 
{
	pLineData = pData;
	if ( pViewer )
		pViewer->MakeInterior( pLine, pLineData );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::Pushed( class CWindow *pWho )
{
	if ( bSuppressPopupList )
		return;
		
	ShowList( !IsModalList() );
	bSuppressPopupList = true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CWindowComboBox::IsModalList() const
{
	return pList && pList->IsVisible();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::ShowList( bool bShow )
{
	if ( !pList && pList->IsVisible() != bShow )
		return;

	pList->ShowWindow( bShow );
	if ( bShow )
	{
		pList->SetPriority( pInstance->nListPriority );
		GetRoot()->AddChild( pList, true );
	}
	else
	{
		if ( pList->GetParent() )
			pList->GetParent()->RemoveChild( pList );
	}

	if ( bShow )
		pList->SetFocus( true );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::OnSelectData( CObjectBase *pData )
{
	int nIndex = -1;
	for ( int i = 0; i < items.size(); ++i )
	{
		if ( pData == items[i]->GetUserData() )
		{
			nIndex = i;
			break;
		}
	}
	Select( nIndex );

	if ( pList->GetSelectedItem() )
		RunAnimationAndCommands( pInstance->onSelection, "", true, true );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::Clicked( interface IWindow *pWho, const int nButton )
{
	if ( IsModalList() )
	{
		if ( nButton == MSTATE_BUTTON1 )
			ShowList( false );
	}
	else
	{
		// покажем список также в случае клика в строку комбо-бокса
		if ( !bSuppressPopupList && nButton == MSTATE_BUTTON1 )
			ShowList( true );
	}
	bSuppressPopupList = true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWindowComboBox::OnFocus( const bool bFocus )
{
	if ( !bFocus && IsModalList() && !bSuppressPopupList )
	{
		bSuppressPopupList = true;
		ShowList( false );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CWindowComboBox::OnEscape( const SGameMessage &msg )
{
	if ( IsEnabled() && IsModalList() )
	{
		ShowList( false );
		return true;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CWindowComboBox::IsRelatedFocus( IWindow *pWindow ) const 
{
	return this == pWindow || dynamic_cast_ptr<IWindow*>( pList ) == pWindow; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
