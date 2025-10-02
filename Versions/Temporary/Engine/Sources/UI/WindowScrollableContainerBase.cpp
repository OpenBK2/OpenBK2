#include "StdAfx.h"
#include "windowscrollablecontainerbase.h"

#include "WindowScrollBar.h"
#include "UIVisitor.h"
#include "windowsimple.h"

int CWindowScrollableContainerBase::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
  saver.Add( 2, &pSelected );
	saver.Add( 3, &pShared );
	saver.Add( 4, &elementIDs );
	saver.Add( 5, &nPosSize );															// total size of elements
	saver.Add( 6, &pScrollBar );	
	saver.Add( 7, &pBorder );  // for clipping
	saver.Add( 8, &pContainer );  // contains elements
	saver.Add( 9, &elements );
	saver.Add( 10, &pSelection );
	saver.Add( 11, &pPreSelection );
	saver.Add( 12, &pPreSelected );
	return 0;
}

void CWindowScrollableContainerBase::AfterLoad()
{
	CWindow::AfterLoad();
	if ( pScrollBar )
		pScrollBar->SetNotifySink( this );
}

void CWindowScrollableContainerBase::Visit( interface IUIVisitor *pVisitor )
{
	CTRect<float> rc;
	FillWindowRect( &rc );
	VirtualToScreen( rc, &rc );
	CClipStore s( pVisitor, rc );
	CWindow::Visit( pVisitor );
}

void CWindowScrollableContainerBase::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	CWindow::InitByDesc( _pDesc );

	const NDb::SWindowScrollableContainerBase *pDesc = checked_cast<const NDb::SWindowScrollableContainerBase*>( _pDesc );
  pShared = checked_cast_ptr<const NDb::SWindowScrollableContainerBaseShared *>( pDesc->pShared );

	// optional field  
	pScrollBar = checked_cast<CWindowScrollBar*>( CUIFactory::MakeWindow( pShared->pScrollBar ) );
	AddChild( pScrollBar, false );
	
	// required field
	pBorder = dynamic_cast<CWindowSimple*>( CUIFactory::MakeWindow( pShared->pBorder ) );
	AddChild( pBorder, false );
	NI_ASSERT( pBorder, "Border not found" );

	// optional field  
	pSelection = checked_cast<CWindow*>( CUIFactory::MakeWindow( pShared->pSelection ) );
	pPreSelection = checked_cast<CWindow*>( CUIFactory::MakeWindow( pShared->pPreSelection ) );

	pContainer = dynamic_cast<CWindowSimple*>( CUIFactory::MakeWindow( pShared->pBorder ) );	
	NI_ASSERT( pContainer, "Can't create container" );
	if ( pContainer )
	{
		pContainer->SetName( "" ); // служебные элементы не должны быть именованы
		while ( pContainer->GetNumChildren() > 0 ) // клонированных детей у служебного элемента быть не должно
		{
			pContainer->RemoveChild( pContainer->GetChild( 0 ) );
		}
	}

	UpdateScrollBar();
}

void CWindowScrollableContainerBase::Reposition( const CTRect<float> &parentRect )
{
	if ( !pBorder || !pContainer )
		return;

	if ( pSelection )
		RemoveChild( pSelection );
	if ( pPreSelection )
		RemoveChild( pPreSelection );

	CWindow::Reposition( parentRect );
	if ( pScrollBar )
		pScrollBar->SetNotifySink( this );

	int nBorderH, nBorderW;
	pBorder->GetPlacement( 0, 0, &nBorderW, &nBorderH );
	pContainer->SetPlacement( 0, 0, nBorderW, nBorderH, EWPF_POS_X | EWPF_SIZE_X | EWPF_SIZE_Y );
	pContainer->SetAllign( NDb::EPA_LOW_END, NDb::EPA_LOW_END );
	pBorder->AddChild( pContainer, true );

	if ( pSelection )
	{
		pSelection->SetName("");				
		NI_VERIFY( pSelection->IsTransparent(),
			"Error: Selection mask should be transparent, Setting Priority to -1", 
			pSelection->SetPriority( -1 ) );		
		pContainer->AddChild( pSelection, true );
	}
	if ( pPreSelection )
	{
		pPreSelection->SetName("");
		NI_VERIFY( pPreSelection->IsTransparent(),
			"Error: PreSelection mask should be transparent, Setting Priority to -1", 
			pPreSelection->SetPriority( -1 ) );		
		pContainer->AddChild( pPreSelection, true );
	}


}

CWindowScrollableContainerBase::CElements::iterator CWindowScrollableContainerBase::GetAfter( IWindow *pElement )
{
	CElements::iterator after = elements.begin();
	for ( ; after != elements.end(); ++after )
	{
		if ( after->szName == dynamic_cast<CWindow*>(pElement)->GetName() )
			break;
	}
	return after;
}

