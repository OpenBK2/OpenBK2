#include "stdafx.h"

#include "DefaultChildFrame.h"

IMPLEMENT_DYNCREATE(CDefaultChildFrame, SECWorksheet)


BEGIN_MESSAGE_MAP(CDefaultChildFrame, SECWorksheet)
END_MESSAGE_MAP()


BOOL CDefaultChildFrame::PreCreateWindow( CREATESTRUCT &rCreateStruct )
{
	if( !SECWorksheet::PreCreateWindow( rCreateStruct ) )
	{
		return FALSE;
	}

	rCreateStruct.style &= ~WS_VISIBLE;
	rCreateStruct.dwExStyle &= ~WS_EX_CLIENTEDGE;
	rCreateStruct.lpszClass = AfxRegisterWndClass( 0 );

	return TRUE;
}


BOOL CDefaultChildFrame::OnCmdMsg( UINT nID, int nCode, void *pExtra, AFX_CMDHANDLERINFO *pHandlerInfo )
{
	if ( pwndContents )
	{
		if ( pwndContents->OnCmdMsg( nID, nCode, pExtra, pHandlerInfo ) )
		{
			return TRUE;
		}
	}
	//
	return SECWorksheet::OnCmdMsg( nID, nCode, pExtra, pHandlerInfo );
}

// basement storage  


