#include "stdafx.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "ResourceDefines.h"

#include "LogPane.h"

// Moved from DW_Log.cpp unchanged, but that a missing view no longer crashes a
// log call: the buffer is kept either way.

CLogPaneContents::CLogPaneContents()
	: pLogView( 0 )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_LOG, this );
}


CLogPaneContents::~CLogPaneContents()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_LOG );
	delete pLogView;
	pLogView = 0;
}


void CLogPaneContents::SetView( ILogView *pView )
{
	if ( pLogView != pView )
	{
		delete pLogView;
		pLogView = pView;
	}
}


void CLogPaneContents::Log( ELogOutputType eLogOutputType, const std::string &szText )
{
	if ( !szText.empty() )
	{
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
		if ( pLogView != 0 )
		{
			pLogView->Redraw();
		}
	}
}


void CLogPaneContents::ClearLog()
{
	logBufferList.clear();
	if ( pLogView != 0 )
	{
		pLogView->Clear();
		pLogView->Redraw();
	}
}


void CLogPaneContents::UpdateLog()
{
	if ( ( pLogView != 0 ) &&
			 ( Singleton<IUserDataContainer>() != 0 ) &&
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


void CLogPaneContents::Append( const NLog::SLogBuffer &rLogBuffer )
{
	if ( pLogView == 0 )
	{
		return;
	}
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	if ( ( ( rLogBuffer.eLogOutputType == LT_NORMAL ) && ( pUserData->bShowLogMessages ) ) ||
			 ( ( rLogBuffer.eLogOutputType == LT_IMPORTANT ) && ( pUserData->bShowLogWarnings ) ) ||
			 ( ( rLogBuffer.eLogOutputType == LT_ERROR ) && ( pUserData->bShowLogErrors ) ) )
	{
		pLogView->Append( rLogBuffer.eLogOutputType, rLogBuffer.szText );
	}
}


bool CLogPaneContents::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	if ( ( Singleton<IUserDataContainer>() == 0 ) ||
			 ( Singleton<IUserDataContainer>()->Get() == 0 ) ||
			 ( pLogView == 0 ) )
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


bool CLogPaneContents::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CLogPaneContents::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CLogPaneContents::UpdateCommand(), pbCheck == 0" );
	//
	if ( ( Singleton<IUserDataContainer>() == 0 ) ||
			 ( Singleton<IUserDataContainer>()->Get() == 0 ) ||
			 ( pLogView == 0 ) )
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
