#include "stdafx.h"
#include "MapEditorLib/MfcWidget.h"

#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/ResourceDefines.h"
#include "System/Dg.h"
#include "System/GResource.h"

#include "ChildFrameBase.h"

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
		pwndChildFrame->Destroy();
		pwndChildFrame = 0;
	}
	//
	if ( IMainFrame *pMainFrame = Singleton<IMainFrameContainer>()->Get() )
	{
		if ( pwndChildFrame = pMainFrame->CreateChildFrame( IDR_CHILD_FRAME_0 ) )
		{
			pwndChildFrame->Show( false );
			if ( pChildWnd->Create( 0,
				0,
				AFX_WS_DEFAULT_VIEW,
				CRect( 0, 0, 0, 0 ),
				ToCWnd( pwndChildFrame ),
				AFX_IDW_PANE_FIRST,
				0 ) )
			{
				CWndWidget contentsWidget( pChildWnd );
				pMainFrame->SetChildFrameWindowContents( pwndChildFrame, &contentsWidget );
				// Maximize drops WS_SYSMENU as well, which is what these two lines did.
				pwndChildFrame->Maximize();
				pwndChildFrame->Show( true );
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
		pwndChildFrame->Destroy();
		pwndChildFrame = 0;
	}
}


void CChildFrameBase::Enter()
{
	DebugTrace( "CChildFrameBase::Enter()" );
	if ( pwndChildFrame )
	{
		pwndChildFrame->Focus();
	}
}


void CChildFrameBase::Leave()
{
	DebugTrace( "CChildFrameBase::Leave()" );
}

// basement storage  


