#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "scintilla/scintilla.h"
#include "ResourceDefines.h"

#include "DW_Log.h"

CDWLog::CDWLog()
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_LOG, this );
}


CDWLog::~CDWLog()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_LOG );
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
	if ( !wndContents.CreateEx( this, WS_EX_CLIENTEDGE, WS_CHILD | WS_VISIBLE, CRect( 0, 0, 0, 0 ), IDC_LOG_WINDOW ) )
	{
		return -1;
	}
	wndContents.Command( SCI_SETREADONLY, false );
	// 
	wndContents.Command( SCI_STYLESETFORE, LT_NORMAL, 0x000000 );
	wndContents.Command( SCI_STYLESETFORE, LT_IMPORTANT, 0x227722 );
	wndContents.Command( SCI_STYLESETFORE, LT_ERROR, 0x3333ff );
	//
	wndContents.ShowWindow( SW_SHOW );
	return 0;
}


void CDWLog::OnSize( UINT nType, int cx, int cy ) 
{
	SECControlBar::OnSize( nType, cx, cy );
	
	if ( wndContents.GetSafeHwnd() != NULL )
	{
		CRect insideRect;
		GetInsideRect( insideRect );

		wndContents.SetWindowPos( 0,
															insideRect.left,
															insideRect.top,
															insideRect.Width(),
															insideRect.Height(),
															SWP_NOZORDER | SWP_NOACTIVATE );
	}
}


void CDWLog::Log( ELogOutputType eLogOutputType, const string &szText )
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
		wndContents.UpdateWindow();
	}
}


void CDWLog::ClearLog()
{
	logBufferList.clear();
	wndContents.Command( SCI_CLEARALL );
	wndContents.UpdateWindow();
}


void CDWLog::UpdateLog()
{
	if ( ( Singleton<IUserDataContainer>() != 0 ) &&
			 ( Singleton<IUserDataContainer>()->Get() != 0 ) )
	{
		wndContents.Command( SCI_CLEARALL );
		for ( NLog::CLogBufferList::iterator itLogBuffer = logBufferList.begin(); itLogBuffer != logBufferList.end(); ++itLogBuffer )
		{
			Append( *itLogBuffer );
		}
		wndContents.UpdateWindow();
	}
}


void CDWLog::Append( const NLog::SLogBuffer &rLogBuffer )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	if ( ( ( rLogBuffer.eLogOutputType == LT_NORMAL ) && ( pUserData->bShowLogMessages ) ) ||
			 ( ( rLogBuffer.eLogOutputType == LT_IMPORTANT ) && ( pUserData->bShowLogWarnings ) ) ||
			 ( ( rLogBuffer.eLogOutputType == LT_ERROR ) && ( pUserData->bShowLogErrors ) ) )
	{
		const int nTextEnd = wndContents.Command( SCI_GETLENGTH );
		const int nPosition = wndContents.Command( SCI_GETCURRENTPOS );
		const int nAnchor = wndContents.Command( SCI_GETANCHOR );
		wndContents.Command( SCI_APPENDTEXT, rLogBuffer.szText.size(), (int)( rLogBuffer.szText.c_str() ) );
		wndContents.Command( SCI_STARTSTYLING, nTextEnd, 0x1f );
		wndContents.Command( SCI_SETSTYLING, rLogBuffer.szText.size(), rLogBuffer.eLogOutputType );
		if ( ( nPosition == nAnchor ) && ( nPosition == nTextEnd ) )
		{
			const int nLenght = wndContents.Command( SCI_GETLENGTH );
			wndContents.Command( SCI_GOTOPOS, nLenght );
		}
	}
}


bool CDWLog::HandleCommand( UINT nCommandID, DWORD dwData )
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
		default:
			return false;
	}
}


bool CDWLog::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
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
		default:
			return false;
	}
}


// basement storage  



