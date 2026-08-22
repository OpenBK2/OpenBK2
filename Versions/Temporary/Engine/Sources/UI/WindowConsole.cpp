// WindowConsole.cpp: implementation of the CWindowConsole class.
//

#include "stdafx.h"
#include "WindowConsole.h"
#include "WindowEditLine.h"

#include "UIVisitor.h"

#include "UIML.h"
#include "DBUIConsts.h"
#include "System/Commands.h"

#include <cstdint>

#include <fmt/format.h>

static int nConsoleSize = 100;

void ShowStatsWindow( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() ) 
		return;
	const std::string szVal = NStr::ToMBCS( paramsSet[0] );
	const int nVal = NStr::ToInt( szVal );

	if ( nVal == 1)
		Singleton<IDebugSingleton>()->ShowStatsWindow( true );
	else
		Singleton<IDebugSingleton>()->ShowStatsWindow( false );
}

void ShowDebugInfo( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() ) 
		return;
	const std::string szVal = NStr::ToMBCS( paramsSet[0] );
	const int nVal = NStr::ToInt( szVal );
	if ( nVal == 1 )
		Singleton<IDebugSingleton>()->ShowDebugInfo( true );
	else
		Singleton<IDebugSingleton>()->ShowDebugInfo( false );
}

// Construction/Destruction


static const int CONSOLE_HEIGHT = 240;			//Высота консоли в пикселах
static const int TEXT_LEFT_SPACE = 20;			//Отступ от левого края экрана до текста в консоли
static const int TEXT_VERTICAL_SIZE = 20;		//Размер шрифта по вертикали
static const int MINUS_PAGE_SIZE = 5;				//Специальная константа отступа для PgUp PgDown,
static const int CURSOR_ANIMATION_TIME = 400;		//период переключения курсора
static const wchar_t szPrefix[] = L">>";

CWindowConsole::CWindowConsole() : currTime( 0 ), nBeginCommand( 0 ), nBeginString( 0 ), nConsoleSequenceID(0)
{
	impotantMsgs.AddObserver( "show_console", &CWindowConsole::OnShowConsole );
	impotantMsgs.AddObserver( "win_char", &CWindowConsole::OnChar );
	impotantMsgs.AddObserver( "win_key", &CWindowConsole::OnKeyDown );
	impotantMsgs.AddObserver( "console_first_string", &CWindowConsole::OnCtrlHome );
	impotantMsgs.AddObserver( "console_last_string", &CWindowConsole::OnCtrlEnd );
}

CWindowConsole::SColorString::SColorString( const wchar_t *pszStr, uint32_t col, const int nWidth )
: szString( pszStr ), dwColor( col ) 
{  
	const std::wstring szText = pszStr;
	pGfxText = CreateML();
	CUIFactory::RegisterMLHandlers( pGfxText );
	pGfxText->SetText( szText, 0 );
	pGfxText->Generate( VirtualToScreenX( nWidth ) );
}

void CWindowConsole::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowConsole *pDesc( checked_cast<const NDb::SWindowConsole*>( _pDesc ) );
	pInstance = pDesc->Duplicate();
	CWindow::InitByDesc( _pDesc );

	pShared = checked_cast_ptr<const NDb::SWindowConsoleShared *>( pDesc->pShared );
	pEditLine = dynamic_cast<CWindowEditLine*>( CUIFactory::MakeWindow( pShared->pEditLine ) );
	AddChild( pEditLine, false );
	
	pUpperSign = CreateML();
	CUIFactory::RegisterMLHandlers( pUpperSign );
	pUpperSign->SetText( L"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^", 0 );
}

void CWindowConsole::Init()
{
	CWindow::Init();
	ShowWindow( false );
	GetScreen()->RegisterEffect( "ShowConsole", pShared->makeVisible );
	GetScreen()->RegisterToSegment( this, true );
}

void CWindowConsole::NotifyStateSequenceFinished()
{
}

