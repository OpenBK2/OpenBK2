#include "stdafx.h"

#include "WxHost.h"

#include "MapEditor/MapEditorApp.h"

#include <wx/app.h>

namespace
{
	// What CWinApp::m_lpCmdLine held, which is what the editor parses: the
	// command line after the program's name and the blanks that follow it,
	// the way the CRT cuts it for WinMain. wx's own argv is not used, because
	// it splits and unquotes the arguments and the editor never did.
	std::string CommandLineArguments()
	{
		const char *pszLine = ::GetCommandLineA();
		if ( *pszLine == '"' )
		{
			++pszLine;
			while ( ( *pszLine != '\0' ) && ( *pszLine != '"' ) )
			{
				++pszLine;
			}
			if ( *pszLine == '"' )
			{
				++pszLine;
			}
		}
		else
		{
			while ( ( *pszLine != '\0' ) && ( *pszLine != ' ' ) && ( *pszLine != '\t' ) )
			{
				++pszLine;
			}
		}
		while ( ( *pszLine == ' ' ) || ( *pszLine == '\t' ) )
		{
			++pszLine;
		}
		return pszLine;
	}


	class CEditorWxApp : public wxApp
	{
	public:
		// wxApp::OnInit is deliberately not called: it would parse the command
		// line as wx's, and reject what it does not know.
		bool OnInit() override
		{
			// Stated rather than left to the default, because the default is the
			// wrong answer here and the failure would be spectacular. wx normally
			// ends the application when its last top-level window closes, so a wx
			// tool window being closed before the frame exists could shut the
			// whole editor down. The main frame ends the loop when it goes, and
			// nothing else gets a vote.
			SetExitOnFrameDelete( false );
			if ( !NWxHost::GetEditorApp().Initialize( CommandLineArguments() ) )
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
