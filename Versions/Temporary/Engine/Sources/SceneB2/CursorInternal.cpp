#include "stdafx.h"

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <wtypes.h>
#include <winuser.h>
#else
#include "port/window.h"

#include <SDL3/SDL.h>
#endif

#include "CursorInternal.h"
#include "System/VFSOperations.h"
#include "System/WinFrame.h"
#include "System/WinCursor.h"

#include <fmt/format.h>

BASIC_REGISTER_CLASS( SCENEB2, ICursor );

CCursor::CCursor() 
: nCurrMode( 0 ), bAcquired( false ), bShow( true ), bCanShow( true )
{
	Show( false );
}

CCursor::~CCursor() 
{ 
	for ( CModesMap::iterator it = modes.begin(); it != modes.end(); ++it )
	{
		if ( it->second != 0 ) 
			NWinCursor::DestroyCursor( it->second );
	}
	modes.clear();
}

void CCursor::RegisterMode( const int nMode, const std::string &szFileName )
{
//	NI_ASSERT( modes.find(nMode) == modes.end(), StrFmt("Cursor mode %d already registered", nMode) );
	if ( szFileName.empty() || szFileName == " " )
		return;

	//
	NWinCursor::TCursor hCursor = NWinCursor::LoadCursor( szFileName );
	NI_ASSERT( hCursor != 0, fmt::format("Can't load cursor \"{}\" from file", szFileName) );
	if ( hCursor != 0 ) 
	{
		modes[nMode] = hCursor;
		modesFiles[nMode] = szFileName;
	}
}

bool CCursor::SetMode( const int nMode )
{
	if ( !modes.empty() )
	{
		nCurrMode = nMode;
		CModesMap::const_iterator posCursor = modes.find( nMode );
		if ( posCursor == modes.end() )
			posCursor = modes.find( 0 );
		hCurrCursor = posCursor->second;
	//	hCurrCursor = modes[eMode];
		OnSetCursor();

		return true;
	}
	else
		return false;
}

void CCursor::OnSetCursor()
{
	if ( bCanShow )
		NWinFrame::SetCursor( bShow ? hCurrCursor : 0 );
}

void CCursor::Show( const bool _bShow )
{
	bShow = _bShow;
	if ( bShow ) 
		SetMode( nCurrMode );
	else
		NWinFrame::SetCursor( 0 );
}

void CCursor::CanShow( const bool _bCanShow )
{
	bCanShow = _bCanShow;
	if ( !bCanShow )
		Show( false );
//	else
//		OnSetCursor();
}

void CCursor::SetBounds( const int x1, const int y1, const int x2, const int y2 )
{
	rcClip.Set( x1, y1, x2, y2 );
	AcquireLocal();
}

void CCursor::AcquireLocal()
{
#if BOOST_OS_WINDOWS
	if ( bAcquired ) 
		::ClipCursor( (const RECT*)&rcClip );
	else
		::ClipCursor( 0 );
#else
	// SDL confines the pointer to a rectangle inside a window rather than to one
	// on the screen, and takes a width and a height where Win32 takes a second
	// corner. The game window is borderless and covers the display, so the two
	// coordinate systems coincide: the callers pass 0,0 to the screen size, or a
	// one pixel box when the camera wants the pointer held still while it drags.
	SDL_Window *pWindow = AsSdlWindow( NWinFrame::GetWnd() );
	if ( pWindow == 0 )
		return;
	if ( bAcquired )
	{
		const SDL_Rect rect = { static_cast<int>( rcClip.left ), static_cast<int>( rcClip.top ),
			static_cast<int>( rcClip.right - rcClip.left ), static_cast<int>( rcClip.bottom - rcClip.top ) };
		SDL_SetWindowMouseRect( pWindow, &rect );
	}
	else
		SDL_SetWindowMouseRect( pWindow, 0 );
#endif
}

void CCursor::Acquire( const bool bAcquire )
{
	bAcquired = bAcquire;
	AcquireLocal();
}

void CCursor::SetPos( const int nX, const int nY )
{
#if BOOST_OS_WINDOWS
	::SetCursorPos( nX, nY );
#else
	// Warped within the window rather than globally. A Wayland compositor does
	// not let a client place the pointer on the screen, and it does not have to:
	// the callers work in the coordinates of a window that covers the display.
	SDL_Window *pWindow = AsSdlWindow( NWinFrame::GetWnd() );
	if ( pWindow != 0 )
		SDL_WarpMouseInWindow( pWindow, static_cast<float>( nX ), static_cast<float>( nY ) );
#endif
}

const CVec2 CCursor::GetPos() const
{
#if BOOST_OS_WINDOWS
	POINT point;
	::GetCursorPos( &point );
	return CVec2( point.x, point.y );
#else
	// Window relative for the same reason SetPos warps that way, and because the
	// global position is not something every backend will answer.
	float fX = 0, fY = 0;
	SDL_GetMouseState( &fX, &fY );
	return CVec2( fX, fY );
#endif
}

int CCursor::operator&( IBinSaver &saver )
{
	saver.Add( 1, &nCurrMode );
	saver.Add( 2, &rcClip );
	saver.Add( 3, &bAcquired );
	saver.Add( 4, &bShow );
	saver.Add( 5, &modesFiles );
	saver.Add( 6, &bCanShow );
	//
	if ( saver.IsReading() ) 
	{
		modes.clear();
		std::unordered_map<int, std::string> modesFilesCopy = modesFiles;
		modesFiles.clear();
		for ( std::unordered_map<int, std::string>::iterator iter = modesFilesCopy.begin(); iter != modesFilesCopy.end(); ++iter )
			RegisterMode( iter->first, iter->second );

		Show( bShow );
	}
	//
	return 0;
}

ICursor *CreateCursor()
{
	return new CCursor();
}

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x1007AC00, CCursor )


