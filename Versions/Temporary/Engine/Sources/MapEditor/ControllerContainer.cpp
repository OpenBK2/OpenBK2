#include "StdAfx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"

#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_Progress.h"
#include "ControllerContainer.h"

CControllerContainer::CControllerContainer()
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_CONTROLLER_CONTAINER, this );
}


CControllerContainer::~CControllerContainer()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_CONTROLLER_CONTAINER );
}


void CControllerContainer::Add( IController *pOperation )
{
	if ( !pOperation->IsEmpty() )
	{
		if ( pOperation->IsAbsolute() )
		{
			Clear();
		}
		else
		{
			controllerList.push_back( pOperation );
			redoOperationList.clear();
			if ( controllerList.size() > UNDO_BUFFER_SIZE )
			{
				controllerList.pop_front();
			}
		}
	}
}


void CControllerContainer::Clear()
{
	controllerList.clear();
	redoOperationList.clear();
}


bool CControllerContainer::CanUndo() const
{
	return !( controllerList.empty() );
}


bool CControllerContainer::CanRedo() const
{
	return !( redoOperationList.empty() );
}


bool CControllerContainer::Undo( int nCount )
{
	if ( nCount >= 0 )
	{
		NProgress::Create( true );
		CString strPM;
		strPM.LoadString( IDS_PM_UNDO );
		NProgress::SetMessage( string( strPM ) );
		NProgress::SetRange( 0, nCount + 1 );
		//
		for ( int nIndex = 0; nIndex <= nCount; ++nIndex )
		{
			if ( controllerList.empty() )
			{
				break;
			}
			CPtr<IController> pOperation = controllerList.back();
			if ( !pOperation->Undo( true, true, 0 ) )
			{
				return false;
			}
			controllerList.pop_back();
			redoOperationList.push_back( pOperation );
			NProgress::IteratePosition();
		}
		//
		NProgress::Destroy();
	}
	return true;
}


bool CControllerContainer::Redo( int nCount )
{
	if ( nCount >= 0 )
	{
		NProgress::Create( true );
		CString strPM;
		strPM.LoadString( IDS_PM_REDO );
		NProgress::SetMessage( string( strPM ) );
		NProgress::SetRange( 0, nCount + 1 );
		//
		for ( int nIndex = 0; nIndex <= nCount; ++nIndex )
		{
			if ( redoOperationList.empty() )
			{
				break;
			}
			CPtr<IController> pOperation = redoOperationList.back();
			if ( !pOperation->Redo( true, true, 0 ) )
			{
				return false;
			}
			redoOperationList.pop_back();
			controllerList.push_back( pOperation );
			NProgress::IteratePosition();
		}
		//
		NProgress::Destroy();
	}
	return true;
}


bool CControllerContainer::UndoArrow()
{
	SECWorkbook *pMainFrame = Singleton<IMainFrameContainer>()->GetSECWorkbook();
	CPoint mouseCursorPos;
	GetCursorPos( &mouseCursorPos );

	CTPoint<int> leftBottomPos;
	if ( Singleton<IMainFrameContainer>()->Get()->GetToolBarButtonLeftBottomPos( CTPoint<int>( mouseCursorPos.x, mouseCursorPos.y ), ID_CC_UNDO, &leftBottomPos ) )
	{
		CDescriptionList undoDescriptionList;
		GetDescriptionList( &undoDescriptionList, true );
		//
		if ( !::IsWindow( wndMDDLDialog.m_hWnd ) )
		{
			wndMDDLDialog.Create( CMDDLDialog::IDD, pMainFrame );
		}
		CRect dialogRect;
		wndMDDLDialog.SetParams( ID_CC_UNDO, undoDescriptionList );
		wndMDDLDialog.GetWindowRect( &dialogRect );
		wndMDDLDialog.MoveWindow( leftBottomPos.x, leftBottomPos.y, dialogRect.Width(), dialogRect.Height(), true );
		wndMDDLDialog.ShowWindow( SW_SHOW );
	}
	return true;
}