CWindowScrollableContainerBase::CElements::iterator CWindowScrollableContainerBase::GetBefore( CWindowScrollableContainerBase::CElements::iterator after )
{
	CElements::iterator before = after;
	if ( before != elements.end() )
		++before;																// list insert before position, so we need ++after
	return before;
}

void CWindowScrollableContainerBase::InsertAfter( IWindow *pElement, IWindow *pInsert, const bool bSelectable )
{
	CElements::iterator after = GetAfter( pElement );
	CElements::iterator before = GetBefore( after );
	CWindow * pNegativeSelection = pShared->pNegativeSelection ? checked_cast<CWindowScrollBar*>( CUIFactory::MakeWindow( pShared->pNegativeSelection ) ) : 0;
	AddElement( dynamic_cast<CWindow*>( pInsert ), bSelectable, 0, pNegativeSelection );
	elements.insert( before, SElement(  dynamic_cast<CWindow*>(pInsert), bSelectable, 0, pNegativeSelection ) );
}

void CWindowScrollableContainerBase::RemoveItems()
{
	for ( CElements::iterator el = elements.begin(); el != elements.end(); )
	{
		pContainer->RemoveChild( el->szName );
		if ( el->szNegativeSelection.empty() )
			pContainer->RemoveChild( el->szNegativeSelection );
		el = elements.erase( el );
	}
	Update();
}

void CWindowScrollableContainerBase::Remove( IWindow * pRemove )
{
	for ( CElements::iterator el = elements.begin(); el != elements.end(); ++el )
	{
		if ( el->szName == dynamic_cast<CWindow*>(pRemove)->GetName() )
		{
			elements.erase( el );
			RemoveElement( dynamic_cast<CWindow*>( pRemove ) );			
			break;
		}
	}
}

bool CWindowScrollableContainerBase::IsHorisontal() const
{
	if ( pScrollBar )
		return pScrollBar->IsHorisontal();
	return false;
}

void CWindowScrollableContainerBase::Update()
{
	if ( !elements.empty() )
	{
		elements.front().nPos = 0;
		UpdateItemCoordinates( elements.begin() );
	}
	else
	{
		nPosSize = 0;
	}
	UpdateScrollBar();
}

void CWindowScrollableContainerBase::UpdateItemCoordinates( CWindowScrollableContainerBase::CElements::iterator _from )
{
	int nPosSoFar = _from->nPos;
	
	CElements::iterator from = _from;
	while( from != elements.end() )
	{
		int nSizeY, nSizeX;
		CWindow *pChild = dynamic_cast<CWindow*>(pContainer->GetChild( from->szName, false ));
		CWindow *pNegativeSelection = dynamic_cast<CWindow*>(pContainer->GetChild( from->szNegativeSelection, false ));
		pChild->GetPlacement( 0, 0, &nSizeX, &nSizeY );
		if ( IsHorisontal() )
		{
			pChild->SetPlacement( nPosSoFar, 0, 0, 0, EWPF_POS_X );
			nPosSoFar += nSizeX;
		}
		else
		{
			pChild->SetPlacement( 0, nPosSoFar, 0, 0, EWPF_POS_Y );
			nPosSoFar += nSizeY;
		}
				
		++from;
		if ( pNegativeSelection )
		{
			UpdateSelectionPosition( pNegativeSelection, pChild);
			pNegativeSelection->ShowWindow( pChild != pSelected );
		}

		if ( from != elements.end() )
			nPosSoFar += pShared->nInterval;
	}
	nPosSize = nPosSoFar;
	UpdateSelectionPosition( pSelection, pSelected );
	UpdateSelectionPosition( pPreSelection, pPreSelected );
}

void CWindowScrollableContainerBase::RemoveElement( CWindow *pElement )
{
	CWindowScrollableContainerBase::CElements::iterator pos = GetAfter( pElement );
	
	const int nID = NStr::ToInt( pElement->GetName() );
	NI_ASSERT( nID != 0, StrFmt("not native element \"%s\"", pElement->GetName() ));
	elementIDs.Return( nID );
	pContainer->RemoveChild( pElement );
	Update();
	UpdateScrollBar();	
	pElement->SetClickNotify( 0 );

	if ( pSelected && pSelected.GetPtr() == pElement )
	{
		pSelected = 0;
		UpdateSelectionPosition( pSelection, pSelected );
		// find next and select
		if ( pos != elements.end() )
		{
			IWindow * pNewSel = pContainer->GetChild( pos->szName, true );
			if ( pNewSel )
				Select( pNewSel );
		}
	}

	if ( pPreSelected )
		if ( pPreSelected.GetPtr() == pElement )
		{
			pPreSelected = 0;
			UpdateSelectionPosition( pPreSelection, pPreSelected );
		}
}

