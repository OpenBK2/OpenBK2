#include "stdafx.h"

#include "AboutView.h"


#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/WxImage.h"
#include "MapEditorLib/WxWidget.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxToolDialog.h"

// Generated at build time by cmake/gitrevision.cmake (add_git_revision).
#include "GitRevision.h"

#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/platinfo.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/version.h>

// About, in wx. The fourth dialog, and the first that needs a picture.
//
// **That is the point of it.** Every graphic this editor has is a .ico or a
// .bmp compiled into MapEditor.dll's resource section and reached by a numeric
// id -- IDR_MAINFRAME is 129 -- and that is the one form wx has no portable way
// to load: wxBitmapBundle::FromResources and wxICON both take a name, and
// neither exists off Windows. So the icon is read from a file and decoded by
// NWinImage, exactly the way the game's window icon already is off Windows.
// See MapEditorLib/WxImage.h; main_frame.ico is installed beside the editor
// binary for it.
//
// The bundle carries every size the .ico holds rather than one chosen here,
// which is what the container is for and what lets wx pick per display scale.
//
// Below the MFC dialog's contents, and not in it, a details box for bug
// reports: which build this is (revision, CI build number, configuration,
// architecture), which wx it runs on (what WxEditor's CollectDetails reports),
// and the session it is (the MOD, the code page, the display scale). Read-only
// text rather than labels so any of it can be selected, and a Copy button for
// all of it.

namespace
{
	// Installed by MapEditor/CMakeLists.txt beside the executable. The same file
	// the .rc compiles in as IDR_MAINFRAME, so the two implementations show the
	// same picture rather than two drawings that happen to look alike.
	const char *const PSZ_ICON_FILE = "main_frame.ico";

	// What the MFC dialog draws: IDD_ABOUT gives the icon 21x20 dialog units,
	// which the dialog manager renders as the 32x32 entry. Measured on the
	// running editor, not read off the template.
	const int N_ICON_SIZE = 32;

	// The details box's width in characters: wide enough for the longest line,
	// the revision with its branch and date. Its height follows its lines.
	const int N_DETAILS_COLUMNS = 72;


	const char* ConfigurationName()
	{
#if defined( _FINALRELEASE )
		return "final release";
#elif defined( _DEBUG )
		return "debug";
#else
		return "release";
#endif
	}


	// The instruction set the compiler was told it may use (-DARCHITECTURE),
	// highest first.
	const char* InstructionSetName()
	{
#if defined( __AVX512F__ )
		return "AVX512";
#elif defined( __AVX2__ )
		return "AVX2";
#elif defined( __AVX__ )
		return "AVX";
#elif defined( _M_X64 ) || ( defined( _M_IX86_FP ) && ( _M_IX86_FP >= 2 ) )
		return "SSE2";
#else
		return "default";
#endif
	}


	// Everything a bug report should say about the editor it came from, one
	// "name: value" per line.
	wxString CollectDetails( const wxWindow *pWindow )
	{
		const SUserData *const pUserData = Singleton<IUserDataContainer>()->Get();
		wxString strDetails;
		strDetails << "Version       : " << wxString::FromUTF8( pUserData->constUserData.szVersion.c_str() ) << "\n";
		strDetails << "Revision      : " << GIT_REVISION_STR << " (" << GIT_BRANCH_STR << ", " << GIT_COMMIT_DATE_STR << ")\n";
		// CI numbers its builds and stamps their time; a local build is 0 and
		// has no stamp (cmake/versioninfo.cmake), so the compile time stands in.
		strDetails << "Build         : " << REVISION_NUMBER_STR;
		if ( ( BUILD_DATE_TIME_STR[0] != 0 ) && ( std::string( BUILD_DATE_TIME_STR ) != "1970-01-01 00:00:00" ) )
		{
			strDetails << ", built " << BUILD_DATE_TIME_STR;
		}
		else
		{
			strDetails << ", local build, compiled " << __DATE__ << " " << __TIME__;
		}
		strDetails << "\n";
		strDetails << "Configuration : " << ConfigurationName() << ", " << ( sizeof( void* ) * 8 ) << "-bit, " << InstructionSetName() << "\n";
		strDetails << "Compiler      : MSVC " << _MSC_FULL_VER << "\n";
		strDetails << "MOD           : " << ( pUserData->szOpenedMODFolder.empty() ? wxString( "none" ) : wxString::FromUTF8( pUserData->szOpenedMODFolder.c_str() ) ) << "\n";
		strDetails << "wx (compiled) : " << wxVERSION_STRING << ", debug level " << wxDEBUG_LEVEL << "\n";
		// The DLL actually loaded, which is what can differ from the line above.
		strDetails << "wx (running)  : " << wxGetLibraryVersionInfo().GetVersionString() << "\n";
		strDetails << "wx port       : " << wxPlatformInfo::Get().GetPortIdName() << ", Unicode " << wxUSE_UNICODE << ", sizeof(wxChar) " << static_cast<int>( sizeof( wxChar ) ) << "\n";
		strDetails << "OS            : " << wxGetOsDescription() << "\n";
		// The narrow strings are UTF-8 only where the manifest's code page took
		// (Windows 10 1903 and later); anywhere else 65001 is missing here.
		strDetails << "ANSI code page: " << static_cast<unsigned>( ::GetACP() ) << "\n";
		strDetails << "Display scale : " << wxString::Format( "%.2f", pWindow->GetDPIScaleFactor() ) << "\n";
		return strDetails;
	}

