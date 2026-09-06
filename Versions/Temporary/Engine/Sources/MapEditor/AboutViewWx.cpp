#include "stdafx.h"

#include "AboutView.h"

#ifdef OBK2_WITH_WX

#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/WxImage.h"
#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

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

			// Not wrapped. The template reserves two lines for this string, 182x25
			// dialog units, but with the shell font the editor actually applies it
			// fits on one -- checked against the MFC dialog on screen. Wrapping it
			// at the template's width would put a break where the original has
			// none, and make the dialog narrower than the one it replaces for no
			// reason.
			pText->Add( NWx::Child<wxStaticText>(
											this, wxID_ANY,
											wxString::FromUTF8( "Copyright \xC2\xA9 2003 Nival Interactive."
																					" All rights reserved." ) ),
									wxSizerFlags().Border( wxTOP, 8 ) );

			pTop->Add( pText, wxSizerFlags( 1 ).Expand().Border( wxTOP | wxRIGHT, 10 ) );
			pSizer->Add( pTop, wxSizerFlags().Expand() );

			pSizer->Add( CreateStdDialogButtonSizer( wxOK ),
									 wxSizerFlags().Centre().Border( wxALL, 10 ) );
			SetSizerAndFit( pSizer );
			Centre();
		}

	private:
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
	void RunWx( IWidget *pParent )
	{
		CAboutWxDialog dialog( nullptr );
		NWxModal::ShowModalOver( &dialog, pParent );
	}
}

#endif // OBK2_WITH_WX
