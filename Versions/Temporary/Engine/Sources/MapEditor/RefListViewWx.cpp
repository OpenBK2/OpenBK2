#include "stdafx.h"

#include "RefListView.h"


#include "libdb/ResourceManager.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/WxWidget.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPlacement.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/timer.h>

// The reference list in wx: the wait while the database is scanned, and the
// list of what points at the object.
//
// Almost nothing of what these two do is here. Parsing what the scan returned,
// finding the fields that point at the target and clearing them are in
// RefListView.cpp; see RefListView.h for why. What is left is two layouts, a
// timer and a message box.

namespace
{
	// The labels CResizeDialog::GetXMLFilePath answers with, so both
	// implementations share the same Editor/ResizeDialogStyles files.
	const char *const PSZ_WAIT_STATE_NAME = "CRefListWaitDialog";
	const char *const PSZ_LIST_STATE_NAME = "CRefListDialog";


	// "Requesting info from XDBWatcher", IDD_REF_LIST_WAIT: a line of text and
	// one button, while the scan runs.
	class CRefListWaitWxDialog : public CWxToolDialog
	{
		NWxPlacement::CSizedPlacement placement { PSZ_WAIT_STATE_NAME };
		wxTimer timer;

		std::string szTargetTypeName;
		std::string szTargetName;
		std::list<std::string> *pResultList = nullptr;
		bool bComplete = false;

	public:
		CRefListWaitWxDialog( wxWindow *pParent, const std::string &rszTypeName, const std::string &rszName,
													std::list<std::string> *_pResultList )
			: CWxToolDialog( pParent, wxID_ANY, "Requesting info from XDBWatcher",
											 wxDefaultPosition, wxDefaultSize,
											 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
				szTargetTypeName( rszTypeName ), szTargetName( rszName ), pResultList( _pResultList )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( NWx::Child<wxStaticText>( this, wxID_ANY,
																						 "Waiting for XDBWatcher to finish file scan.." ),
									 wxSizerFlags().Border( wxALL, 8 ) );
			// The template's one button says Cancel and is IDOK, which is why the
			// caller asks IsComplete() rather than looking at the modal result.
			// Kept: wxID_OK with that label does the same thing.
			wxButton *const pClose = NWx::Child<wxButton>( this, wxID_OK, "Cancel" );
			pSizer->Add( pClose, wxSizerFlags().Centre().Border( wxLEFT | wxRIGHT | wxBOTTOM, 8 ) );
			SetSizerAndFit( pSizer );
			SetMinSize( GetSize() );
			placement.Restore( this );

			timer.Bind( wxEVT_TIMER, &CRefListWaitWxDialog::OnTimer, this );
		}

		bool WasPlaced() const { return placement.WasPlaced(); }
		bool IsComplete() const { return bComplete; }

		void SaveState() { placement.Save( this ); }

		// CRefListWaitDialog::OnInitDialog: ask once before the dialog is up, and
		// only start polling if the answer was not already complete. Called from
		// the runner rather than a wx event so that a scan that finishes at once
		// never shows a window at all, which is what happens in MFC.
		bool Scan()
		{
			IResourceManager *pResourceManager = Singleton<IResourceManager>();
			NI_VERIFY( pResourceManager, "Cannot find resource manager", return false )
			pResourceManager->FillReferencingObjects( &bComplete, szTargetTypeName, szTargetName,
																							 *pResultList );
			return bComplete;
		}

		void StartPolling() { timer.Start( 300 ); }

	private:
		void OnTimer( wxTimerEvent & )
		{
			if ( bComplete )
			{
				return;
			}
			if ( Scan() )
			{
				timer.Stop();
				EndModal( wxID_OK );
			}
		}
	};


	// "References", IDD_REF_LIST: the objects that point at the target over the
	// fields of whichever one is picked, and the two buttons that empty them.
	class CRefListWxDialog : public CWxToolDialog
	{
		NWxPlacement::CSizedPlacement placement { PSZ_LIST_STATE_NAME };

		wxListCtrl *pObjects = nullptr;
		wxTextCtrl *pFields = nullptr;
		wxButton *pEmptyCurrent = nullptr;

		std::string szTargetTypeName;
		std::string szTargetName;
		std::vector<NRefList::SReferenceObject> referenceObjects;
		CPtr<IManipulator> pCurrentManipulator;
		std::list<std::string> currentFields;
		long nSelectedItem = -1;