void CWindowScrollableContainerBase::UpdateSelectionPosition( IWindow *_pSelection, IWindow *_pSelected )
{
	if ( !_pSelection ) 
		return;

	if ( !_pSelected )
		_pSelection->ShowWindow( false );
	else
	{
		int nY, nX, nW, nH;
		dynamic_cast<CWindow*>( _pSelected )->GetPlacement( &nX, &nY, 0, &nH );
		pContainer->GetPlacement( 0, 0, &nW, 0 );
		_pSelection->SetPlacement( nX, nY, nW, nH, EWPF_ALL );
		_pSelection->ShowWindow( true );
	}
}

void CWindowScrollableContainerBase::EnsureElementVisible( IWindow *pElement )
{
	int nX, nY;
	CWindow *pEl = dynamic_cast<CWindow*>( pElement );

	pEl->GetPlacement( &nX, &nY, 0, 0 );
	if ( pScrollBar )
	{
		if ( pScrollBar->IsHorisontal() )
			pScrollBar->SetPos( nX );
		else
			pScrollBar->SetPos( nY );
	}
}

void CWindowScrollableContainerBase::AddElement( CWindow *pElement, const bool _bSelectable, const int _nPosSize, CWindow *pNegativeSelection )
{
	pElement->SetName( StrFmt( "%i", elementIDs.Get()) );
	pContainer->AddChild( pElement, false );
	pElement->SetClickNotify( this );
	pElement->ShowWindow( true );
	if ( pNegativeSelection )
	{
		pNegativeSelection->SetName( StrFmt( "%s_sel", pElement->GetName() ) );
		pContainer->AddChild( pNegativeSelection, false );
		UpdateSelectionPosition( pNegativeSelection, pElement );
		pNegativeSelection->ShowWindow( true );
	}
}

void CWindowScrollableContainerBase::PushBack( IWindow *_pElement, const bool bSelectable )
{
	CWindow *pElement = dynamic_cast<CWindow*>( _pElement );

	nPosSize += elements.empty() ? 0 : pShared->nInterval;
	
	CWindow * pNegativeSelection = pShared->pNegativeSelection ? checked_cast<CWindowScrollBar*>( CUIFactory::MakeWindow( pShared->pNegativeSelection ) ) : 0;
	AddElement( pElement, bSelectable, nPosSize, pNegativeSelection );
	elements.push_back( SElement(pElement, bSelectable, nPosSize, pNegativeSelection) );

	int nElementH, nElementW;
	
	const NDb::EPositionAllign eHor = pElement->GetHorAllign();
	const NDb::EPositionAllign eVer = pElement->GetVerAllign();

	if ( IsHorisontal() )
	{
		pElement->SetAllign( NDb::EPA_LOW_END, eVer );
		pElement->SetPlacement( nPosSize, 0, 0, 0, EWPF_POS_X );
	}
	else
	{
		pElement->SetAllign( eHor, NDb::EPA_LOW_END );
		pElement->SetPlacement( 0, nPosSize, 0, 0, EWPF_POS_Y );
	}
	pContainer->RepositionChildren( pElement );
	pElement->GetPlacement( 0, 0, &nElementW, &nElementH );

	if ( IsHorisontal() )
		nPosSize += nElementW;
	else
		nPosSize += nElementH;
	
	UpdateSelectionPosition( pNegativeSelection, pElement );
	UpdateScrollBar();
}

void CWindowScrollableContainerBase::UpdateScrollBar()
{
	int nBorderH, nBorderW;
	pBorder->GetPlacement( 0, 0, &nBorderW, &nBorderH );
	if ( IsHorisontal() )
	{
		pContainer->SetPlacement( 0, 0, nPosSize, 0, EWPF_SIZE_X );
		if ( pScrollBar )
			pScrollBar->SetRange( 0, nPosSize, nBorderW );

	}
	else
	{
		pContainer->SetPlacement( 0, 0, 0, nPosSize, EWPF_SIZE_Y );
		if ( pScrollBar )
			pScrollBar->SetRange( 0, nPosSize, nBorderH );
	}
	RepositionChildren();
}

void CWindowScrollableContainerBase::SliderPosition( const float fPosition, class CWindow *pWho )
{
	if ( IsHorisontal() )
		pContainer->SetPlacement(  -fPosition, 0, 0, 0, EWPF_POS_X );
	else
		pContainer->SetPlacement(  0, -fPosition, 0, 0, EWPF_POS_Y );
}

bool CWindowScrollableContainerBase::OnMouseMove( const CVec2 &vPos, const int nButton )
{
	if ( pScrollBar )
		pScrollBar->AllowMouseScrolling( IsInside( vPos ) );
	
	if ( nButton == 0 )
		PreSelect( pContainer->Pick( vPos, false ) );

	//pPreSelection
	return CWindow::OnMouseMove( vPos, nButton );
}

