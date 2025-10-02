#include "stdafx.h"

#include "..\MapEditorLib\Interface_MainFrame.h"
#include "..\MapEditorLib\ResourceDefines.h"
#include "..\System\Dg.h"
#include "..\System\GResource.h"

#include "ChildFrameBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CChildFrameBase::CChildFrameBase() : pwndChildFrame( 0 ), pChildWnd( 0 )
{
}


CChildFrameBase::~CChildFrameBase()
{
	Destroy();
}


bool CChildFrameBase::Create()
{
	DebugTrace( "CChildFrameBase::Create()" );
	NI_ASSERT( pChildWnd, "CChildFrameBase::Create(), pChildWnd should be created" );

	if ( pwndChildFrame )
	{
		pwndChildFrame->MDIDestroy();
		pwndChildFrame = 0;
	}
	//
	if ( IMainFrame *pMainFrame = Singleton<IMainFrameContainer>()->Get() )
	{
		if ( pwndChildFrame = pMainFrame->CreateChildFrame( IDR_CHILD_FRAME_0 ) )
		{
			pwndChildFrame->ShowWindow( SW_HIDE );
			if ( pChildWnd->Create( 0,
				0,
				AFX_WS_DEFAULT_VIEW,
				CRect( 0, 0, 0, 0 ),
				pwndChildFrame,
				AFX_IDW_PANE_FIRST,
				0 ) )
			{
				pMainFrame->SetChildFrameWindowContents( pwndChildFrame, pChildWnd );
				pwndChildFrame->ModifyStyle( WS_SYSMENU, 0 );
				pwndChildFrame->MDIMaximize();
				pwndChildFrame->ShowWindow( SW_SHOW );
				//
			}
		}
		return true;
	}
	return false;
}


void CChildFrameBase::Destroy()
{
	ClearHoldQueue();
	NGScene::CResourceFileOpener::Clear();
	DebugTrace( "CChildFrameBase::Destroy()" );
	if ( pwndChildFrame )
	{
		pwndChildFrame->MDIDestroy();
		pwndChildFrame = 0;
	}
}


void CChildFrameBase::Enter()
{
	DebugTrace( "CChildFrameBase::Enter()" );
	if ( pwndChildFrame )
	{
		pwndChildFrame->SetFocus();
	}
}


void CChildFrameBase::Leave()
{
	DebugTrace( "CChildFrameBase::Leave()" );
}

// basement storage  

