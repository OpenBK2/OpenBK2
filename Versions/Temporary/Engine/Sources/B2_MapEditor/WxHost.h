#pragma once

// wx hosted inside the editor's MFC application.
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
// The reason it can be in the same translation unit as MFC at all: wx needs
// neither UNICODE nor _UNICODE from a consumer. Three different switches are
// easy to run together here -- wxUSE_UNICODE is wx's own and decides what
// wxString is, UNICODE is Win32's and picks GetMessageA against GetMessageW,
// _UNICODE is the CRT's and decides TCHAR. MFC keys off _UNICODE, which is what
// selects CStringA against CStringW and which MFC import library is used, and
// MFC's MBCS and Unicode runtimes must not both load into one process. Defining
// it to satisfy wx would have asked for the second one. It is not defined, and
// the running editor loads exactly one MFC. See cmake/wxwidgets.cmake.


// wx/msw/mfc.h refuses to be included before the MFC headers, so this file has
// to come after stdafx.h. It is included from exactly one place for that reason.
#include <wx/wx.h>
#include <wx/msw/mfc.h>

#include "MapEditor/MainFrameWx.h"

namespace NWxHost
{
	// Declared before the class that calls it; defined in WxHost.cpp. See the
	// comment on it further down.
	void ShowProbeFrameIfAsked();
	// Whether the probe frame is still up. Answerable rather than assumed,
	// because the frame is held by a wxWeakRef that goes null when wx destroys
	// it -- see WxOwnership.h.
	bool IsProbeFrameOpen();

	// Whose message loop this session runs on: wx's for the wx frame, MFC's for
	// CMainFrame. Decided once, before wx starts, because wx makes its main
	// loop from traits it creates the first time it is asked for them.
	void SetWxOwnsLoop( bool bWxOwnsLoop );
	bool WxOwnsLoop();

	// What of the MFC application wx's loop still has to run: its
	// PreTranslateMessage, which walks the MFC windows from a message's window
	// up to the main window -- the property tree and the MFC palettes translate
	// the editor's accelerators there, and an MFC modeless dialog does its
	// keyboard navigation there -- and its OnIdle, which updates MFC's command UI
	// and frees MFC's temporary window objects. The base application's own, not
	// wxMFCApp's, which would call back into wx.
	struct SMfcHooks
	{
		void *pContext;
		BOOL ( *pfnPreTranslateMessage )( void *pContext, MSG *pMsg );
		BOOL ( *pfnOnIdle )( void *pContext, LONG lCount );
	};

	// wx's main loop, with MFC's look at every message and MFC's idle work in it,
	// until the WM_QUIT the main frame posts when it goes.
	int RunWxMainLoop( const SMfcHooks &rHooks );

	// The wxApp. In an MFC session the message loop belongs to MFC, and
	// wxAppWithMFC redirects the two things a wxApp would otherwise do to its own
	// event loop -- exiting it, and waking it for idle -- at MFC's instead. In a
	// session whose loop is wx's (WxOwnsLoop) those go to wx's own; its idle runs
	// MFC's as well, and the loop it makes runs MFC's PreTranslateMessage before
	// wx's own look at a message.
	//
	// OnInit deliberately creates nothing. In wx's own sample the wxApp creates
	// the main window and MFC wraps it; here the editor's startup makes the main
	// window, whichever toolkit's, so there is nothing for wx to make until a
	// real front-end asks.
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
			// shut the whole editor down, with unsaved work in it. The main frame
			// ends the loop when it goes, whichever frame and whichever loop, and
			// nothing else gets a vote.
			SetExitOnFrameDelete( false );
			return true;
		}

		virtual void ExitMainLoop() override;
		virtual void WakeUpIdle() override;
		virtual bool ProcessIdle() override;

	protected:
		virtual wxAppTraits* CreateTraits() override;
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

		// wxMFCApp::InitInstance is deliberately NOT called. Its order is
		// base-then-wx, which is right when the base is a plain CWinApp and
		// wrong here: the editor's InitInstance builds CMainFrame, and
		// CMainFrame builds its docking panes, and a pane whose contents are wx
		// needs wx to already exist. So wx starts first and the editor second.
		//
		// This was learned twice, the same way both times: a wx window created
		// before wxEntryStart dies on its first WM_ERASEBKGND inside
		// wxBrushList::FindOrCreateBrush, because wx's stock objects are made by
		// module initialisation that has not run. The stack arrives through
		// mfc140!_AfxActivationWndProc and reads like an MFC/wx conflict. It is
		// not one. If that signature appears again, look at ordering first.
		virtual BOOL InitInstance()
		{
			// Before wx starts, which may already make its traits, and they make
			// the loop.
			SetWxOwnsLoop( NMainFrameWx::IsWanted() );
			if ( !wxEntryStart( TBaseApp::m_hInstance ) )
			{
				return FALSE;
			}
			if ( !wxTheApp || !wxTheApp->CallOnInit() )
			{
				return FALSE;
			}
			// InitMainWnd is not called either: MFC owns the main window here,
			// and the override below says so.
			if ( !TBaseApp::InitInstance() )
			{
				return FALSE;
			}
			ShowProbeFrameIfAsked();
			return TRUE;
		}

		// Shut down in the opposite order to starting up: wx came up before the
		// editor, so it goes down after it. In practice the frame and everything
		// in it is already destroyed by the time MFC calls this -- the message
		// loop has ended -- but ordering that has to be reasoned about twice is
		// ordering worth writing down once.
		virtual int ExitInstance()
		{
			const int nResult = TBaseApp::ExitInstance();
			if ( wxTheApp )
			{
				wxTheApp->CallOnExit();
			}
			wxEntryCleanup();
			return nResult;
		}

		// The message loop, which AfxWinMain runs between InitInstance and
		// AfxWinTerm. MFC's, CWinThread::Run, for CMainFrame. For the wx frame,
		// wx's, and then what CWinThread::Run does when its loop ends on WM_QUIT:
		// ExitInstance, whose answer is the process's. MFC's entry point and its
		// module initialisation stay MFC's either way.
		virtual int Run()
		{
			if ( !WxOwnsLoop() )
			{
				return TBaseApp::Run();
			}
			const SMfcHooks hooks = { this, &PreTranslateHook, &IdleHook };
			RunWxMainLoop( hooks );
			return this->ExitInstance();
		}

	private:
		// The base application's own, for SMfcHooks: what MFC's pump would have
		// called, without wxMFCApp's wx half.
		static BOOL PreTranslateHook( void *pContext, MSG *pMsg )
		{
			return static_cast<CWxHostedApp*>( pContext )->TBaseApp::PreTranslateMessage( pMsg );
		}

		static BOOL IdleHook( void *pContext, LONG lCount )
		{
			return static_cast<CWxHostedApp*>( pContext )->TBaseApp::OnIdle( lCount );
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

