#include "stdafx.h"

#include "EditorInstance.h"

#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/WxWidget.h"

#include <wx/ipc.h>
#include <wx/log.h>
#include <wx/snglinst.h>
#include <wx/string.h>
#include <wx/toplevel.h>

#include <memory>

namespace
{
	// The topic both ends name. wxIPC wants a service and a topic; the service
	// says where to connect and the topic what the conversation is about, and
	// one topic is all this needs.
	const char *const TOPIC = "OpenFile";

	// What the second instance sends when it only wants the window raised. An
	// empty Execute would do, but a word says what it means in a trace.
	const char *const JUST_RAISE = "";

	std::string g_szName = "OBK2MapEditor";
	std::unique_ptr<wxSingleInstanceChecker> g_pChecker;
	NEditorInstance::TOpenHandler g_pfnOpen = nullptr;


	// Where a client connects.
	//
	// The two transports wx picks between want different things here, which is
	// the one part of this that is not the same on both platforms. On Windows
	// wxIPC is DDE and the service is a name; everywhere else it is TCP, where
	// wx takes a port number or, on Unix, the path of a socket file. A socket
	// file keeps this to the user's own session rather than opening a port, and
	// costs no firewall prompt.
	wxString ServiceName()
	{
#if wxUSE_DDE_FOR_IPC
		return wxString::FromUTF8( g_szName.c_str() );
#else
		return wxString::Format( "/tmp/%s-%ld", g_szName.c_str(),
														 static_cast<long>( ::getuid() ) );
#endif
	}


	void RaiseMainFrame()
	{
		IMainFrameContainer *const pContainer = Singleton<IMainFrameContainer>();
		IWidget *const pWindow = ( pContainer != nullptr ) ? pContainer->GetMainWindow() : nullptr;
		wxWindow *const pFrame = ( pWindow != nullptr ) ? ToWxOwnerWindow( pWindow ) : nullptr;
		if ( pFrame == nullptr )
		{
			return;
		}
		// What BringAppOnTop did with GetWindowPlacement and ShowWindow: restore
		// it if it was minimised, then bring it forward.
		if ( wxTopLevelWindow *const pTop = wxDynamicCast( pFrame, wxTopLevelWindow ) )
		{
			if ( pTop->IsIconized() )
			{
				pTop->Iconize( false );
			}
		}
		pFrame->Raise();
	}


	class CInstanceConnection : public wxConnection
	{
	public:
		// The path arrives as the Execute data. wxConnection hands it over as
		// bytes and a size, which is what WM_COPYDATA did.
		virtual bool OnExecute( const wxString &WXUNUSED( topic ), const void *pData,
														size_t nSize, wxIPCFormat eFormat ) override
		{
			RaiseMainFrame();
			const wxString path = GetTextFromData( pData, nSize, eFormat );
			if ( !path.empty() && ( g_pfnOpen != nullptr ) )
			{
				g_pfnOpen( std::string( path.utf8_str() ) );
			}
			return true;
		}
	};


	class CInstanceServer : public wxServer
	{
	public:
		virtual wxConnectionBase* OnAcceptConnection( const wxString &rTopic ) override
		{
			return ( rTopic == TOPIC ) ? new CInstanceConnection() : nullptr;
		}
	};


	std::unique_ptr<CInstanceServer> g_pServer;
}


namespace NEditorInstance
{
	void SetName( const std::string &rszName )
	{
		g_szName = rszName;
	}


	bool IsAnotherRunning()
	{
		if ( !g_pChecker )
		{
			// The name is the lock. wxSingleInstanceChecker adds the user's name to
			// it itself, so two people on one machine do not exclude each other.
			g_pChecker.reset( new wxSingleInstanceChecker( wxString::FromUTF8( g_szName.c_str() ) ) );
		}
		return g_pChecker->IsAnotherRunning();
	}


	bool AskRunningToOpen( const std::string &rszFilePath )
	{
		// Quiet: failing to connect is an expected answer here, not something to
		// put a dialog in front of anyone about.
		wxLogNull noLog;
		wxClient client;
		const std::unique_ptr<wxConnectionBase> pConnection(
			client.MakeConnection( "localhost", ServiceName(), TOPIC ) );
		if ( !pConnection )
		{
			return false;
		}
		const wxString payload = rszFilePath.empty()
			? wxString::FromUTF8( JUST_RAISE )
			: wxString::FromUTF8( rszFilePath.c_str() );
		return pConnection->Execute( payload );
	}


	void StartAnswering( TOpenHandler pfnOpen )
	{
		g_pfnOpen = pfnOpen;
		if ( g_pServer )
		{
			return;
		}
		wxLogNull noLog;
		g_pServer.reset( new CInstanceServer() );
		if ( !g_pServer->Create( ServiceName() ) )
		{
			// Losing the race for the service is not fatal: the editor runs, it
			// just cannot be handed a file by a second instance.
			g_pServer.reset();
		}
	}


	void Release()
	{
		g_pServer.reset();
		g_pChecker.reset();
		g_pfnOpen = nullptr;
	}
}
