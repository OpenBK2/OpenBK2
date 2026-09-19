#include "stdafx.h"
#include "MapEditorLib/Resources.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"

#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_Progress.h"
#include "ControllerContainer.h"
#include "MenuDropDownView.h"

#include <cstdint>

CControllerContainer::CControllerContainer() : pMenuDropDown( 0 )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_CONTROLLER_CONTAINER, this );
}


CControllerContainer::~CControllerContainer()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_CONTROLLER_CONTAINER );
	delete pMenuDropDown;
	pMenuDropDown = 0;
}


CControllerContainer::CEditSession::CEditSession( CControllerContainer *_pContainer )
	: pContainer( _pContainer )
{
	pContainer->BeginEditSession();
}


CControllerContainer::CEditSession::~CEditSession()
{
	if ( pContainer != nullptr && !Finish( false ) )
	{
		// Never leave the surrounding editor using an abandoned modal history.
		DebugTrace( "Failed to cancel a property edit session; retaining its remaining changes" );
		Finish( true );
	}
}


bool CControllerContainer::CEditSession::Finish( bool bAccept )
{
	if ( pContainer == nullptr )
	{
		return true;
	}
	if ( !pContainer->EndEditSession( bAccept ) )
	{
		return false;
	}
	pContainer = nullptr;
	return true;
}


void CControllerContainer::BeginEditSession()
{
	editSessions.emplace_back();
	controllerList.swap( editSessions.back().savedUndo );
	redoOperationList.swap( editSessions.back().savedRedo );
}


void CControllerContainer::SetEditOperationApplied( IController *pOperation, bool bApplied )
{
	if ( !editSessions.empty() )
	{
		for ( auto pos = editSessions.back().operations.rbegin(); pos != editSessions.back().operations.rend(); ++pos )
		{
			if ( pos->pController == pOperation )
			{
				pos->bApplied = bApplied;
				return;
			}
		}
	}
}


bool CControllerContainer::EndEditSession( bool bAccept )
{
	NI_VERIFY( !editSessions.empty(), "No property edit session to finish", return false );
	SEditSession &rSession = editSessions.back();
	if ( !bAccept )
	{
		// Reverse only changes still applied, including edits to objects the
		// picker no longer shows. Undo updates the model and all its open views.
		for ( auto pos = rSession.operations.rbegin(); pos != rSession.operations.rend(); ++pos )
		{
			if ( pos->bApplied )
			{
				if ( !pos->pController->Undo( true, true, nullptr ) )
				{
					return false;
				}
				pos->bApplied = false;
				controllerList.remove( pos->pController );
				// A partial rollback also invalidates the local redo branch.
				redoOperationList.clear();
			}
		}
		controllerList.clear();
		redoOperationList.clear();
	}
	else if ( !rSession.operations.empty() )
	{
		// OK keeps the edits undoable and replaces the previous redo branch.
		rSession.savedUndo.splice( rSession.savedUndo.end(), controllerList );
		rSession.savedRedo.clear();
		rSession.savedRedo.swap( redoOperationList );
	}
	controllerList.swap( rSession.savedUndo );
	redoOperationList.swap( rSession.savedRedo );
	std::list<SEditOperation> operations;
	operations.swap( rSession.operations );
	editSessions.pop_back();
	if ( bAccept && !editSessions.empty() )
	{
		// An inner OK belongs to its outer picker until that picker accepts.
		editSessions.back().operations.splice( editSessions.back().operations.end(), operations );
	}
	if ( editSessions.empty() )
	{
		while ( controllerList.size() > UNDO_BUFFER_SIZE )
		{
			controllerList.pop_front();
		}
		while ( redoOperationList.size() > UNDO_BUFFER_SIZE )
		{
			redoOperationList.pop_front();
		}
	}
	return true;
}


