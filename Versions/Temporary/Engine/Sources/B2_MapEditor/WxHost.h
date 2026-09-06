#pragma once

// wx hosted inside the editor's MFC application.
//
// Compiled only when BUILD_WX_EDITOR is on; main.cpp includes this
// unconditionally and gets an empty file otherwise, so the wx path is one
// #ifdef in one header rather than a scatter of them through the app class.
//
// The shape is fixed by who owns what. MFC's CWinApp owns the entry point, the
// main window and the message loop, and it is going to keep owning them for as
// long as both toolkits are in the process, so wx goes in as the guest: started
// by hand after MFC is up, given a look at every message before MFC translates
// it, pumped from MFC's idle, and shut down before MFC exits. wxWidgets ships
// exactly this in wx/msw/mfc.h -- wxMFCApp<T> for the CWinApp side and
// wxAppWithMFC for the wxApp side -- so this is wx's own supported arrangement
// rather than something invented here.
//
// The reason it can be in the same translation unit as MFC at all: wx does not
// need _UNICODE. wxUSE_UNICODE is wx's own, out of its setup.h, and decides what
// wxString is; _UNICODE is Win32's and decides TCHAR mapping and which MFC
// runtime is linked. Defining it to satisfy wx would have quietly asked for the
// Unicode MFC alongside the MBCS one the rest of the editor uses. See
// cmake/wxwidgets.cmake.

#ifdef OBK2_WITH_WX

// wx/msw/mfc.h refuses to be included before the MFC headers, so this file has
// to come after stdafx.h. It is included from exactly one place for that reason.
#include <wx/wx.h>
#include <wx/msw/mfc.h>

namespace NWxHost
{
	// Declared before the class that calls it; defined in WxHost.cpp. See the
	// comment on it further down.
	void ShowProbeFrameIfAsked();

	// The wxApp for a process whose message loop belongs to MFC. wxAppWithMFC
	// redirects the two things a wxApp would otherwise do to its own event loop
	// -- exiting it, and waking it for idle -- at MFC's instead.
	//
	// OnInit deliberately creates nothing. In wx's own sample the wxApp creates
	// the main window and MFC wraps it; here MFC created the main window twenty
	// years ago and wx is the newcomer, so there is nothing for it to make until
	// a real front-end asks.
	class CWxHostApp : public wxAppWithMFC
	{
	public:
		virtual bool OnInit()
		{
			// Stated rather than left to the default, because the default is the
			// wrong answer here and the failure would be spectacular. wx normally
			// ends the application when its last top-level window closes, and
			// wxAppWithMFC implements "end the application" as ::PostQuitMessage,
			// which is MFC's message loop. So a wx tool window being closed could
			// shut the whole editor down, with unsaved work in it. MFC owns this
			// process's lifetime; wx does not get a vote.
			SetExitOnFrameDelete( false );
			return true;
		}
	};

	// Mixed into the editor's own CWinApp-derived class. wxMFCApp<T> derives
	// from T and wraps InitInstance, ExitInstance, PreTranslateMessage and
	// OnIdle around it.
	//
	// Two of its four have to be overridden, both because wx's default assumes
	// the wx side owns the main window and here the MFC side does:
	//
	//   InitMainWnd  - wx's version takes wxTheApp->GetTopWindow(), wraps it in
	//                  a wxMFCWnd and makes that CWinApp::m_pMainWnd. The editor
	//                  already has a main window and its own m_pMainWnd, and
	//                  there is no wx top-level window at startup, so wx's
	//                  version would fail the startup outright.
	//
	//   ExitInstance - wx's version deletes m_pMainWnd, which it is entitled to
	//                  do because in its model that pointer is the wxMFCWnd it
	//                  made itself. Here it is the editor's CMainFrame, which
	//                  MFC destroys through PostNcDestroy, and deleting it here
	//                  would be a double free on the way out. The rest of what
	//                  that method does -- CallOnExit then wxEntryCleanup -- is
	//                  kept, in that order.
	template <typename TBaseApp>
	class CWxHostedApp : public wxMFCApp<TBaseApp>
	{
	public:
		typedef wxMFCApp<TBaseApp> CWxBase;

		virtual BOOL InitInstance()
		{
			// CWxBase::InitInstance runs the editor's own InitInstance first and
			// only then starts wx, so anything that needs wx has to be after this
			// line rather than inside the editor's. Getting that backwards is
			// what the first version of this did, and it died in
			// wxBrushList::FindOrCreateBrush on the probe frame's first
			// WM_ERASEBKGND -- wx's stock objects not existing yet.
			if ( !CWxBase::InitInstance() )
			{
				return FALSE;
			}
			ShowProbeFrameIfAsked();
			return TRUE;
		}

		virtual int ExitInstance()
		{
			if ( wxTheApp )
			{
				wxTheApp->CallOnExit();
			}
			wxEntryCleanup();
			// Straight to the editor's own, stepping over wxMFCApp's.
			return TBaseApp::ExitInstance();
		}

	protected:
		virtual BOOL InitMainWnd()
		{
			return TRUE;
		}
	};

	// The probe. Puts a wx frame on screen beside the editor's own, so that the
	// question the whole exercise is asking -- can these two share a process and
	// a message loop -- has an answer that can be looked at and photographed
	// rather than argued about.
	//
	// Off unless OBK2_WX_PROBE is set in the environment. Not a command-line
	// switch: CEditorApp treats a non-empty command line as a file to open and
	// hands it to the single-instance checker, so a switch there would be taken
	// for a map name.
	void ShowProbeFrameIfAsked();
}

#endif // OBK2_WITH_WX
