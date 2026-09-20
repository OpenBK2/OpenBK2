#pragma once

#include "MapEditorLib_export.h"

#include <string>

// Putting text on the clipboard.
//
// This replaces the Win32 sequence, which is worth explaining because the
// GlobalAlloc in it looks like a memory question and is not one:
//
//   hText = GlobalAlloc( GMEM_MOVEABLE, nBytes );   // movable, not fixed
//   memcpy( GlobalLock( hText ), ... ); GlobalUnlock( hText );
//   OpenClipboard( hOwner ); EmptyClipboard();
//   SetClipboardData( CF_UNICODETEXT, hText );      // the clipboard owns it now
//   CloseClipboard();
//
// SetClipboardData does not copy the buffer, it takes the handle: on success
// the clipboard owns the block and the caller must not free it, and on failure
// the caller must. That ownership transfer is the only reason the allocation
// was a movable HGLOBAL rather than anything else, and it is 16-bit Windows
// heritage that the clipboard API never shed. Hand the transfer to wx and the
// whole dance goes away rather than needing a port; so does the owner window
// that OpenClipboard demanded, which was ::GetActiveWindow().
//
// wxTextDataObject is the same format. On MSW it is CF_UNICODETEXT, which is
// what the editor asked for; elsewhere wx picks what the platform uses.
//
// Text is UTF-8, as every narrow string in the tree is. The call this replaced
// widened its bytes one at a time with std::wstring( s.begin(), s.end() ),
// which is a latin-1 widening rather than a UTF-8 decode; it was right only
// because the text was digits, braces and commas. wxString::FromUTF8 is right
// whatever the text turns out to be.
//
// Declared without naming a toolkit and implemented once in ClipboardWx.cpp,
// for the reason MessageBoxes.h gives: an editor state that includes a wx
// header loses windows.h's A/W macros and stops linking against
// NDb::GetObjectA. The wx views call wxTheClipboard directly and are welcome
// to; this is for everything that must not see wx.
namespace NClipboard
{
	// False if the clipboard could not be opened or the text not set. Empty
	// text clears the clipboard, as the wx views do.
	MAPEDITORLIB_EXPORT bool SetText( const std::string &rszText );
}
