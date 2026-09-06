#include "stdafx.h"

#include "WxHost.h"

#ifdef OBK2_WITH_WX

#include <wx/statline.h>

#include <cstdlib>

namespace NWxHost
{
	namespace
	{
		// The probe frame, built in layers so that OBK2_WX_PROBE selects how much
		// of it exists:
		//
		//   1  bare frame and a status bar
		//   2  + a panel with static text on it
		//   3  + a button
		//
		// It is built this way because the first version, which was level 3, died
		// with an access violation in wxButton::SetDefaultStyle underneath
		// wxTopLevelWindowMSW::DoRestoreLastFocus, on the WM_ACTIVATE that
		// arrives through mfc140!_AfxActivationWndProc. Bisecting by rebuilding
		// three times would have cost three full editor builds; this costs one
		// and the level is set from outside.
		class CProbeFrame : public wxFrame
		{
		public:
			explicit CProbeFrame( int nLevel )
				: wxFrame( nullptr, wxID_ANY,
									 wxString::Format( "wx probe L%d in B2_MapEditor - %s",
																		 nLevel, wxVERSION_STRING ),
									 wxPoint( 80, 80 ), wxSize( 470, 210 ) )
			{
				CreateStatusBar();
				SetStatusText( wxString::Format( "level %d", nLevel ) );

				if ( nLevel < 2 )
				{
					return;
				}

				wxPanel *pPanel = new wxPanel( this );
				wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
				pSizer->Add( new wxStaticText( pPanel, wxID_ANY,
					"A wxWidgets window inside the MFC editor process.\n"
					"MFC owns the message loop; wx is pumped from its idle." ),
					wxSizerFlags().Border( wxALL, 12 ) );
				pSizer->Add( new wxStaticLine( pPanel ), wxSizerFlags().Expand() );

				if ( nLevel >= 3 )
				{
					// A control that only works if events are being delivered,
					// which is the half of this a screenshot cannot show.
					wxButton *pButton = new wxButton( pPanel, wxID_ANY, "Click me" );
					pButton->Bind( wxEVT_BUTTON, &CProbeFrame::OnClicked, this );
					pSizer->Add( pButton, wxSizerFlags().Border( wxALL, 12 ) );
				}

				pPanel->SetSizer( pSizer );
			}

		private:
			void OnClicked( wxCommandEvent& )
			{
				++nClicks;
				SetStatusText( wxString::Format( "clicked %d time(s) - events reach wx", nClicks ) );
			}

			int nClicks = 0;
		};
	}


	void ShowProbeFrameIfAsked()
	{
		const char *pszProbe = std::getenv( "OBK2_WX_PROBE" );
		if ( pszProbe == 0 || pszProbe[0] == '0' || pszProbe[0] == '\0' )
		{
			return;
		}
		const int nLevel = std::atoi( pszProbe );
		// Owned by wx once shown: a wxFrame deletes itself when destroyed.
		( new CProbeFrame( nLevel > 0 ? nLevel : 1 ) )->Show( true );
	}
}


// The wxApp instance. wxIMPLEMENT_APP_NO_MAIN registers the factory that
// wxEntryStart uses; it writes no WinMain, which is the point -- MFC has one.
wxIMPLEMENT_APP_NO_MAIN( NWxHost::CWxHostApp );

#endif // OBK2_WITH_WX
