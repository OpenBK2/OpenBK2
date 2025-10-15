#include "stdafx.h"
#include "window1lvltreecontrol.h"
#include "WindowMSButton.h"

REGISTER_SAVELOAD_CLASS(UI, 0x11075B81, CWindow1LvlTreeControl)

int CWindow1LvlTreeControl::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindowScrollableContainerBase*>( this ) );
	saver.Add( 2, &items );
	saver.Add( 5, &pInstance );
	saver.Add( 6, &pShared );
	return 0;
}

void CWindow1LvlTreeControl::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindow1LvlTreeControl *pDesc( checked_cast<const NDb::SWindow1LvlTreeControl*>( _pDesc ) );
	pInstance = pDesc->Duplicate();

	CWindowScrollableContainerBase::InitByDesc( _pDesc );
	pShared = checked_cast_ptr<const NDb::SWindow1LvlTreeControlShared*>( pDesc->pShared );
}

IWindow * CWindow1LvlTreeControl::AddItem()
{
	CPtr<CWindowMSButton> pInsert = dynamic_cast<CWindowMSButton*>( CUIFactory::MakeWindow( pShared->pItemSample ) );
	pInsert->Init();

	pInsert->SetNotifySink( this );
	pInsert->ShowWindow( true );
	items.push_back( SItem(pInsert) );
	PushBack( pInsert, 0 );
	return items.back().pItem;
}

void CWindow1LvlTreeControl::Expand( CWindow1LvlTreeControl::SItem *pContainer )
{
	pContainer->bCollapsed = false;
	if ( !pContainer->subitems.empty() ) 
	{
		for ( SItem::CSubItems::iterator it = pContainer->subitems.end(); ; )
		{
			--it;
			InsertAfter( pContainer->pItem, *it, true );
			if ( it == pContainer->subitems.begin() ) 
				break;
		}
	}
	/*
	for ( SItem::CSubItems::reverse_iterator it = pContainer->subitems.rbegin(); it != pContainer->subitems.rend(); ++it )
		InsertAfter( pContainer->pItem, *it, true );
	*/
	Update();
}

void CWindow1LvlTreeControl::Collapse( CWindow1LvlTreeControl::SItem *pContainer )
{
	pContainer->bCollapsed = true;
	if ( !pContainer->subitems.empty() ) 
	{
		for ( SItem::CSubItems::iterator it = pContainer->subitems.end(); ; )
		{
			--it;
			Remove( *it );
			if ( it == pContainer->subitems.begin() ) 
				break;
		}
	}
	/*
	for ( SItem::CSubItems::reverse_iterator it = pContainer->subitems.rbegin(); it != pContainer->subitems.rend(); ++it )
		Remove( *it );
	*/
	Update();
}

CWindow1LvlTreeControl::SItem * CWindow1LvlTreeControl::GetContainer( IWindow* pCont )
{
	for ( std::vector<SItem>::iterator it = items.begin(); it != items.end(); ++it )
	{
		if ( it->pItem == pCont )
		{
			return &(*it);
		}
	}
	return 0;
}

void CWindow1LvlTreeControl::Released( class CWindow *pWho )
{
	SItem * pCont = GetContainer( pWho );
	if ( pCont )
		pCont->bCollapsed ? Expand( pCont ) : Collapse( pCont );
}

void CWindow1LvlTreeControl::Init()
{
	CWindow::Init();
	RemoveItems();
	items.clear();
}

IWindow * CWindow1LvlTreeControl::AddSubItem( IWindow *pItem )
{
	SItem *pCont = GetContainer( pItem );
	NI_ASSERT( pCont != 0, StrFmt( "not found container \"%s\"", dynamic_cast<CWindow*>(pItem)->GetName().c_str() ) );
	if ( pCont )
	{
		IWindow *pPrev;
		if ( !pCont->subitems.empty() )
			pPrev = pCont->subitems.back();
		else
			pPrev = pCont->pItem;
		CPtr<CWindow> pInsert = dynamic_cast<CWindow*>( CUIFactory::MakeWindow( pShared->pSubItemSample ) );
		pInsert->Init();
		pCont->subitems.push_back( pInsert.GetPtr() );
		pInsert->ShowWindow( true );
		if ( !pCont->bCollapsed )
			InsertAfter( pPrev, pInsert, 0 );
		return pInsert;
	}
	return 0;
}

