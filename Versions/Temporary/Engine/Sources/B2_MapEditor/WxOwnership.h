#pragma once

// wx's object ownership, stated once, and two helpers that put it in the name.
//
// The rules below were read out of wx 3.3.3's own source rather than recalled,
// because two of them are the opposite of what the code looks like:
//
//   * A child window is owned by its PARENT. wxWindowBase::DestroyChildren
//     deletes every child from the parent's destructor (wincmn.cpp).
//
//   * A window owns its SIZER. wxWindowBase::SetSizer deletes the previous one
//     and the window deletes the current one.
//
//   * A sizer owns nested SIZERS but NOT the windows in it. wxSizerItem::Free
//     does `delete m_sizer` for a sizer item and, for a window item, only
//     `m_window->SetContainingSizer(nullptr)` (sizer.cpp). So pSizer->Add( pWnd )
//     transfers nothing, and a pointer to a child window stays valid for as
//     long as its parent does -- including after it has been added to a sizer.
//
//   * A top-level window is owned by wx, which keeps it in wxTopLevelWindows and
//     destroys it when it is closed.
//
// None of this is expressible with std::unique_ptr. wx deletes these objects
// itself, so an owning smart pointer over a parented widget is a double free,
// the same as it would be in Qt. That constraint is not negotiable.
//
// What *is* fixable is the part that misleads: a bare `new` at a call site reads
// as an allocation nobody owns, and `( new CFrame( ... ) )->Show( true )` reads
// as a leak. It is not one, but a reader should not have to open sizer.cpp to
// find that out, and a static analyser will not. These two helpers move the
// `new` behind a name that says who owns the result, so the call sites stop
// lying and there is one place to annotate if clang-tidy's
// cppcoreguidelines-owning-memory is ever turned on.

#ifdef OBK2_WITH_WX

#include <wx/wx.h>
#include <wx/weakref.h>

#include <utility>

namespace NWx
{
	// A window owned by its parent. The returned pointer is borrowed: valid
	// until the parent is destroyed, and unaffected by being put in a sizer.
	//
	//   wxButton *const pButton = NWx::Child<wxButton>( pPanel, wxID_ANY, "Go" );
	template <typename TWindow, typename... TArgs>
	TWindow* Child( TArgs&&... rArgs )
	{
		return new TWindow( std::forward<TArgs>( rArgs )... );
	}

	// A top-level window, owned by wx. A weak reference is the only honest
	// handle to one: wx destroys it when it is closed, and this goes null when
	// that happens, which is something the host can actually test rather than
	// assume.
	//
	//   wxWeakRef<CMyFrame> pFrame = NWx::TopLevel<CMyFrame>( args );
	template <typename TWindow, typename... TArgs>
	wxWeakRef<TWindow> TopLevel( TArgs&&... rArgs )
	{
		return new TWindow( std::forward<TArgs>( rArgs )... );
	}
}

#endif // OBK2_WITH_WX
