#include "StdAfx.h"
#include "WinFrame2Input.h"
#include "../Input/GameMessage.h"


/**
void CWinToInputMessageConverter::ParseChars()
{
	if ( szCharBuffer.empty() )
		return;
	wstring szRes;
	NStr::ToUnicode( &szRes, szCharBuffer );
	for ( int k = 0; k < szRes.size(); ++k )
		NInput::PostEvent( "win_char", szRes[k], 0 );//AddWinMessage( NInput::CT_WIN_CHAR, szRes[k] );
	szCharBuffer = "";
}
/**/

void CWinToInputMessageConverter::Do()
{
	NWinFrame::SWindowsMsg wMsg;
	while ( NWinFrame::GetMessage( &wMsg ) )
	{
		string szGameMessage;
		int nParam1 = 0;
		int nParam2 = 0;
		int nCount = 0;
		NInput::EControlType eControlType = NInput::CT_UNKNOWN;
		if ( NInput::ConvertMessage( wMsg, &szGameMessage, &nParam1, &nParam2, &nCount, &eControlType ) )
		{
			for ( int nIndex = 0; nIndex < nCount; ++nIndex )
			{
				switch( eControlType )
				{
					case NInput::CT_WINDOWS:
					 NInput::PostWinEvent( szGameMessage, nParam1, nParam2 );
					 break;
					default:
					 NInput::PostEvent( szGameMessage, nParam1, nParam2 );
					 break;
				}
			}
		}
	}
/**
	NWinFrame::SWindowsMsg wMsg;
	while ( NWinFrame::GetMessage( &wMsg ) )
	{
		if ( wMsg.msg == NWinFrame::SWindowsMsg::CHAR )
		{
			for ( int k = 0; k < wMsg.nRep; ++k )
				szCharBuffer += (char)wMsg.nKey;
		}
		else if ( wMsg.msg == NWinFrame::SWindowsMsg::KEY_DOWN )
		{
			ParseChars();
			for ( int k = 0; k < wMsg.nRep; ++k )
				NInput::PostEvent( "win_key", wMsg.nKey, 0 );//NInput::AddWinMessage( NInput::CT_WIN_KEY, wMsg.nKey );
		}
		const char *pszWinMsg = 0;
		switch ( wMsg.msg )
		{
		//case NWinFrame::SWindowsMsg::MOUSE_WHEEL: uMsg = WM_MOUSEWHEEL; break;
		case NWinFrame::SWindowsMsg::MOUSE_MOVE: 
			pszWinMsg = "win_mouse_move"; 
			break;
		case NWinFrame::SWindowsMsg::RB_DBLCLK: 
			pszWinMsg = "win_right_button_dblclk"; 
			break;
		case NWinFrame::SWindowsMsg::LB_DBLCLK: 
			pszWinMsg = "win_left_button_dblclk"; 
			break;
		case NWinFrame::SWindowsMsg::RB_DOWN: 
			pszWinMsg = "win_right_button_down"; 
			break;
		case NWinFrame::SWindowsMsg::LB_DOWN: 
			pszWinMsg = "win_left_button_down"; 
			break;
		case NWinFrame::SWindowsMsg::RB_UP: 
			pszWinMsg = "win_right_button_up"; 
			break;
		case NWinFrame::SWindowsMsg::LB_UP: 
			pszWinMsg = "win_left_button_up"; 
			break;
		//case NWinFrame::SWindowsMsg::KEY_DOWN: uMsg = WM_KEYDOWN; break;
		//case NWinFrame::SWindowsMsg::KEY_UP: uMsg = WM_KEYUP; break;
		//case NWinFrame::SWindowsMsg::CHAR: uMsg = WM_CHAR; break;
		}
		if ( pszWinMsg == 0 )
			continue;
		int nParam1 = 0, nParam2 = 0;
		switch ( wMsg.msg )
		{
			case NWinFrame::SWindowsMsg::MOUSE_MOVE:
			case NWinFrame::SWindowsMsg::LB_DBLCLK: 
			case NWinFrame::SWindowsMsg::RB_DBLCLK:
			case NWinFrame::SWindowsMsg::RB_DOWN: 
			case NWinFrame::SWindowsMsg::LB_DOWN: 
			case NWinFrame::SWindowsMsg::RB_UP: 
			case NWinFrame::SWindowsMsg::LB_UP: 
				nParam1 = PackCoords( CVec2( wMsg.x, wMsg.y) );
				nParam2 = wMsg.dwFlags;
				break;
			//case NWinFrame::SWindowsMsg::KEY_DOWN:
			//case NWinFrame::SWindowsMsg::KEY_UP:
			//case NWinFrame::SWindowsMsg::CHAR:
			//	lParam = wMsg.y + (wMsg.dwFlags << 16);
			//	wParam = wMsg.x;
			//	break;
			default:
				ASSERT(0);
				break;
		}
		NInput::PostWinEvent( pszWinMsg, nParam1, nParam2 );
		//Singleton<IInput>()->AcceptWindowsInput( uMsg, wParam, lParam );
	}
	ParseChars();
/**/
}