void CWindowConsole::Reposition( const CTRect<float> &rcParent )
{
	SetPlacement( 0, -CONSOLE_HEIGHT, rcParent.Width(), CONSOLE_HEIGHT, EWPF_ALL );
	if ( pEditLine )
	{
		pEditLine->SetAllign( NDb::EPA_LOW_END, NDb::EPA_HIGH_END );
		pEditLine->SetPlacement( 0, 0, rcParent.Width(), 0, EWPF_SIZE_X );
	}
	pUpperSign->Generate( VirtualToScreenX( rcParent.Width() ) );
	CWindow::Reposition( rcParent );
}

void CWindowConsole::ReadConsoleStrings()
{
	IConsoleBuffer *pBuf = Singleton<IConsoleBuffer>();
	IDebugSingleton *pDebug = Singleton<IDebugSingleton>();

	IConsoleBuffer::SConsoleLine l;
	while ( pBuf->GetNextLine( &l, &nConsoleSequenceID ) )
	{
		if ( l.nStream == CONSOLE_STREAM_CONSOLE )
		{
			if( nBeginString!=0 )
				nBeginString++;
			vectorOfStrings.push_back( SColorString( l.szText.c_str(), l.dwColor, 1024) );
		}
		else if ( l.nStream >= CONSOLE_STREAM_DEBUG_WINDOW && l.nStream < CONSOLE_STREAM_DEBUG_WINDOW + 2 )
		{
			// debug Windows
			IConsoleOutput *pTextView = dynamic_cast<IConsoleOutput*>( pDebug->GetDebugInfoWindow( l.nStream - CONSOLE_STREAM_DEBUG_WINDOW ) );
			if ( pTextView )
				pTextView->AddString( l.szText, l.dwColor );
		}
		else if ( l.nStream >= CONSOLE_STREAM_DEBUG_WINDOW + 2 && l.nStream < CONSOLE_STREAM_DEBUG_WINDOW + 6 )
		{
			// debug strings
			ITextView *pTextView = dynamic_cast<ITextView*>( pDebug->GetDebugInfoWindow( l.nStream - CONSOLE_STREAM_DEBUG_WINDOW ) );
			if ( pTextView )
				pTextView->SetText( l.szText );
		}
	}
}

void CWindowConsole::Segment( const int timeDiff )
{
	// retrieve and parse commands from console buffer
	IConsoleBuffer *pBuffer = Singleton<IConsoleBuffer>();
	ReadConsoleStrings();
	// read commands
	std::wstring szCmd;
	uint32_t color = 0;
	while ( ReadFromPipe( PIPE_CONSOLE_CMDS, &szCmd, &color ) )
	{
		vectorOfCommands.push_back( szCmd );
		if( nBeginString!=0 )
			nBeginString++;
		vectorOfStrings.push_back( SColorString(szCmd.c_str(), color, 1024) );
		ParseCommand( szCmd );

		ReadConsoleStrings();
	}

	if ( vectorOfStrings.size() > nConsoleSize * 1.5 )
	{
		const int nNewSize = (std::max)( nBeginString + 1, nConsoleSize );
		const int nStrToCut = vectorOfStrings.size() - nNewSize;
		if ( nStrToCut > nConsoleSize / 2 )
			vectorOfStrings.erase( vectorOfStrings.begin(), vectorOfStrings.begin() + nStrToCut );
	}
}

void CWindowConsole::Visit( struct IUIVisitor *pVisitor )
{
	CWindow::Visit( pVisitor );
	
	if ( IsVisible() )
	{
		CTRect<float> wndRect;
		FillWindowRect( &wndRect );
		
		int nCurrentY = wndRect.y2 - 2 * TEXT_VERTICAL_SIZE;
		if ( nBeginString != 0 )
		{
			nCurrentY -= TEXT_VERTICAL_SIZE;
			CTPoint<float> vPosition( TEXT_LEFT_SPACE, nCurrentY );
			CTRect<float> tmp;
			VirtualToScreen( wndRect, &tmp );
			VirtualToScreen( vPosition, &vPosition );
			pVisitor->VisitUIText( pUpperSign, vPosition, tmp );
		}
		nCurrentY -= TEXT_VERTICAL_SIZE;

		// отобразим строчки в консоли
		int nSize = vectorOfStrings.size();
		for ( int i = nBeginString; i < nSize; ++i )
		{
			CTPoint<float> vPosition( TEXT_LEFT_SPACE, nCurrentY );
			VirtualToScreen( vPosition, &vPosition );

			CTRect<float> tmp;
			VirtualToScreen( wndRect, &tmp );
			pVisitor->VisitUIText( vectorOfStrings[nSize - i - 1].pGfxText, vPosition, tmp );
			nCurrentY -= TEXT_VERTICAL_SIZE;
			if ( nCurrentY < 0 )
				break;
		}
	}
}