bool CControllerContainer::RedoArrow()
{
	SECWorkbook *pMainFrame = Singleton<IMainFrameContainer>()->GetSECWorkbook();
	CPoint mouseCursorPos;
	GetCursorPos( &mouseCursorPos );

	CTPoint<int> leftBottomPos;
	if ( Singleton<IMainFrameContainer>()->Get()->GetToolBarButtonLeftBottomPos( CTPoint<int>( mouseCursorPos.x, mouseCursorPos.y ), ID_CC_REDO, &leftBottomPos ) )
	{
		CDescriptionList redoDescriptionList;
		GetDescriptionList( &redoDescriptionList, false );

		if ( !::IsWindow( wndMDDLDialog.m_hWnd ) )
		{
			wndMDDLDialog.Create( CMDDLDialog::IDD, pMainFrame );
		}
		CRect dialogRect;
		wndMDDLDialog.SetParams( ID_CC_REDO, redoDescriptionList );
		wndMDDLDialog.GetWindowRect( &dialogRect );
		wndMDDLDialog.MoveWindow( leftBottomPos.x, leftBottomPos.y, dialogRect.Width(), dialogRect.Height(), true );
		wndMDDLDialog.ShowWindow( SW_SHOW );
	}
	return true;
}


int CControllerContainer::GetDescriptionList( CDescriptionList *pDescriptionList, bool bUndoList ) const
{
	int nCount = 0;
	CString strDescription;
	if ( bUndoList )
	{
		for ( CControllerList::const_iterator itController = controllerList.begin(); itController != controllerList.end(); ++itController )
		{
			if ( pDescriptionList )
			{
				strDescription.Empty();
				( *itController )->GetDescription( &strDescription );
				pDescriptionList->push_front( strDescription );
			}
			++nCount;
		}
	}
	else
	{
		for ( CControllerList::const_iterator itRedoOperation = redoOperationList.begin(); itRedoOperation != redoOperationList.end(); ++itRedoOperation )
		{
			if ( pDescriptionList )
			{
				strDescription.Empty();
				( *itRedoOperation )->GetDescription( &strDescription );
				pDescriptionList->push_front( strDescription );
			}
			++nCount;
		}
	}
	return nCount;
}


int CControllerContainer::RemoveTemporaryControllers( const string &rszTemporaryLabel )
{
	int nCount = 0;
	string szTemporaryLabel;
	// Undo list
	{
		for ( CControllerList::iterator itController = controllerList.begin(); itController != controllerList.end(); )
		{
			szTemporaryLabel.clear();
			( *itController )->GetTemporaryLabel( &szTemporaryLabel );
			if ( szTemporaryLabel == rszTemporaryLabel )
			{
				itController = controllerList.erase( itController );
				++nCount;
			}
			else
			{
				++itController;
			}
		}
	}
	// Redo list
	{
		for ( CControllerList::iterator itRedoOperation = redoOperationList.begin(); itRedoOperation != redoOperationList.end(); )
		{
			szTemporaryLabel.clear();
			( *itRedoOperation )->GetTemporaryLabel( &szTemporaryLabel );
			if ( szTemporaryLabel == rszTemporaryLabel )
			{
				itRedoOperation = redoOperationList.erase( itRedoOperation );
				++nCount;
			}
			else
			{
				++itRedoOperation;
			}
		}
	}
	return nCount;
}


bool CControllerContainer::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_CC_UNDO:
			return Undo( dwData );
		case ID_CC_UNDO_ARROW:
			return UndoArrow();
		case ID_CC_REDO:
			return Redo( dwData );
		case ID_CC_REDO_ARROW:
			return RedoArrow();
		default:
			return false;
	}
}


bool CControllerContainer::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CControllerContainer::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CControllerContainer::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_CC_UNDO:
		case ID_CC_UNDO_ARROW:
			( *pbEnable ) = CanUndo();
			( *pbCheck ) = false;
			return true;
		case ID_CC_REDO:
		case ID_CC_REDO_ARROW:
			( *pbEnable ) = CanRedo();
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
}


// basement storage  


