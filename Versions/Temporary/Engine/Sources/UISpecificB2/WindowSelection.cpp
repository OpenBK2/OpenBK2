#include "stdafx.h"
#include "WindowSelection.h"

REGISTER_SAVELOAD_CLASS(UISPECIFICB2, 0x110BD481, CWindowSelection);

void CWindowSelection::Visit( struct IUIVisitor *pVisitor )
{
	CWindow::Visit( pVisitor );
	if ( pSelectorRect != 0 && bSelectorVisible )
		pSelectorRect->Visit( pVisitor );
}

bool CWindowSelection::MsgStartSelection( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	vSelectionFirstPoint = vPos;
	return false;
}

bool CWindowSelection::MsgEndSelection( const SGameMessage &msg )
{
	bSelectorVisible = false;
	return false;
}

bool CWindowSelection::MsgCancelSelection( const SGameMessage &msg )
{
	bSelectorVisible = false;
	return false;
}

void CWindowSelection::RegisterObservers()
{
	AddObserver( "start_selection", &CWindowSelection::MsgStartSelection );
	AddObserver( "update_selection", &CWindowSelection::MsgUpdateSelection );
	AddObserver( "end_selection", &CWindowSelection::MsgEndSelection );
	AddObserver( "cancel_selection", &CWindowSelection::MsgCancelSelection );
}

bool CWindowSelection::MsgUpdateSelection( const SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	const CVec2 vPos = UnPackCoords( msg.nParam1 );
	CVec2 vLeftTop, vRightBottom;
	if ( vSelectionFirstPoint.x < vPos.x )
	{
		vLeftTop.x = vSelectionFirstPoint.x;
		vRightBottom.x = vPos.x;
	}
	else
	{
		vLeftTop.x = vPos.x;
		vRightBottom.x = vSelectionFirstPoint.x;
	}
	if ( vSelectionFirstPoint.y < vPos.y )
	{
		vLeftTop.y = vSelectionFirstPoint.y;
		vRightBottom.y = vPos.y;
	}
	else
	{
		vLeftTop.y = vPos.y;;
		vRightBottom.y = vSelectionFirstPoint.y;
	}
	ScreenToVirtual( vLeftTop, &vLeftTop );
	ScreenToVirtual( vRightBottom, &vRightBottom );
	if ( 0 != pSelectorRect.GetPtr() )
		pSelectorRect->SetPos( vLeftTop, vRightBottom-vLeftTop );
	bSelectorVisible = true;
	return false;
}

int CWindowSelection::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &pInstance );
	saver.Add( 3, &bSelectorVisible );
	saver.Add( 4, &vSelectionFirstPoint );
	saver.Add( 5, &pSelectorRect );
	return 0;
}

void CWindowSelection::InitByDesc( const struct NDb::SUIDesc *pDesc )
{
	pInstance = checked_cast<const NDb::SWindowSelection*>( pDesc )->Duplicate();
	CWindow::InitByDesc( pDesc );
	pSelectorRect = checked_cast<CBackground*>( CUIFactory::MakeWindowPart( pInstance->pSelectorTexture ) );
	pSelectorRect->Init();
	bSelectorVisible = false;
	vSelectionFirstPoint = VNULL2;
}