bool CWindowConsole::ProcessEvent( const struct SGameMessage &msg )
{
	if( !IsVisible() )
		return false;

	if( impotantMsgs.ProcessEvent( msg, this ) )
		return true;
	return CWindow::ProcessEvent( msg );
}

bool CWindowConsole::OnKeyDown( const SGameMessage &msg )
{
	if( !IsVisible() )
		return false;
	//Autocomplete instead of tabulate
	if( msg.nParam1==VK_TAB )
	{
		std::wstring wsInput( pEditLine->GetText() );
		if ( !wsInput.empty() )
		{
			std::vector<std::string> varsSet;
			NGlobal::GetIDList( &varsSet );

			std::list<std::wstring> resultList;
			for ( int nTemp = 0; nTemp < varsSet.size(); nTemp++ )
			{
				std::wstring wsTemp = NStr::ToUnicode( varsSet[nTemp] );
				if ( wsTemp.length() < wsInput.length() )
					continue;

				if ( wsInput == wsTemp.substr( 0, wsInput.length() ) )
					resultList.push_back( wsTemp );
			}

			if ( resultList.size() > 1 )
			{
				std::wstring wsSamePart( resultList.front() );

				csSystem << CC_BLUE << L"Commands( " << (int)resultList.size() << L" ):" << endl;
				for ( std::list<std::wstring>::const_iterator iTemp = resultList.begin(); iTemp != resultList.end(); iTemp++ )
				{
					std::wstring wsNewSame( wsSamePart );
					for ( int nTemp = 0; nTemp < (std::min)( iTemp->length(), wsSamePart.length() ); nTemp++ )
					{
						if ( (*iTemp)[nTemp] == wsSamePart[nTemp] )
							continue;

						wsNewSame = wsSamePart.substr( 0, nTemp );
						break;
					}
					wsSamePart = wsNewSame;

					const NGlobal::CValue &sValue = NGlobal::GetVar( NStr::ToMBCS( *iTemp ), 0.0f );
					csSystem << L"\t" << (*iTemp) << " string = '" << sValue.GetString() << "' float = '" << sValue.GetFloat() << "'" << endl;
				}

				pEditLine->SetText( wsSamePart.c_str() );
				//pEditLine->SetCursorPosition( wsSamePart.length() + 1 );
			}
			else if ( resultList.size() == 1 )
			{
				std::wstring wsTemp( resultList.front() + L" " );
				pEditLine->SetText( wsTemp.c_str() );
				//pEditLine->SetCursorPosition( wsTemp.length() + 1 );
			}
		}
		return true;
	}
	
	if( msg.nParam1==VK_PRIOR )
		return OnPgUp( msg );

	if( msg.nParam1==VK_NEXT )
		return OnPgDn( msg );

	if( msg.nParam1==VK_UP )
		return OnUp( msg );

	if( msg.nParam1==VK_DOWN )
		return OnDown( msg );

	return false;
}

bool CWindowConsole::OnChar( const SGameMessage &msg )
{
	if( !IsVisible() )
		return false;

	//Чтобы тильда, с помощью которой открыли консоль, не попадала в строку ввода
	if( msg.nParam1=='`' )
		return true;

	return false;
}

