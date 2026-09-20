#include "stdafx.h"

#include "ProgressView.h"


#include "MapEditorLib/WxWidget.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/gauge.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timer.h>

// The progress dialog in wx: a label over a bar, shown half a second after it
// is created and repainted by hand whenever it changes.
//
// The bar is a plain one. IDD_PROGRESS_SIMPLE's is an msctls_progress32 behind
// MFC's CProgressCtrl -- there is no Stingray anywhere in this dialog -- so
// wxGauge is the same control with a different wrapper.
//
// The two things that are not obvious are in ProgressView.h: the half second
// before it appears, and why every setter repaints. The second is the reason
// this file paints its parent too: the frame behind it will not paint itself
// while the thread that owns it is busy.

namespace
{
	class CProgressWxDialog : public CWxToolDialog
	{
		wxStaticText *pLabel = nullptr;
		wxGauge *pBar = nullptr;
		wxTimer showTimer;

	public:
		// START_TIMER_INTERVAL, which is how long the editor may be busy before
		// the user is told about it.
		static const int START_TIMER_INTERVAL = 500;

		// The owner is taken as the wx parent rather than kept as a handle of our
		// own: this window lives on through every progress update after Create
		// returns, and an IWidget is borrowed -- holding one is what crashed
		// opening an object from the recent list, through a widget that had left
		// the stack. wx keeps the parent, and GetParent answers with it.
		explicit CProgressWxDialog( wxWindow *pOwner )
			: CWxToolDialog( pOwner, wxID_ANY, "Progress" )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			// SS_LEFTNOWORDWRAP: one line, clipped rather than wrapped.
			pLabel = NWx::Child<wxStaticText>( this, wxID_ANY, wxString(), wxDefaultPosition,
																				 wxDefaultSize, wxST_ELLIPSIZE_END );
			pSizer->Add( pLabel, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT | wxTOP, 8 ) );
			pBar = NWx::Child<wxGauge>( this, wxID_ANY, 100, wxDefaultPosition,
																	wxSize( -1, ConvertDialogToPixels( wxSize( 0, 14 ) ).y ),
																	wxGA_HORIZONTAL | wxBORDER_SIMPLE );
			pSizer->Add( pBar, wxSizerFlags().Expand().Border( wxALL, 8 ) );
			SetSizerAndFit( pSizer );
			// The template's width, which is what makes room for the long resource
			// names this shows.
			SetSize( wxSize( ConvertDialogToPixels( wxSize( 296, 0 ) ).x, GetSize().y ) );
			// DS_CENTER: the dialog manager centres this one on the screen, not on
			// the frame, and that is where the MFC one appears. Spelled out because
			// Centre() centres on the parent now that this dialog has one.
			CentreOnScreen();

			showTimer.Bind( wxEVT_TIMER, &CProgressWxDialog::OnShowTimer, this );
			showTimer.StartOnce( START_TIMER_INTERVAL );
		}

		// CProgressDialog::UpdateControls: this window and the frame behind it,
		// painted now, because nothing else is going to paint them.
		void UpdateControls()
		{
			Update();
			if ( wxWindow *const pOwner = GetParent() )
			{
				pOwner->Update();
			}
		}

		void SetProgressTitle( const std::string &rszTitle )
		{
			SetTitle( wxString::FromUTF8( rszTitle.c_str() ) );
			UpdateControls();
		}

		void SetProgressMessage( const std::string &rszMessage )
		{
			pLabel->SetLabelText( wxString::FromUTF8( rszMessage.c_str() ) );
			UpdateControls();
		}

		void SetProgressRange( int nStart, int nFinish )
		{
			// wxGauge counts from zero, where the progress control takes both ends;
			// the offset is kept here so the positions the editor sends still mean
			// what they meant.
			nRangeStart = nStart;
			pBar->SetRange( ( nFinish > nStart ) ? ( nFinish - nStart ) : 1 );
			UpdateControls();
		}

		void SetProgressPosition( int nPosition )
		{
			pBar->SetValue( Clamp( nPosition - nRangeStart, 0, pBar->GetRange() ) );
			UpdateControls();
		}

		void IterateProgressPosition()
		{
			const int nNewValue = pBar->GetValue() + 1;
			pBar->SetValue( ( nNewValue > pBar->GetRange() ) ? 0 : nNewValue );
			UpdateControls();
		}

		void ShowNow()
		{
			Show( true );
			UpdateControls();
		}

	private:
		int nRangeStart = 0;

		void OnShowTimer( wxTimerEvent & )
		{
			ShowNow();
		}
	};


	class CWxProgressView : public NProgressView::IView
	{
		wxWeakRef<CProgressWxDialog> dialog;

	public:
		virtual ~CWxProgressView()
		{
			Destroy();
		}

		// pParent is only used during this call: the window it resolves to
		// outlives it and wx keeps that as the dialog's parent.
		virtual bool Create( IWidget *pParent )
		{
			Destroy();
			dialog = NWx::TopLevel<CProgressWxDialog>( ToWxOwnerWindow( pParent ) );
			if ( !dialog )
			{
				return false;
			}
			return true;
		}

		virtual bool IsCreated() const
		{
			return dialog != nullptr;
		}

		virtual void Show()
		{
			if ( dialog )
			{
				dialog->ShowNow();
			}
		}

		virtual void Destroy()
		{
			if ( dialog )
			{
				dialog->Destroy();
				dialog = nullptr;
			}
		}

		virtual void SetTitle( const std::string &rszTitle )
		{
			if ( dialog )
			{
				dialog->SetProgressTitle( rszTitle );
			}
		}

		virtual void SetMessage( const std::string &rszMessage )
		{
			if ( dialog )
			{
				dialog->SetProgressMessage( rszMessage );
			}
		}

		virtual void SetRange( int nStart, int nFinish )
		{
			if ( dialog )
			{
				dialog->SetProgressRange( nStart, nFinish );
			}
		}

		virtual void SetPosition( int nPosition )
		{
			if ( dialog )
			{
				dialog->SetProgressPosition( nPosition );
			}
		}

		virtual void IteratePosition()
		{
			if ( dialog )
			{
				dialog->IterateProgressPosition();
			}
		}
	};
}


namespace NProgressView
{
	IView* Create()
	{
		return new CWxProgressView;
	}
}

