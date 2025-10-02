#include "StdAfx.h"
#include "windowsimple.h"
#include "UIVisitor.h"

REGISTER_SAVELOAD_CLASS(0x110772C1, CWindowSimple)


void CWindowSimple::Visit( struct IUIVisitor *pVisitor )
{
	CTRect<float> rc;
	FillWindowRect( &rc );
	VirtualToScreen( rc, &rc );
	CClipStore s( pVisitor, rc );
	CWindow::Visit( pVisitor );
}

