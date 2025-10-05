#include "stdafx.h"

#include "ChatControl.h"
#include "UISpecificB2/UISpecificB2.h"

CChatControlWrapper::CChatControlWrapper( IScrollableContainer *_pList, int _nMaxItems )
: pList( _pList ), nMaxItems( _nMaxItems ), nItems( 0 )
{
	if ( !pList )
		return;
	pItem = GetChildChecked<IWindow>( pList, "Item", true );
	if ( pItem )
		pItem->ShowWindow( false );
}

void CChatControlWrapper::AddItem( const std::wstring &wszText )
{
	if ( !pList || !pItem )
		return;

	IWindow *pNewItem = AddWindowCopy( pList, pItem );

	ITextView *pNewItemText = GetChildChecked<ITextView>( pNewItem, "ItemText", true );
	bool bResized = pNewItemText->SetText( pNewItemText->GetDBText() + wszText );
	if ( bResized )
	{
		int nHeight;
		pNewItemText->GetPlacement( 0, 0, 0, &nHeight );
		pNewItem->SetPlacement( 0, 0, 0, nHeight, EWPF_SIZE_Y );
	}

	pList->PushBack( pNewItem, false );
	items.push_back( pNewItem );
	if ( nItems >= nMaxItems )
	{
		pList->Remove( items.front() );
		items.pop_front();
	}
	else
		++nItems;

	pList->EnsureElementVisible( pNewItem );
	pList->Update();
}