void CWindowScrollableContainerBase::Init()
{
	elements.clear();
	CWindow::Init();
	for ( CElements::iterator it = elements.begin(); it != elements.end(); ++it )
		(dynamic_cast<CWindow*>(pContainer->GetChild( it->szName, false )))->SetClickNotify( this );
	UpdateSelectionPosition( pPreSelection, pPreSelected );
	UpdateSelectionPosition( pSelection, pSelected );
}

CWindow* CWindowScrollableContainerBase::GetElement( const string &szName )
{
	return dynamic_cast<CWindow*>(pContainer->GetChild( szName, false ));
}

void CWindowScrollableContainerBase::SelectWithSelection( IWindow *pElement, IWindow *_pSelection )
{
	if ( !_pSelection ) 
		return;
	if ( !pElement )			//0 as parameter, unselect
	{
		UpdateSelectionPosition( _pSelection, pElement );
		return;
	}
	for ( CElements::iterator it = elements.begin(); it != elements.end(); ++it )
	{
		if ( it->szName == dynamic_cast<CWindow*>( pElement )->GetName() )
		{
			if ( it->bSelectable )
				UpdateSelectionPosition( _pSelection, pElement );
			break;
		}
	}
}

void CWindowScrollableContainerBase::PreSelect( IWindow *pElement )
{
	pPreSelected = pElement;
	SelectWithSelection( pElement, pPreSelection );
}

void CWindowScrollableContainerBase::Select( IWindow *pElement )
{
	// move negative selection window from former selected to new selected
	if ( pShared->pNegativeSelection )
	{
		if ( pSelected )
		{
			for ( CElements::iterator it = elements.begin(); it != elements.end(); ++it )
			{
				if ( pSelected->GetName() == it->szName )
				{
					IWindow * pNS = pContainer->GetChild( it->szNegativeSelection, false );
					if ( pNS )
					{
						pNS->ShowWindow( true );
						UpdateSelectionPosition( pNS, pSelected );
					}
					break;
				}
			}
		}
		if ( pElement )
		{
			for ( CElements::iterator it = elements.begin(); it != elements.end(); ++it )
			{
				if ( pElement->GetName() == it->szName )
				{
					IWindow * pNS = pContainer->GetChild( it->szNegativeSelection, false );
					if ( pNS )
						pNS->ShowWindow( false );
					break;
				}
			}
		}
	}
	pSelected = pElement;
	SelectWithSelection( pElement, pSelection );
	if ( CUIFactory::GetConsts() )
		GetScreen()->RunAnimationSequienceForward( CUIFactory::GetConsts()->buttonClickSound, this );
	//GetScreen()->RunAnimationSequienceForward( checked_cast<NDb::SWindowScrollableContainerBase*>(GetInstance())->onSelection, this );
}

void CWindowScrollableContainerBase::DoubleClicked( interface IWindow *pWho, int nButton )
{
	GetScreen()->RunAnimationSequienceForward( checked_cast<NDb::SWindowScrollableContainerBase*>(GetInstance())->onDoubleClick, this );
}

void CWindowScrollableContainerBase::Clicked( interface IWindow *pWho, int nButton )
{
	if ( (MSTATE_BUTTON1 & nButton) )
		Select( pWho );
}

IWindow * CWindowScrollableContainerBase::GetItem( const string &szName ) 
{ 
	return pContainer->GetChild( szName, false ); 
}

int CWindowScrollableContainerBase::GetItemNumber( IWindow *pElement )
{
	CWindow *pWnd = dynamic_cast<CWindow*>( pElement );
	if( pWnd->GetParent() == pContainer )
		return NStr::ToInt( pWnd->GetName() ) - 1;
	return -1;
}

IWindow *CWindowScrollableContainerBase::GetItem( const int nItem )
{
	if ( elements.size() <= nItem ) 
		return 0;
	return GetItem( elements[nItem].szName );
}

void CWindowScrollableContainerBase::Resort( IWindowSorter *pSorter )
{
	SSortPred pr( pSorter, pContainer );
	sort( elements.begin(), elements.end(), pr );
	Update();
}

int CWindowScrollableContainerBase::GetBaseHeight() const
{
	int nHeight;
	GetPlacement( 0, 0, 0, &nHeight );
	int nBorderH = 0;
	if ( pBorder )
		pBorder->GetPlacement( 0, 0, 0, &nBorderH );
	return nHeight - nBorderH;
}

void CWindowScrollableContainerBase::ResetScroller()
{
	if ( pScrollBar )
		pScrollBar->SetPos( 0 );
}

void CWindowScrollableContainerBase::SetDiscreteScroll( int nVisibleSlots )
{
	if ( pScrollBar )
		pScrollBar->SetNSpecialPositions( nVisibleSlots == -1 ? 0 : elements.size() - (nVisibleSlots - 1) );

	UpdateScrollBar();
}

