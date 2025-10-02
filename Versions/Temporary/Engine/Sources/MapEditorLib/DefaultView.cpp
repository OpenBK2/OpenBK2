#include "StdAfx.h"

#include "DefaultView.h"
#include "Interface_FolderCallback.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CDefaultView::SetViewManipulator( IManipulator* _pViewManipulator,
																			 const SObjectSet &rObjectSet,
																			 const string &rszTemporaryLabel )
{
	//DebugTrace( "CDefaultView::SetViewManipulator:<%s>:%d, UOLabel:<%s>, controller:%d", rObjectSet.szObjectTypeName.c_str(), rObjectSet.objectNameSet.size(), rszTemporaryLabel.c_str(), bController );
	Singleton<IViewContainer>()->Remove( this, objectSet );
	//
	pViewManipulator = _pViewManipulator;
	objectSet = rObjectSet;
	szTemporaryLabel = rszTemporaryLabel;
	//
	Singleton<IViewContainer>()->Add( this, objectSet );
}


void CDefaultView::RemoveViewManipulator()
{
	//DebugTrace( "CDefaultView::RemoveViewManipulator:<%s>:%d, UOLabel:<%s>", objectSet.szObjectTypeName.c_str(), objectSet.objectNameSet.size(), szTemporaryLabel.c_str() );
	if ( !szTemporaryLabel.empty() )
	{
		Singleton<IControllerContainer>()->RemoveTemporaryControllers( szTemporaryLabel );
	}
	//
	Singleton<IViewContainer>()->Remove( this, objectSet );
	//
	pViewManipulator = 0;
	objectSet.Clear();
	szTemporaryLabel.clear();
}


void CDefaultView::Enter()
{
	Singleton<IFolderCallback>()->LockObjects( objectSet );
}


void CDefaultView::Leave()
{
	Singleton<IFolderCallback>()->UnockObjects( objectSet );
}


// basement storage  

