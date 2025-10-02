#include "StdAfx.h"

#include "..\MapEditorLib\ChildFrameFactory.h"
#include "ChildFrameContainer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CChildFrameContainer::~CChildFrameContainer()
{
	Destroy();
}


bool CChildFrameContainer::CanCreate( const string &rszChildFrameTypeName )
{
	if ( !rszChildFrameTypeName.empty() )
	{
		return NChildFrameFactory::CanCreateChildFrame( rszChildFrameTypeName );
	}
	return false;
}


bool CChildFrameContainer::IsActive( const string &rszChildFrameTypeName )
{
	if ( !rszChildFrameTypeName.empty() )
	{
		return ( szActiveChildFrameTypeName == rszChildFrameTypeName );
	}
	return false;
}


bool CChildFrameContainer::Create( const string &rszChildFrameTypeName )
{
	Destroy();
	if ( !rszChildFrameTypeName.empty() )
	{
		if ( CanCreate( rszChildFrameTypeName ) )
		{
			DebugTrace( "NChildFrameFactory: Create ChildFrame: <%s>", rszChildFrameTypeName.c_str() );
			pActiveChildFrame = NChildFrameFactory::CreateChildFrame( rszChildFrameTypeName );
			if ( pActiveChildFrame != 0 )
			{
				DebugTrace( "Create ChildFrame: <%s>", rszChildFrameTypeName.c_str() );
				if ( pActiveChildFrame->Create() )
				{
					szActiveChildFrameTypeName = rszChildFrameTypeName;
					DebugTrace( "Enter ChildFrame: <%s>", rszChildFrameTypeName.c_str() );
					pActiveChildFrame->Enter();
					return true;
				}
				else
				{
					pActiveChildFrame = 0;
				}
			}
		}
	}
	return false;
}


void CChildFrameContainer::Destroy()
{
	if ( pActiveChildFrame != 0 )
	{
		DebugTrace( "Leave ChildFrame: <%s>", szActiveChildFrameTypeName.c_str() );
		pActiveChildFrame->Leave();
		DebugTrace( "Destroy ChildFrame: <%s>", szActiveChildFrameTypeName.c_str() );
		pActiveChildFrame->Destroy();
		pActiveChildFrame = 0;
	}
	szActiveChildFrameTypeName.clear();
}


void CChildFrameContainer::Enter()
{
	if ( pActiveChildFrame != 0 )
	{
		DebugTrace( "Enter(*) ChildFrame: <%s>", szActiveChildFrameTypeName.c_str() );
		pActiveChildFrame->Enter();
	}
}


void CChildFrameContainer::Leave()
{
	if ( pActiveChildFrame != 0 )
	{
		DebugTrace( "Leave(*) ChildFrame: <%s>", szActiveChildFrameTypeName.c_str() );
		pActiveChildFrame->Leave();
	}
}

// basement storage  