bool CWindowConsole::OnUp( const struct SGameMessage &msg )
{
	if ( !IsVisible() ) 
		return false;
	
	if( vectorOfCommands.empty() )
		return true;

	if( nBeginCommand<0 || nBeginCommand>=vectorOfCommands.size() )
		nBeginCommand = vectorOfCommands.size();
	
	if( nBeginCommand>0 )
		nBeginCommand--;

	pEditLine->SetText( vectorOfCommands[nBeginCommand].c_str() );
	

	/*
	if ( nBeginCommand == vectorOfCommands.size() || nBeginCommand == -1 )
		return true;

	//отобразим предыдущую команду
	nBeginCommand++;
	if ( nBeginCommand == vectorOfCommands.size() )
	{
		nBeginCommand = -1;
		pEditLine->SetText( L"" );
	}
	else
	{
		pEditLine->SetText( vectorOfCommands[vectorOfCommands.size()-nBeginCommand].c_str() );
	}
	*/
	return true;
}

bool CWindowConsole::OnDown( const struct SGameMessage &msg )
{
	if ( !IsVisible() ) 
		return false;

	if( vectorOfCommands.empty() )
		return true;
	
	if( nBeginCommand<0)
		return true;

	if( nBeginCommand<vectorOfCommands.size()-1 )
	{
		nBeginCommand++;
		pEditLine->SetText( vectorOfCommands[nBeginCommand].c_str() );
	}

	/*
	if ( nBeginCommand == -1 && !vectorOfCommands.empty() )
	{
		nBeginCommand = vectorOfCommands.size() - 1;
		pEditLine->SetText( vectorOfCommands[nBeginCommand].c_str() );
	}

	if ( nBeginCommand > 0 )
	{
		//сдвинем позицию на единицу вниз
		nBeginCommand--;
		pEditLine->SetText( vectorOfCommands[vectorOfCommands.size()-nBeginCommand].c_str() );
	}
	*/
	return true;
}

bool CWindowConsole::OnCtrlHome( const struct SGameMessage &msg )
{
	if ( !IsVisible() ) 
		return false;

	if ( vectorOfStrings.size() > CONSOLE_HEIGHT / TEXT_VERTICAL_SIZE )
		nBeginString = vectorOfStrings.size() - CONSOLE_HEIGHT / TEXT_VERTICAL_SIZE + MINUS_PAGE_SIZE;
	
	return true;
}

bool CWindowConsole::OnCtrlEnd( const struct SGameMessage &msg )
{
	if ( !IsVisible() ) 
		return false;

	nBeginString = 0;
	
	return true;
}

bool CWindowConsole::OnPgUp( const struct SGameMessage &msg )
{
	if ( !IsVisible() ) 
		return false;

	if ( nBeginString + CONSOLE_HEIGHT / TEXT_VERTICAL_SIZE < vectorOfStrings.size() )
		nBeginString += CONSOLE_HEIGHT / TEXT_VERTICAL_SIZE - MINUS_PAGE_SIZE;
	else
		nBeginString = vectorOfStrings.size() - CONSOLE_HEIGHT / TEXT_VERTICAL_SIZE + MINUS_PAGE_SIZE;
	
	return true;
}

bool CWindowConsole::OnPgDn( const struct SGameMessage &msg )
{
	if ( !IsVisible() ) 
		return false;

	if ( nBeginString - CONSOLE_HEIGHT / TEXT_VERTICAL_SIZE > 0 )
		nBeginString -= CONSOLE_HEIGHT / TEXT_VERTICAL_SIZE - MINUS_PAGE_SIZE;
	else
		nBeginString = 0;
	
	return true;
}

void CWindowConsole::SetParent( CWindow *_pParent )
{
	IWindow *pParent = GetParent();
	if ( pParent && IsValid( pParent ) ) 
	{
		if ( IsVisible() )
		{
			IScreen * pIScreen = GetScreen();
			if ( !pIScreen )
			{
				ASSERT(0);
			}
			else
				pIScreen->UndoStateCommandSequence( "ShowConsole" );
		}
	}
	CWindow::SetParent( _pParent );
}

bool CWindowConsole::OnShowConsole( const struct SGameMessage &msg )
{
	if( !IsVisible() )
		return false;
	
	IScreen * pIScreen = GetScreen();
	if ( !pIScreen )
	{
		ASSERT(0);
		return false;
	}
	
	pIScreen->RunStateCommandSequience( "ShowConsole", this, 0, false );

	return true;
}

