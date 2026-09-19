#include "stdafx.h"

#include "WxHost.h"
#include "MapEditorLib/WxOwnership.h"


#include <wx/apptrait.h>
#include <wx/evtloop.h>
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
		// on startup and the layering made it one rebuild to find out that the
		// contents had nothing to do with it. (They did not: the frame was being
		// created before wxEntryStart had run. See WxHost.h.)
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

				// Every window below is owned by its parent and every sizer by the
				// window it is set on; see WxOwnership.h for where that is written
				// down and how it was checked.
				wxPanel *const pPanel = NWx::Child<wxPanel>( this );
				wxBoxSizer *const pSizer = new wxBoxSizer( wxVERTICAL );

				pSizer->Add( NWx::Child<wxStaticText>( pPanel, wxID_ANY,
					"A wxWidgets window inside the MFC editor process.\n"
					"MFC owns the message loop; wx is pumped from its idle." ),
					wxSizerFlags().Border( wxALL, 12 ) );
				pSizer->Add( NWx::Child<wxStaticLine>( pPanel ), wxSizerFlags().Expand() );

				if ( nLevel >= 3 )
				{
					// A control that only works if events are being delivered,
					// which is the half of this a screenshot cannot show.
					//
					// Kept in a named pointer and used after the Add, which is
					// legal: a sizer does not take ownership of a window and does
					// not outlive one either way.
					wxButton *const pButton = NWx::Child<wxButton>( pPanel, wxID_ANY, "Click me" );
					pSizer->Add( pButton, wxSizerFlags().Border( wxALL, 12 ) );
					pButton->Bind( wxEVT_BUTTON, &CProbeFrame::OnClicked, this );
				}

				// SetSizer takes ownership of pSizer, which is why that one is a
				// bare new: there is no parent to name it after.
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

		// Weak on purpose. wx owns the frame and destroys it when it is closed;
		// this goes null at that moment, so "is the probe still up" is a question
		// with an answer rather than a stale pointer.
		wxWeakRef<CProbeFrame> s_pProbeFrame;
	}


	void ShowProbeFrameIfAsked()
	{
		const char *pszProbe = std::getenv( "OBK2_WX_PROBE" );
		if ( pszProbe == 0 || pszProbe[0] == '0' || pszProbe[0] == '\0' )
		{
			return;
		}
		const int nLevel = std::atoi( pszProbe );
		s_pProbeFrame = NWx::TopLevel<CProbeFrame>( nLevel > 0 ? nLevel : 1 );
		s_pProbeFrame->Show( true );
	}


	bool IsProbeFrameOpen()
	{
		return s_pProbeFrame != nullptr;
	}


	namespace
	{
		SMfcHooks s_mfcHooks = { nullptr, nullptr, nullptr };


		// wx's loop, with MFC's look at each message first, in the order
		// wxMFCApp::PreTranslateMessage gave the two under MFC's loop.
		class CEditorEventLoop : public wxGUIEventLoop
		{
		public:
			virtual bool PreProcessMessage( WXMSG *pMsg ) override
			{
				if ( ( s_mfcHooks.pfnPreTranslateMessage != nullptr ) && s_mfcHooks.pfnPreTranslateMessage( s_mfcHooks.pContext, pMsg ) )
				{
					return true;
				}
				return wxGUIEventLoop::PreProcessMessage( pMsg );
			}
		};


		// The traits are where wxApp gets its main loop from.
		class CEditorAppTraits : public wxGUIAppTraits
		{
		public:
			virtual wxEventLoopBase* CreateEventLoop() override
			{
				return new CEditorEventLoop();
			}
		};
	}


	int RunWxMainLoop( const SMfcHooks &rHooks )
	{
		s_mfcHooks = rHooks;
		const int nResult = wxTheApp->OnRun();
		s_mfcHooks = SMfcHooks { nullptr, nullptr, nullptr };
		return nResult;
	}


	bool CWxHostApp::ProcessIdle()
	{
		const bool bMoreIdle = wxApp::ProcessIdle();
		if ( s_mfcHooks.pfnOnIdle != nullptr )
		{
			// The two idle calls CWinThread::Run made after a burst of messages:
			// 0 updates MFC's command UI, 1 frees its temporary window objects and
			// answers that there is nothing more.
			s_mfcHooks.pfnOnIdle( s_mfcHooks.pContext, 0 );
			s_mfcHooks.pfnOnIdle( s_mfcHooks.pContext, 1 );
		}
		return bMoreIdle;
	}


	wxAppTraits* CWxHostApp::CreateTraits()
	{
		return new CEditorAppTraits();
	}
}


// The wxApp instance. wxIMPLEMENT_APP_NO_MAIN registers the factory that
// wxEntryStart uses; it writes no WinMain, which is the point -- MFC has one,
// and CWxHostedApp::Run picks the loop.
wxIMPLEMENT_APP_NO_MAIN( NWxHost::CWxHostApp );

