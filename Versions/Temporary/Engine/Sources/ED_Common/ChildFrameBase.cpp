#include "stdafx.h"
#include "MapEditorLib/MfcWidget.h"

#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/ResourceDefines.h"
#include "System/Dg.h"
#include "System/GResource.h"

#include "ChildFrameBase.h"
#include "ChildFrameWndBase.h"
#include "SceneSurface.h"

CChildFrameBase::CChildFrameBase() : pSurface( 0 ), pwndChildFrame( 0 ), pChildWnd( 0 )
{
}


CChildFrameBase::~CChildFrameBase()
{
	Destroy();
}


void CChildFrameBase::DeleteSurface()
{
	// Only once its window is gone, which destroying the document window does
	// at once: the window's destruction is what tells the viewport to let go.
	delete pSurface;
	pSurface = 0;
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
	DeleteSurface();
	//
	if ( IMainFrame *pMainFrame = Singleton<IMainFrameContainer>()->Get() )
	{
		if ( pwndChildFrame = pMainFrame->CreateChildFrame( IDR_CHILD_FRAME_0 ) )
		{
			pwndChildFrame->Show( false );
			// Which toolkit draws the viewport's window is NSceneSurface's
			// business; the viewport itself is the same either way.
			pSurface = NSceneSurface::Create();
			if ( pSurface->Create( pwndChildFrame, pChildWnd ) )
			{
				pMainFrame->SetChildFrameWindowContents( pwndChildFrame, pSurface->GetWidget() );
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
	DeleteSurface();
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


