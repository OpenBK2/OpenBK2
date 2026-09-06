// The wxWidgets skeleton.
//
// This exists to answer build and link questions, not to do anything: pull the
// header in, call into the library, put a window on screen, and prove the
// toolchain works before anything is written against it. The editor front-end
// comes later and is a different program; this one is deliberately throwaway in
// everything except what it proves.
//
// Two things about it are not arbitrary.
//
// It is a **console** executable, unlike every other GUI program here, so that
// what it found out is readable without a person looking at a screen. `--no-gui`
// prints and exits, which is what a script or a CI step wants; with no argument
// it also puts up a frame, which is the part that proves wx can actually create
// a window in this process.
//
// It does **not** use wxIMPLEMENT_APP. That macro writes a WinMain, and the
// thing this port eventually needs is the opposite: wx started by hand from
// inside a host that already owns the entry point and the message loop, which is
// what MFC does in the real editor. Starting it through wxEntry here keeps the
// skeleton the same shape as the thing it is a rehearsal for.

#include <wx/wx.h>
#include <wx/utils.h>
#include <wx/platinfo.h>
#include <wx/versioninfo.h>
#include <wx/init.h>

#include <cstdio>
#include <cstring>
#include <string>

// The boundary this port has to keep straight, in one place.
//
// wx on MSW is a wide-character library and there is no narrow build of it any
// more. Everything else in this tree is narrow UTF-8, with a manifest setting
// the process code page so the ...A entry points agree. Both are fine; what is
// not fine is converting ad hoc at three hundred call sites, so the conversion
// lives behind functions like these two and the front-end calls them.
static std::string ToUtf8( const wxString &rString )
{
	return std::string( rString.utf8_str() );
}

static wxString FromUtf8( const std::string &rszString )
{
	return wxString::FromUTF8( rszString.c_str() );
}


class CWxSkeletonFrame : public wxFrame
{
public:
	explicit CWxSkeletonFrame( const wxString &rstrDetails )
		: wxFrame( nullptr, wxID_ANY,
							 // The title carries the version so that windump.py can read it
							 // from outside the process, which is how everything else in this
							 // port gets checked.
							 wxString::Format( "wx skeleton - %s", wxVERSION_STRING ),
							 wxDefaultPosition, wxSize( 560, 320 ) )
	{
		wxTextCtrl *pDetails = new wxTextCtrl( this, wxID_ANY, rstrDetails,
																					 wxDefaultPosition, wxDefaultSize,
																					 wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP );
		wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
		pSizer->Add( pDetails, wxSizerFlags( 1 ).Expand().Border( wxALL, 8 ) );
		SetSizer( pSizer );
		CreateStatusBar();
		SetStatusText( "wxWidgets is up." );
	}
};


// Everything the skeleton is trying to find out, in one string, so that the
// console path and the window path report exactly the same thing.
static wxString CollectDetails()
{
	wxString strDetails;
	strDetails << "wxVERSION_STRING : " << wxVERSION_STRING << "\n";
	strDetails << "library version  : " << wxGetLibraryVersionInfo().ToString() << "\n";
	strDetails << "port             : " << wxPlatformInfo::Get().GetPortIdName() << "\n";
	strDetails << "OS               : " << wxGetOsDescription() << "\n";
	strDetails << "architecture     : " << ( sizeof( void* ) * 8 ) << "-bit\n";
	strDetails << "wxUSE_UNICODE    : " << wxUSE_UNICODE << "\n";
	strDetails << "sizeof(wxChar)   : " << (int)sizeof( wxChar ) << "\n";
	// The round trip that matters: a non-ASCII string through the narrow/wide
	// boundary and back. If the conversion helpers are wrong this is where it
	// shows, rather than in a unit name three months from now.
	const std::string szUtf8 = "\xd0\x9b\xd0\xb5\xd0\xb3\xd0\xba\xd0\xb8\xd0\xb9 \xd1\x82\xd0\xb0\xd0\xbd\xd0\xba";
	const wxString strWide = FromUtf8( szUtf8 );
	const bool bRoundTrip = ( ToUtf8( strWide ) == szUtf8 );
	strDetails << "utf-8 round trip : " << ( bRoundTrip ? "ok" : "FAILED" )
						 << " (" << (int)strWide.length() << " wide chars from "
						 << (int)szUtf8.size() << " utf-8 bytes)\n";
	return strDetails;
}


class CWxSkeletonApp : public wxApp
{
public:
	virtual bool OnInit()
	{
		if ( !wxApp::OnInit() )
		{
			return false;
		}
		( new CWxSkeletonFrame( CollectDetails() ) )->Show( true );
		return true;
	}
};

wxIMPLEMENT_APP_NO_MAIN( CWxSkeletonApp );


int wmain( int argc, wchar_t **argv )
{
	bool bNoGui = false;
	for ( int nArg = 1; nArg < argc; ++nArg )
	{
		if ( wcscmp( argv[nArg], L"--no-gui" ) == 0 )
		{
			bNoGui = true;
		}
	}

	{
		// Not everything here works on an uninitialised wx: wxPlatformInfo::Get()
		// asserts with "failed to initialize wxPlatformInfo" if it is the first
		// thing called, and so does wxGetOsDescription behind it. wxInitializer
		// is the documented way to bring wx up far enough to use it without a
		// wxApp, which is exactly what the --no-gui path wants, and getting an
		// answer out of it is itself a check that wx can initialise at all.
		wxInitializer initializer;
		if ( !initializer.IsOk() )
		{
			std::fprintf( stderr, "wxInitialize failed\n" );
			return 1;
		}
		std::printf( "%s", ToUtf8( CollectDetails() ).c_str() );
		std::fflush( stdout );
	}

	if ( bNoGui )
	{
		return 0;
	}

	// wxEntry takes argc by reference because it removes the arguments it
	// consumes.
	int nArgc = argc;
	return wxEntry( nArgc, argv );
}
