#pragma once

// wx, run from inside the editor's MFC application object.
//
// MFC's CWinApp still owns the entry point (AfxWinMain) and its module
// initialisation, for as long as MFC is linked, so wx is started by hand from
// the application's InitInstance, runs its own message loop from Run, and is
// shut down from ExitInstance. Everything on screen is wx's; MFC has no window
// and no part in the loop.
//
// Never give B2_MapEditor a WinMain of its own to be rid of this: that drops
// MFC's appmodul.obj, and with it the AfxInitialize call MFC still relies on.
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
//
// This used to host wx as a guest of MFC's loop, through wx/msw/mfc.h's
// wxMFCApp and wxAppWithMFC, and later ran wx's loop with MFC's
// PreTranslateMessage and OnIdle hooked into it, for the MFC windows that
// were left. There are none now, and all of that went.

#include <wx/app.h>
#include <wx/init.h>

namespace NWxHost
{
	// The wxApp. OnInit deliberately creates nothing: the editor's startup
	// makes the main window.
	class CWxHostApp : public wxApp
	{
	public:
		virtual bool OnInit()
		{
			// Stated rather than left to the default, because the default is the
			// wrong answer here and the failure would be spectacular. wx normally
			// ends the application when its last top-level window closes, so a wx
			// tool window being closed before the frame exists could shut the
			// whole editor down. The main frame ends the loop when it goes, and
			// nothing else gets a vote.
			SetExitOnFrameDelete( false );
			return true;
		}
	};

	// Mixed into the editor's own CWinApp-derived class: wx up before it, wx's
	// loop instead of MFC's, wx down after it.
	template <typename TBaseApp>
	class CWxHostedApp : public TBaseApp
	{
	public:
		// wx starts first and the editor second: the editor's InitInstance
		// builds the main frame, which is wx's, so wx has to exist first.
		//
		// This was learned twice, the same way both times: a wx window created
		// before wxEntryStart dies on its first WM_ERASEBKGND inside
		// wxBrushList::FindOrCreateBrush, because wx's stock objects are made by
		// module initialisation that has not run. If that signature appears
		// again, look at ordering first.
		virtual BOOL InitInstance()
		{
			if ( !wxEntryStart( TBaseApp::m_hInstance ) )
			{
				return FALSE;
			}
			if ( !wxTheApp || !wxTheApp->CallOnInit() )
			{
				return FALSE;
			}
			return TBaseApp::InitInstance();
		}

		// Shut down in the opposite order to starting up: wx came up before the
		// editor, so it goes down after it.
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
		// AfxWinTerm: wx's, until the WM_QUIT the main frame posts when it goes,
		// and then what CWinThread::Run does when its loop ends: ExitInstance,
		// whose answer is the process's.
		virtual int Run()
		{
			wxTheApp->OnRun();
			return this->ExitInstance();
		}
	};
}