	public:
		CRefListWxDialog( wxWindow *pParent, const std::string &rszTypeName, const std::string &rszName,
											std::list<std::string> *pReferenceObjects )
			: CWxToolDialog( pParent, wxID_ANY, "References", wxDefaultPosition, wxDefaultSize,
											 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
				szTargetTypeName( rszTypeName ), szTargetName( rszName )
		{
			// The title carries the object, as OnInitDialog appends it.
			SetTitle( wxString::FromUTF8(
									( std::string( "References " ) +
										NRefList::FullName( rszTypeName, rszName ) ).c_str() ) );

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			// Both of these take half the extra height and all the extra width --
			// RESIZE_HOR | RESIZE_VER at 1.0 and 0.5 in the MFC constructor.
			// The template's LVS_SHOWSELALWAYS has no wx flag of its own; wxListCtrl
			// leaves the selection visible when the focus moves off it anyway, which
			// is what that style is for.
			pObjects = NWx::Child<wxListCtrl>( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																				 wxLC_REPORT | wxBORDER_SUNKEN );
			// One column, wider than the control will ever be, which is how the MFC
			// dialog gets a single column that never shows a second one. The
			// minimum has to be said separately, and small: wxListCtrl's best size
			// is the sum of its columns, so a 10000 pixel column asks the sizer for
			// a 10000 pixel dialog -- the first wx one came out as wide as the
			// screen. It is also what the window's minimum is taken from, and this
			// dialog has to be able to shrink to the size the MFC one was left at.
			pObjects->InsertColumn( 0, "Objects", wxLIST_FORMAT_LEFT, 10000 );
			pObjects->SetMinSize( ConvertDialogToPixels( wxSize( 100, 40 ) ) );
			// ...except that wx answers a 10000 pixel column with a horizontal
			// scrollbar, where the list view simply clips it. The column follows
			// the control's width instead, which is what the MFC one looks like.
			pObjects->Bind( wxEVT_SIZE, &CRefListWxDialog::OnObjectsResized, this );
			pSizer->Add( pObjects, wxSizerFlags( 1 ).Expand().Border( wxALL, 6 ) );
			// ES_MULTILINE | ES_READONLY, and the MFC one has no border of its own
			// but a client edge.
			pFields = NWx::Child<wxTextCtrl>( this, wxID_ANY, "No object selected.",
																				wxDefaultPosition, wxDefaultSize,
																				wxTE_MULTILINE | wxTE_READONLY | wxBORDER_SUNKEN );
			pFields->SetMinSize( ConvertDialogToPixels( wxSize( 100, 40 ) ) );
			pSizer->Add( pFields, wxSizerFlags( 1 ).Expand().Border( wxLEFT | wxRIGHT, 6 ) );

			wxBoxSizer *pButtons = new wxBoxSizer( wxHORIZONTAL );
			pEmptyCurrent = NWx::Child<wxButton>( this, wxID_ANY, "Empty current" );
			wxButton *const pEmptyAll = NWx::Child<wxButton>( this, wxID_ANY, "Empty all" );
			wxButton *const pOk = NWx::Child<wxButton>( this, wxID_OK, "OK" );
			pButtons->Add( pEmptyCurrent, wxSizerFlags().Border( wxRIGHT, 8 ) );
			pButtons->Add( pEmptyAll, wxSizerFlags().Border( wxRIGHT, 8 ) );
			pButtons->Add( pOk );
			pSizer->Add( pButtons, wxSizerFlags().Centre().Border( wxALL, 6 ) );
			SetSizerAndFit( pSizer );
			// The template's size, for a first open; the file takes over after that.
			SetSize( ConvertDialogToPixels( wxSize( 254, 239 ) ) + ( GetSize() - GetClientSize() ) );
			placement.Restore( this );

			NRefList::BuildObjects( &referenceObjects, *pReferenceObjects );
			for ( size_t i = 0; i < referenceObjects.size(); ++i )
			{
				pObjects->InsertItem( static_cast<long>( i ),
															wxString::FromUTF8( referenceObjects[i].szDisplayName.c_str() ) );
			}
			// "Move focus from Set Empty button to the Cancel button to avoid
			// accidental nullification", says the MFC dialog, and puts it on OK.
			pOk->SetFocus();
			pEmptyCurrent->Enable( false );

			pObjects->Bind( wxEVT_LIST_ITEM_SELECTED, &CRefListWxDialog::OnObjectSelected, this );
			pEmptyCurrent->Bind( wxEVT_BUTTON, &CRefListWxDialog::OnEmptyCurrent, this );
			pEmptyAll->Bind( wxEVT_BUTTON, &CRefListWxDialog::OnEmptyAll, this );
		}

		bool WasPlaced() const { return placement.WasPlaced(); }
		void SaveState() { placement.Save( this ); }

	private:
		void OnObjectsResized( wxSizeEvent &rEvent )
		{
			pObjects->SetColumnWidth( 0, pObjects->GetClientSize().x );
			rEvent.Skip();
		}

		void OnObjectSelected( wxListEvent &rEvent )
		{
			pEmptyCurrent->Enable( true );
			if ( rEvent.GetIndex() == nSelectedItem )
			{
				return;
			}
			nSelectedItem = rEvent.GetIndex();
			std::string szText;
			pCurrentManipulator = NRefList::FindFields( &currentFields, &szText,
																									referenceObjects[nSelectedItem],
																									szTargetTypeName, szTargetName );
			pFields->ChangeValue( wxString::FromUTF8( szText.c_str() ) );
		}

		void OnEmptyCurrent( wxCommandEvent & )
		{
			if ( nSelectedItem < 0 )
			{
				return;
			}
			wxBusyCursor wait;
			std::string szText = "Clearing..\r\n";
			const bool bCleared = NRefList::ClearFields( &currentFields, &szText, pCurrentManipulator );
			pFields->ChangeValue( wxString::FromUTF8( szText.c_str() ) );
			if ( !bCleared )
			{
				return;
			}
			pObjects->DeleteItem( nSelectedItem );
			referenceObjects.erase( referenceObjects.begin() + nSelectedItem );
			nSelectedItem = -1;
			pEmptyCurrent->Enable( false );
		}

		void OnEmptyAll( wxCommandEvent & )
		{
			// IDS_REF_LIST_EMPTY_ALL_LONG_TIME_WARNING, under the application's
			// title, with OK and Cancel as MB_OKCANCEL gives it.
			wxMessageDialog question(
				this, "This is a very lengthy operation. The application may not respond for a long "
							"time.\nDo you really want to continue?",
				wxString::FromUTF8(
					Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str() ),
				wxOK | wxCANCEL | wxICON_QUESTION );
			if ( question.ShowModal() != wxID_OK )
			{
				return;
			}
			wxBusyCursor wait;
			pCurrentManipulator = 0;
			nSelectedItem = -1;
			if ( !NRefList::ClearAll( referenceObjects, szTargetTypeName, szTargetName ) )
			{
				// IDS_REF_LIST_SET_EMPTY_FAILURE
				wxMessageDialog failure(
					this, "Nullification of some references have failed.",
					wxString::FromUTF8(
						Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str() ),
					wxOK | wxICON_WARNING );
				failure.ShowModal();
			}
			// The MFC dialog closes itself when Empty all is done.
			EndModal( wxID_OK );
		}
	};
}


namespace NRefList
{
	bool RunScan( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
									std::list<std::string> *pReferenceObjects )
	{
		if ( pReferenceObjects == 0 )
		{
			return false;
		}
		CRefListWaitWxDialog dialog( ToWxOwnerWindow( pParent ), rszTypeName, rszName, pReferenceObjects );
		// A scan that is already finished closes the MFC dialog from its
		// OnInitDialog, before it is ever seen; this simply never shows it.
		if ( dialog.Scan() )
		{
			return true;
		}
		dialog.StartPolling();
		if ( !dialog.WasPlaced() )
		{
			dialog.CentreOnParent();
		}
		dialog.ShowModal();
		dialog.SaveState();
		return dialog.IsComplete();
	}


	void Run( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
							std::list<std::string> *pReferenceObjects )
	{
		if ( pReferenceObjects == 0 )
		{
			return;
		}
		CRefListWxDialog dialog( ToWxOwnerWindow( pParent ), rszTypeName, rszName, pReferenceObjects );
		if ( !dialog.WasPlaced() )
		{
			dialog.CentreOnParent();
		}
		dialog.ShowModal();
		dialog.SaveState();
	}
}