	class CAboutWxDialog : public CWxToolDialog
	{
	public:
		explicit CAboutWxDialog( wxWindow *pParent )
			// IDD_ABOUT has no WS_THICKFRAME, so no wxRESIZE_BORDER. The caption
			// is built the way OnInitDialog builds it.
			: CWxToolDialog( pParent, wxID_ANY,
											 wxString::FromUTF8( ( "About " + Title() ).c_str() ) )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			wxBoxSizer *pTop = new wxBoxSizer( wxHORIZONTAL );

			// An empty bundle draws nothing and costs nothing, which is the right
			// answer if the file is missing: an About box without its icon still
			// tells you the version.
			pTop->Add( NWx::Child<wxStaticBitmap>( this, wxID_ANY,
																						 NWxImage::LoadIcon( PSZ_ICON_FILE, N_ICON_SIZE ) ),
								 wxSizerFlags().Border( wxALL, 10 ) );

			wxBoxSizer *pText = new wxBoxSizer( wxVERTICAL );
			pText->Add( NWx::Child<wxStaticText>( this, wxID_ANY,
																						wxString::FromUTF8( Title().c_str() ) ) );

			// Two statics, as the template has: "Version" and the number beside
			// it, rather than one string with the word baked in.
			wxBoxSizer *pVersion = new wxBoxSizer( wxHORIZONTAL );
			pVersion->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Version" ) );
			pVersion->Add( NWx::Child<wxStaticText>( this, wxID_ANY,
																							 wxString::FromUTF8( Version().c_str() ) ),
										 wxSizerFlags().Border( wxLEFT, 6 ) );
			pText->Add( pVersion, wxSizerFlags().Border( wxTOP, 6 ) );

			// The copyright line as the MFC dialog has it, then the port's credit on
			// a line of its own. Explicit breaks, and no space after them: a space
			// after a break is drawn as an indent.
			pText->Add( NWx::Child<wxStaticText>(
											this, wxID_ANY,
											wxString::FromUTF8( "Copyright \xC2\xA9 2003 Nival Interactive. All rights reserved.\n"
																					"Open Source port by the OpenBK2 team." ) ),
									wxSizerFlags().Border( wxTOP, 8 ) );

			pTop->Add( pText, wxSizerFlags( 1 ).Expand().Border( wxTOP | wxRIGHT, 10 ) );
			pSizer->Add( pTop, wxSizerFlags().Expand() );

			// The details, in a fixed-width font so the names line up.
			pSizer->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Details for bug reports:" ),
									 wxSizerFlags().Border( wxLEFT | wxRIGHT | wxTOP, 10 ) );
			wxTextCtrl *const pDetails = NWx::Child<wxTextCtrl>( this, wxID_ANY, CollectDetails( this ), wxDefaultPosition, wxDefaultSize,
																													 wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxHSCROLL );
			pDetails->SetFont( wxFont( wxFontInfo( GetFont().GetPointSize() ).Family( wxFONTFAMILY_TELETYPE ) ) );
			// Every line shown without scrolling: the lines' height in the box's
			// own font, one spare line, and the horizontal scroll bar's height.
			const wxSize lineSize = pDetails->GetTextExtent( wxString( 'M', N_DETAILS_COLUMNS ) );
			const int nLines = pDetails->GetNumberOfLines() + 1;
			pDetails->SetMinSize( wxSize( pDetails->GetSizeFromTextSize( lineSize.x ).x,
																		lineSize.y * nLines + wxSystemSettings::GetMetric( wxSYS_HSCROLL_Y, this ) + FromDIP( 8 ) ) );
			pSizer->Add( pDetails, wxSizerFlags( 1 ).Expand().Border( wxLEFT | wxRIGHT | wxTOP, 10 ) );

			// Copy on the left, OK where the MFC dialog has it.
			wxBoxSizer *pButtons = new wxBoxSizer( wxHORIZONTAL );
			wxButton *const pCopy = NWx::Child<wxButton>( this, wxID_COPY, "&Copy details" );
			pCopy->Bind( wxEVT_BUTTON, [pDetails]( wxCommandEvent & ) { CopyToClipboard( pDetails->GetValue() ); } );
			pButtons->Add( pCopy );
			pButtons->AddStretchSpacer();
			pButtons->Add( CreateStdDialogButtonSizer( wxOK ) );
			pSizer->Add( pButtons, wxSizerFlags().Expand().Border( wxALL, 10 ) );
			SetSizerAndFit( pSizer );
			// OK takes Enter, as in the MFC dialog, rather than the first button.
			if ( wxWindow *const pOK = FindWindow( wxID_OK ) )
			{
				pOK->SetFocus();
			}
		}

	private:
		static void CopyToClipboard( const wxString &rText )
		{
			wxClipboardLocker locker;
			if ( !!locker )
			{
				wxTheClipboard->SetData( new wxTextDataObject( rText ) );
			}
		}

		static const std::string& Title()
		{
			return Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle;
		}

		static const std::string& Version()
		{
			return Singleton<IUserDataContainer>()->Get()->constUserData.szVersion;
		}
	};
}


namespace NAbout
{
	void Run( IWidget *pParent )
	{
		CAboutWxDialog dialog( ToWxOwnerWindow( pParent ) );
		dialog.CentreOnParent();
		dialog.ShowModal();
	}
}

