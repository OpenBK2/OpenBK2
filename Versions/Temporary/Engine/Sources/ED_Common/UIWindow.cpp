#include "StdAfx.h"
#include "UIWindow.h"
#include "UIVisitor.hpp"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CUIWindow::CUIWindow() : 
	pos(0, 0, 0, 0), pTexture(0), color(0)
{
}


CUIWindow::CUIWindow( int x, int y, int w, int h, DWORD _color, const NDb::STexture *_pTexture ) :
	pos(x, y, w, h), pTexture(_pTexture), color(_color)
{
}


CUIWindow::~CUIWindow()
{
}


void CUIWindow::Visit( IUIVisitor *pVisitor )
{
	CRectLayout rects;
	rects.AddRect( pos.x1, pos.y1, pos.x2, pos.y2, CTRect<float>(0,0,pos.x2,pos.y2), color );
	Singleton<IUIInitialization>()->GetVirtualScreenController()->VirtualToScreen( &rects );
	pVisitor->VisitUIRect( pTexture, 0, rects );
}


CUIWindow *CreateUIWindow( int x, int y, int w, int h, DWORD _color, const NDb::STexture *_pTexture )
{
	CUIWindow *pUIWindow = new CUIWindow( x, y, w, h, _color, _pTexture );
	return pUIWindow;
}