void CWindowConsole::ParseCommand( const std::wstring &szExtCommand )
{
	nBeginCommand = -1;
	std::string szCommandString;
	NStr::ToMBCS( &szCommandString, szExtCommand );
	NStr::TrimLeft( szCommandString );
	if ( szCommandString.empty() )
		return;
	// check for special commands
	if ( szCommandString[0] == '@' )
	{
		if ( NGlobal::GetVar("VVP", 0) == 1 )
			WriteToPipe( PIPE_SCRIPT_CMDS, szCommandString.c_str() + 1 );
		return;
	}
	else if ( szCommandString[0] == '#' )
	{
		WriteToPipe( PIPE_WORLD_CMDS, szCommandString.c_str() + 1 );
		return;
	}
	else if ( szCommandString[0] == '$' )
	{
		WriteToPipe( PIPE_GLOBE_CMDS, szCommandString.c_str() + 1 );
		return;
	}
	else
	{
		NGlobal::ProcessCommand( szExtCommand );
		return;
	}

	// unknown command - report it
//	const string szError = string( "Unknown command: " ) + szCommandString;
//	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, szError.c_str(), 0xffff0000 );
}


// CDebugSingleton


void CDebugSingleton::CreateDebugInfo()
{
	if ( 0 != CUIFactory::GetConsts() )
	{
		pDebugInfo = checked_cast<IWindow*>( CUIFactory::MakeWindow( CUIFactory::GetConsts()->pDebugInfo ) );
		pDebugInfo->ShowWindow( bDebugShown );
	}
}

void CDebugSingleton::CreateConsole()
{
	if ( 0 != CUIFactory::GetConsts() )
		pConsole = checked_cast<IWindow*>( CUIFactory::MakeWindow( CUIFactory::GetConsts()->pConsole ) );
}

IWindow * CDebugSingleton::GetConsole()
{
	if ( !pConsole )
		CreateConsole();
	return pConsole;
}

IWindow * CDebugSingleton::GetDebug()
{
	if ( !pDebugInfo )
		CreateDebugInfo();
	return pDebugInfo;
}

IWindow * CDebugSingleton::GetDebugInfoWindow( const int nWindow )
{
	if ( !pDebugInfo )
		CreateDebugInfo();
	if ( !pDebugInfo )
		return 0;
	return pDebugInfo->GetChild( fmt::format( "DebugWindow{}", nWindow ), false );
}

void CDebugSingleton::ShowDebugInfo( const bool bShow )
{
	bDebugShown = bShow;
	if ( !pDebugInfo )
		CreateDebugInfo();
	if ( pDebugInfo )
		pDebugInfo->ShowWindow( bShow );
}

void CDebugSingleton::CreateStatsWindow()
{
	if ( 0 != CUIFactory::GetConsts() && CUIFactory::GetConsts()->pStatsWindow != 0 )
		pStatsWindow = checked_cast<IWindow*>( CUIFactory::MakeWindow( CUIFactory::GetConsts()->pStatsWindow ) );
}

void CDebugSingleton::ShowStatsWindow( const bool bShow )
{
	if ( !pStatsWindow )
		CreateStatsWindow();
	if ( pStatsWindow )
		pStatsWindow->ShowWindow( bShow );
}

struct IStatsSystemWindow * CDebugSingleton::GetStatsWindow()
{
	if ( !pStatsWindow )
		CreateStatsWindow();
	return dynamic_cast_ptr<IStatsSystemWindow*>( pStatsWindow );
}

IDebugSingleton *CreateDebugSingleton()
{
	return new CDebugSingleton;
}

START_REGISTER(UIDebugInfoCommand)
REGISTER_CMD( "Debug", ShowDebugInfo );
REGISTER_CMD( "Stats", ShowStatsWindow );
REGISTER_VAR_EX( "console_size", NGlobal::VarIntHandler, &nConsoleSize, 100, STORAGE_SAVE );
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( UI, 0x11075B82, CWindowConsole )
REGISTER_SAVELOAD_CLASS( UI, 0x11095440, CDebugSingleton )