void CControllerContainer::Add( IController *pOperation )
{
	if ( !pOperation->IsEmpty() )
	{
		// Irreversible browser actions keep their existing behavior. Property
		// controllers retain the data needed for Cancel during an edit session.
		if ( !editSessions.empty() && !pOperation->IsAbsolute() )
		{
			SEditOperation operation;
			operation.pController = pOperation;
			editSessions.back().operations.push_back( operation );
		}
		if ( pOperation->IsAbsolute() )
		{
			Clear();
		}
		else
		{
			controllerList.push_back( pOperation );
			redoOperationList.clear();
			if ( editSessions.empty() && ( controllerList.size() > UNDO_BUFFER_SIZE ) )
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
	// An irreversible operation also invalidates history saved by a picker.
	// Its reversible property edits are still kept for Cancel.
	for ( SEditSession &rSession : editSessions )
	{
		rSession.savedUndo.clear();
		rSession.savedRedo.clear();
	}
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
		std::string strPM = NResources::GetString( IDS_PM_UNDO );
		NProgress::SetMessage( std::string( strPM ) );
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
			SetEditOperationApplied( pOperation, false );
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
		std::string strPM = NResources::GetString( IDS_PM_REDO );
		NProgress::SetMessage( std::string( strPM ) );
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
			SetEditOperationApplied( pOperation, true );
			NProgress::IteratePosition();
		}
		//
		NProgress::Destroy();
	}
	return true;
}


bool CControllerContainer::UndoArrow()
{
	return ShowOperationList( true );
}


bool CControllerContainer::RedoArrow()
{
	return ShowOperationList( false );
}


bool CControllerContainer::ShowOperationList( bool bUndo )
{
	CPoint mouseCursorPos;
	GetCursorPos( &mouseCursorPos );

	const unsigned nButtonID = bUndo ? ID_CC_UNDO : ID_CC_REDO;
	CTPoint<int> leftBottomPos;
	if ( Singleton<IMainFrameContainer>()->Get()->GetToolBarButtonLeftBottomPos( CTPoint<int>( mouseCursorPos.x, mouseCursorPos.y ), nButtonID, &leftBottomPos ) )
	{
		CDescriptionList descriptionList;
		GetDescriptionList( &descriptionList, bUndo );
		if ( pMenuDropDown == 0 )
		{
			// The frame's own widget, which outlives this container's use of it.
			pMenuDropDown = NMenuDropDown::Create( Singleton<IMainFrameContainer>()->GetMainWindow() );
		}
		if ( pMenuDropDown != 0 )
		{
			pMenuDropDown->Show( leftBottomPos.x, leftBottomPos.y, nButtonID, descriptionList );
		}
	}
	return true;
}


int CControllerContainer::GetDescriptionList( CDescriptionList *pDescriptionList, bool bUndoList ) const
{
	int nCount = 0;
	std::string szDescription;
	if ( bUndoList )
	{
		for ( CControllerList::const_iterator itController = controllerList.begin(); itController != controllerList.end(); ++itController )
		{
			if ( pDescriptionList )
			{
				szDescription.clear();
				( *itController )->GetDescription( &szDescription );
				pDescriptionList->push_front( szDescription );
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
				szDescription.clear();
				( *itRedoOperation )->GetDescription( &szDescription );
				pDescriptionList->push_front( szDescription );
			}
			++nCount;
		}
	}
	return nCount;
}


int CControllerContainer::RemoveTemporaryControllers( const std::string &rszTemporaryLabel )
{
	int nCount = 0;
	std::string szTemporaryLabel;
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
	// A temporary view is releasing its object. Do not retain controllers for
	// it in a modal session after their temporary data has been discarded.
	for ( SEditSession &rSession : editSessions )
	{
		auto matches = [&rszTemporaryLabel]( const CPtr<IController> &pController )
		{
			std::string szLabel;
			pController->GetTemporaryLabel( &szLabel );
			return szLabel == rszTemporaryLabel;
		};
		rSession.savedUndo.remove_if( matches );
		rSession.savedRedo.remove_if( matches );
		rSession.operations.remove_if( [&matches]( const SEditOperation &rOperation ) { return matches( rOperation.pController ); } );
	}
	return nCount;
}


bool CControllerContainer::HandleCommand( unsigned nCommandID, uintptr_t dwData )
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


bool CControllerContainer::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
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


