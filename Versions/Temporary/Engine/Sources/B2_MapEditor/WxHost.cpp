#include "stdafx.h"

#include "WxHost.h"

#include "MapEditor/MapEditorApp.h"

#include <wx/app.h>

namespace
{
	// The editor's arguments: wx's argv without the program's name.
	//
	// This was GetCommandLineA() with the program name cut off by hand, the way
	// the CRT cuts it for WinMain, because that is what CWinApp::m_lpCmdLine
	// held and the editor then trimmed the quotes itself. wx fills argc and argv
	// in wxEntryStart, before OnInit runs, already split and unquoted, so all of
	// that was reproducing work wx had done.
	std::vector<std::string> EditorArguments( const wxApp &rApp )
	{
		std::vector<std::string> args;
		for ( int i = 1; i < rApp.argc; ++i )
		{
			args.push_back( std::string( rApp.argv[i].utf8_str() ) );
		}
		return args;
	}


	class CEditorWxApp : public wxApp
	{
	public:
		// wxApp::OnInit is deliberately not called: it would parse the command
		// line as wx's, and reject what it does not know. argc and argv are set
		// up regardless -- that happens in wxEntryStart, not here.
		bool OnInit() override
		{
			// Stated rather than left to the default, because the default is the
			// wrong answer here and the failure would be spectacular. wx normally
			// ends the application when its last top-level window closes, so a wx
			// tool window being closed before the frame exists could shut the
			// whole editor down. The main frame ends the loop when it goes, and
			// nothing else gets a vote.
			SetExitOnFrameDelete( false );
			if ( !NWxHost::GetEditorApp().Initialize( EditorArguments( *this ) ) )
			{
				// MFC ran ExitInstance after a failed InitInstance; wx does not
				// run OnExit after a failed OnInit, so this does.
				NWxHost::GetEditorApp().Shutdown();
				return false;
			}
			return true;
		}

		// After the loop, before wx itself goes: the editor went down before
		// wx did under CWxHostedApp too.
		int OnExit() override
		{
			NWxHost::GetEditorApp().Shutdown();
			return wxApp::OnExit();
		}
	};
}

// WinMain, wx's. See WxHost.h for why it is no longer MFC's.
wxIMPLEMENT_APP( CEditorWxApp );
