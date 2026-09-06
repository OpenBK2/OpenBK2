#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "Scintilla/Scintilla.h"
#include "ResourceDefines.h"

#include "MapEditorLib/MfcWidget.h"

#include "DW_Log.h"

#include <cstdint>

CDWLog::CDWLog()
	: pLogView( 0 )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_LOG, this );
}


CDWLog::~CDWLog()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_LOG );
	delete pLogView;
	pLogView = 0;
}


BEGIN_MESSAGE_MAP(CDWLog, SECControlBar)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


int CDWLog::OnCreate( LPCREATESTRUCT pCreateStruct ) 
{
	if ( SECControlBar::OnCreate( pCreateStruct ) == -1 )
	{
		return -1;
	}
	//
	pLogView = NLogView::Create();
	// This pane is what the contents nominate as the selection handler when
	// they take focus, which is why copy, clear and select-all are on CDWLog
	// now rather than duplicated in each kind of contents.
	CWndWidget paneWidget( this );
	if ( pLogView == 0 || !pLogView->Create( &paneWidget, this ) )
	{
		return -1;
	}
	pLogView->Show( true );
	return 0;
}


void CDWLog::OnSize( unsigned nType, int cx, int cy ) 
{
	SECControlBar::OnSize( nType, cx, cy );
	
	if ( pLogView != 0 && pLogView->IsCreated() )
	{
		CRect insideRect;
		GetInsideRect( insideRect );
		pLogView->SetBounds( CTRect<int>( insideRect.left, insideRect.top,
																	insideRect.right, insideRect.bottom ) );
	}
}


void CDWLog::Log( ELogOutputType eLogOutputType, const std::string &szText )
{
	if ( !szText.empty() )
	{
		TRACE2( "%d: %s", eLogOutputType, szText );
		//
		NLog::CLogBufferList::iterator posLogBuffer = logBufferList.insert( logBufferList.end(), NLog::SLogBuffer() );
		posLogBuffer->eLogOutputType = eLogOutputType;
		posLogBuffer->szText = szText;
		//
		Append( *posLogBuffer );
		//
		if ( logBufferList.size() > 1024 )
		{
			logBufferList.pop_front();
		}
		//
		pLogView->Redraw();
	}
}


void CDWLog::ClearLog()
{
	logBufferList.clear();
	pLogView->Clear();
	pLogView->Redraw();
}


void CDWLog::UpdateLog()
{
	if ( ( Singleton<IUserDataContainer>() != 0 ) &&
			 ( Singleton<IUserDataContainer>()->Get() != 0 ) )
	{
		pLogView->Clear();
		for ( NLog::CLogBufferList::iterator itLogBuffer = logBufferList.begin(); itLogBuffer != logBufferList.end(); ++itLogBuffer )
		{
			Append( *itLogBuffer );
		}
		pLogView->Redraw();
	}
}


void CDWLog::Append( const NLog::SLogBuffer &rLogBuffer )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	if ( ( ( rLogBuffer.eLogOutputType == LT_NORMAL ) && ( pUserData->bShowLogMessages ) ) ||
			 ( ( rLogBuffer.eLogOutputType == LT_IMPORTANT ) && ( pUserData->bShowLogWarnings ) ) ||
			 ( ( rLogBuffer.eLogOutputType == LT_ERROR ) && ( pUserData->bShowLogErrors ) ) )
	{
		pLogView->Append( rLogBuffer.eLogOutputType, rLogBuffer.szText );
	}
}


bool CDWLog::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	if ( ( Singleton<IUserDataContainer>() == 0 ) ||
			 ( Singleton<IUserDataContainer>()->Get() == 0 ) )
	{
		return false;
	}
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	switch( nCommandID )
	{
		case ID_LOG_SHOW_MESSAGES:
			pUserData->bShowLogMessages = !pUserData->bShowLogMessages;
			UpdateLog();
			return true;
		case ID_LOG_SHOW_WARNINGS:
			pUserData->bShowLogWarnings = !pUserData->bShowLogWarnings;
			UpdateLog();
			return true;
		case ID_LOG_SHOW_ERRORS:
			pUserData->bShowLogErrors = !pUserData->bShowLogErrors;
			UpdateLog();
			return true;
		case ID_LOG_CLEAR_ALL:
			ClearLog();
			return true;
		// Registered as CHID_SELECTION by whichever contents have focus. These
		// were on CLogWindow; they are here so they are written once rather
		// than once per kind of contents.
		case ID_SELECTION_COPY:
			pLogView->Copy();
			return true;
		case ID_SELECTION_CLEAR:
			ClearLog();
			return true;
		case ID_SELECTION_SELECT_ALL:
			pLogView->SelectAll();
			return true;
		default:
			return false;
	}
}


bool CDWLog::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CDWLog::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CDWLog::UpdateCommand(), pbCheck == 0" );
	//
	if ( ( Singleton<IUserDataContainer>() == 0 ) ||
			 ( Singleton<IUserDataContainer>()->Get() == 0 ) )
	{
		return false;
	}
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	switch( nCommandID )
	{
		case ID_LOG_SHOW_MESSAGES:
			( *pbEnable ) = true;
			( *pbCheck ) = pUserData->bShowLogMessages;
			return true;
		case ID_LOG_SHOW_WARNINGS:
			( *pbEnable ) = true;
			( *pbCheck ) = pUserData->bShowLogWarnings;
			return true;
		case ID_LOG_SHOW_ERRORS:
			( *pbEnable ) = true;
			( *pbCheck ) = pUserData->bShowLogErrors;
			return true;
		case ID_LOG_CLEAR_ALL:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_COPY:
			( *pbEnable ) = pLogView->HasSelection();
			( *pbCheck ) = false;
			return true;
		case ID_SELECTION_CLEAR:
		case ID_SELECTION_SELECT_ALL:
			( *pbEnable ) = !pLogView->IsEmpty();
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
}


// basement storage  



