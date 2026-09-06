#include "stdafx.h"

#include "SelectTablesView.h"

#ifdef OBK2_WITH_WX

#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/checklst.h>
#include <wx/dialog.h>
#include <wx/sizer.h>

// Select Tables, in wx: the first modal dialog of the migration.
//
// What this slice tests is not the dialog. It is whether a wx modal dialog can
// be run from inside MFC's message loop and behave like one, because about
// twenty of the editor's dialogs are modal and they all depend on the answer.
//
// The Win32 that answer needs is not here. It is in MapEditorLib/WxModal.h,
// once, because a migrated dialog should be pure wx plus one call -- otherwise
// twenty dialogs means twenty copies of the same SetWindowLongPtr. That header
// is scaffolding and is deleted when the frame itself is wx.

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
}


namespace NSelectTables
{
	bool RunWx( IWidget *pParent, const std::list<std::string> &rTables, CTableSet *pSelectedTables )
	{
		if ( pSelectedTables == 0 )
		{
			return false;
		}
		CSelectTablesWxDialog dialog( nullptr, rTables, *pSelectedTables );
		const bool bAccepted = ( NWxModal::ShowModalOver( &dialog, pParent ) == wxID_OK );

		if ( bAccepted )
		{
			dialog.ReadSelection( pSelectedTables );
		}
		return bAccepted;
	}
}

#endif // OBK2_WITH_WX
