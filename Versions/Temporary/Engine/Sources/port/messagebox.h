#pragma once

#include <boost/predef.h>

// A message box, for the few places that report a failure the log cannot reach.
//
// SDL has a real one rather than something to be assembled: SDL_ShowMessageBox
// takes a button list and returns which was pressed, and SDL_ShowSimpleMessageBox
// is the one-button form. Neither needs SDL_Init first, which matters because
// every caller here is a startup failure that happens before there is a window.
//
// What is deliberately absent is everything beyond one button. All twenty-odd
// MB_YESNO, MB_YESNOCANCEL and MB_DEFBUTTON2 calls in this tree are in the
// editor, which builds on no platform, and the six that are not are MB_OK. So
// the multi-button flags are not defined here at all: a future call that wants
// an answer fails to compile rather than silently receiving IDOK, which is the
// one outcome a confirmation dialog must never invent. SDL_ShowMessageBox is
// what that case would be built on when it arrives.
#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <windows.h>

#include <SDL3/SDL.h>

#include <cstdio>
#endif

#if !BOOST_OS_WINDOWS

#define MB_OK 0x00000000
#define MB_ICONERROR 0x00000010
#define MB_ICONSTOP MB_ICONERROR
#define MB_ICONEXCLAMATION 0x00000030
#define MB_ICONWARNING MB_ICONEXCLAMATION
#define MB_ICONINFORMATION 0x00000040
#define MB_ICONASTERISK MB_ICONINFORMATION

#define MB_ICONMASK 0x000000F0

#define IDOK 1

//! Show a message with a single OK button and wait for it.
//!
//! Always returns IDOK, which is what the one-button form can return. The
//! caller cannot learn anything from a dialog with one button, and none of the
//! callers here look.
inline int MessageBox( HWND hParent, const char *pszText, const char *pszCaption, unsigned nType )
{
	SDL_MessageBoxFlags nFlags = SDL_MESSAGEBOX_INFORMATION;
	switch ( nType & MB_ICONMASK )
	{
	case MB_ICONERROR:
		nFlags = SDL_MESSAGEBOX_ERROR;
		break;
	case MB_ICONEXCLAMATION:
		nFlags = SDL_MESSAGEBOX_WARNING;
		break;
	default:
		break;
	}
	if ( !SDL_ShowSimpleMessageBox( nFlags, pszCaption != 0 ? pszCaption : "", pszText != 0 ? pszText : "",
	                                static_cast<SDL_Window *>( hParent ) ) )
	{
		// SDL says to expect this where there is no video target to draw on, and
		// to say it another way rather than lose it. A message that only appears
		// when a window can be made is no use to someone running over ssh, which
		// is where a failure to bring up the display is most likely to be read.
		fprintf( stderr, "%s: %s\n", pszCaption != 0 ? pszCaption : "Error", pszText != 0 ? pszText : "" );
	}
	return IDOK;
}

#endif
