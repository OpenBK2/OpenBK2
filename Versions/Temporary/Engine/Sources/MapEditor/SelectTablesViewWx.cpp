#include "stdafx.h"

#include "SelectTablesView.h"

#ifdef OBK2_WITH_WX

#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/checklst.h>
#include <wx/dialog.h>
#include <wx/sizer.h>

// Select Tables, in wx: the first modal dialog of the migration.
//
// What this slice is really testing is not the dialog. It is whether a wx modal
// dialog can be run from inside MFC's message loop and behave like a modal
// dialog -- because roughly twenty of the editor's dialogs are modal and they
// all depend on the answer.
//
// Modality has to be arranged by hand, and the reason is worth stating. A wx
// dialog is normally made modal by giving it a wx parent, and there is no wx
// window for the editor's MFC frame. The obvious fix -- wrapping the frame in a
// wxNativeContainerWindow -- is the mistake the Log Window already made once:
// adopting a window means subclassing it, and subclassing the main frame would
// be far worse than losing a pane's caption. So the two things a modal dialog
// needs are done explicitly:
//
//   * the owner is disabled for the duration, which is what stops it taking
//     input and is exactly what MFC's own CWnd::RunModalLoop does;
//   * GWLP_HWNDPARENT is set to the owner, which is what makes Windows keep the
//     dialog above it and hand activation back when it closes.
//
// Both are undone before returning, in the opposite order, including on the
// cancel path.

namespace
{
	// The anchors from CSelectTablesDialog's constructor, translated:
	//
	//   IDC_CT_TABLES_LIST  ANCHORE_LEFT_TOP | RESIZE_HOR_VER  -> proportion 1, wxEXPAND
	//   IDOK, IDCANCEL      ANCHORE_RIGHT_BOTTOM                -> a bottom row, right aligned
	//
	// That is the whole of this dialog's layout, and it is the smallest real
	// instance of the CResizeDialog-anchor to wx-sizer mapping that the .rc
	// converter will have to do 49 times.
	class CSelectTablesWxDialog : public wxDialog
	{
		wxCheckListBox *pTablesList = nullptr;

	public:
		CSelectTablesWxDialog( wxWindow *pParent,
													 const std::list<std::string> &rTables,
													 const CTableSet &rSelected )
			: wxDialog( pParent, wxID_ANY, "Select tables",
									wxDefaultPosition, wxSize( 260, 300 ),
									wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
		{
			pTablesList = NWx::Child<wxCheckListBox>( this, wxID_ANY );

			// Order matters: the MFC one adds in the order of `tables` and the
			// downstream CreateTabs walks the set, so keeping the order keeps the
			// two comparable line for line.
			for ( std::list<std::string>::const_iterator itTable = rTables.begin();
						itTable != rTables.end(); ++itTable )
			{
				const int nIndex = pTablesList->Append( wxString::FromUTF8( itTable->c_str() ) );
				pTablesList->Check( nIndex, rSelected.find( *itTable ) != rSelected.end() );
			}

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pTablesList, wxSizerFlags( 1 ).Expand().Border( wxALL, 6 ) );
			// CreateStdDialogButtonSizer gives the platform's own OK/Cancel order,
			// which is one of the things a toolkit is for.
			pSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ),
									 wxSizerFlags().Right().Border( wxALL, 6 ) );
			SetSizer( pSizer );
			SetMinSize( wxSize( 204, 106 ) );		// GetMinimumXDimension/YDimension
		}

		void ReadSelection( CTableSet *pSelectedTables ) const
		{
			pSelectedTables->clear();
			const unsigned nCount = pTablesList->GetCount();
			for ( unsigned nIndex = 0; nIndex < nCount; ++nIndex )
			{
				if ( pTablesList->IsChecked( nIndex ) )
				{
					InsertHashSetElement( pSelectedTables,
																std::string( pTablesList->GetString( nIndex ).utf8_str() ) );
				}
			}
		}
	};


	// Disables the owner and owns it back. RAII rather than a pair of calls
	// because the cancel path and any exception have to undo it too.
	class CModalOwner
	{
		HWND hwndOwner;
		bool bWasEnabled;

	public:
		explicit CModalOwner( HWND _hwndOwner )
			: hwndOwner( _hwndOwner ), bWasEnabled( false )
		{
			if ( hwndOwner != 0 )
			{
				bWasEnabled = ::IsWindowEnabled( hwndOwner ) != FALSE;
				::EnableWindow( hwndOwner, FALSE );
			}
		}

		~CModalOwner()
		{
			if ( hwndOwner != 0 && bWasEnabled )
			{
				::EnableWindow( hwndOwner, TRUE );
			}
		}
	};
}


namespace NSelectTables
{
	bool RunWx( IWidget *pParent, const std::list<std::string> &rTables, CTableSet *pSelectedTables )
	{
		if ( pSelectedTables == 0 )
		{
			return false;
		}
		// The top-level window, not the one handed in. The caller passes whatever
		// it has -- here a docking pane -- and disabling that would leave the rest
		// of the frame live, which is not modal. MFC does the same walk in
		// CWnd::GetSafeOwner before CDialog::DoModal disables anything, and this
		// has to match it or the wx dialogs behave differently from the MFC ones
		// for no reason a user could understand.
		CWnd *const pwndParent = ToCWnd( pParent );
		HWND hwndOwner = pwndParent != 0 ? pwndParent->GetSafeHwnd() : 0;
		if ( hwndOwner != 0 )
		{
			hwndOwner = ::GetAncestor( hwndOwner, GA_ROOT );
		}

		CSelectTablesWxDialog dialog( nullptr, rTables, *pSelectedTables );

		if ( hwndOwner != 0 )
		{
			// Windows keeps an owned window above its owner and returns activation
			// to it on close. Set before the disable so the dialog is never
			// ownerless while visible.
			::SetWindowLongPtr( (HWND)dialog.GetHandle(), GWLP_HWNDPARENT, (LONG_PTR)hwndOwner );
		}

		bool bAccepted = false;
		{
			CModalOwner modalOwner( hwndOwner );
			bAccepted = ( dialog.ShowModal() == wxID_OK );
		}

		if ( bAccepted )
		{
			dialog.ReadSelection( pSelectedTables );
		}
		return bAccepted;
	}
}

#endif // OBK2_WITH_WX
