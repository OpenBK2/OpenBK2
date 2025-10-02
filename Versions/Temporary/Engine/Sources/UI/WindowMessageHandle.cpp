// WindowMessageHandle.cpp: implementation of the WindowMessageHandle class.
//


#include "stdafx.h"
#include "WindowMessageHandle.h"
#include "Window.h"
#include "UIInternal.h"
//#include "..\Input\Input.h"



// CWindow message handlers


IMPLEMENT_HANDLE_MAP(CWindow)
IMPLEMENT_MESSAGE_HANDLER(CWindow,ShowWindow)
{
	CWindow *pChild = dynamic_cast<CWindow*>(GetChild( msg.szParam, false ));
	if ( pChild )
		pChild->ShowWindow( msg.nParam );
	return pChild;
}

IMPLEMENT_MESSAGE_HANDLER(CWindow,SetFocus)
{
	CWindow *pChild = dynamic_cast<CWindow*>(GetChild( msg.szParam, false ));
	if ( pChild )
	{
		if ( msg.nParam )
			pChild->SetFocus( true );
		else
			SetFocus( false );
	}
	return pChild;
}

IMPLEMENT_MESSAGE_HANDLER(CWindow,Enable)
{
	CWindow *pChild = dynamic_cast<CWindow*>(GetChild( msg.szParam, false ));
	if ( pChild )
		pChild->Enable( msg.nParam );
	return pChild;
}

