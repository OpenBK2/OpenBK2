#include "stdafx.h"

#include "DBLinkView.h"


#include "ObjectBrowserView.h"
#include "ControllerContainer.h"
#include "PC_BaseDialog.h"
#include "PC_Constants.h"
#include "PropertyPaneView.h"
#include "ResourceDefines.h"
#include "Tree_GDBBrowserBase_Constants.h"
#include "WMDefines.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/Interface_View.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/WxModal.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPlacement.h"
#include "MapEditorLib/WxMfcOwnerDialog.h"
#include "libdb/ResourceManager.h"

#include <fmt/format.h>
#include <fmt/printf.h>

#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <memory>

// The link picker in wx: IDD_PC_DB_LINK's controls around the wx database
// browser's contents, made with trees that pick rather than open, and the wx
// property grid the Selection Properties pane uses.
//
// As in CPCDBLinkDialog, the picker is CHID_PC_DB_LINK_DIALOG while it is up
// and every table's tree is told to hand its selection there; the picker
// answers as a CPCBaseDialog, with the grid. A double click on an object is OK.
//
// Like the build data dialog, it is the owner of what its grid's buttons and
// its trees' commands open: a CWxMfcOwnerDialog.

namespace
{
	// CPCDBLinkDialog::GetXMLFilePath's label, shared with the MFC dialog.
	const char *const PSZ_STATE_NAME = "CPCDBLinkDialog";
	const int N_COLUMN_COUNT = 3;


	std::string LoadResourceString( UINT nID )
	{
		CString strText;
		strText.LoadString( nID );
		return std::string( strText.GetString() );
	}


	wxString FromNarrow( const std::string &rszText )
	{
		return wxString::FromUTF8( rszText.c_str(), rszText.size() );
	}


	class CDBLinkWxDialog : public CWxMfcOwnerDialog, public CPCBaseDialog, public IObjectBrowser::IListener
	{
		NWxPlacement::CSizedPlacement placement { PSZ_STATE_NAME };
		// Members, so both go before the base's owner; and the browser after the
		// grid, so its trees, which hand their selection to the grid, go first.
		std::unique_ptr<NPropertyPane::IGrid> pGrid;
		std::unique_ptr<IObjectBrowser> pBrowser;

		wxStaticText *pCurrentSelection = nullptr;
		wxStaticText *pPreviousSelection = nullptr;

		const NDBLink::SRequest request;
		std::string szCurrentTable;
		std::string szCurrentObject;
		bool bEmpty = false;
		// Set while the tables are added, when choosing one is not yet the user's.
		bool bCreateControls = true;
		ICommandHandler *pPreviousCommandHandler = nullptr;
		bool bRegistered = false;

	public:
		explicit CDBLinkWxDialog( const NDBLink::SRequest &rRequest )
			: CWxMfcOwnerDialog( nullptr, wxID_ANY, "Game Data Base Link", wxDefaultPosition, wxDefaultSize,
											 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
				request( rRequest ), szCurrentTable( rRequest.szTable ), szCurrentObject( rRequest.szObject )
		{
			// IDD_PC_DB_LINK: "Objects:" over the tables on the left, "Properties:"
			// over the grid and its status line on the right, the two selections
			// under both, Set Empty on the left of the last line and OK and Cancel
			// on its right.
			const int nGap = FromDIP( 4 );
			const int nBorder = FromDIP( 8 );
			const wxSizerFlags labelFlags = wxSizerFlags().Border( wxBOTTOM, FromDIP( 2 ) );

			wxStaticText *const pTreeStatus = NWx::Child<wxStaticText>( this, wxID_ANY, wxString(), wxDefaultPosition,
																																	wxSize( -1, FromDIP( 18 ) ),
																																	wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END | wxBORDER_SUNKEN );
			pGrid.reset( NPropertyPane::CreateGridWx( this, pTreeStatus, GetMfcOwner(), PSZ_STATE_NAME ) );
			wxWindow *pBrowserWindow = nullptr;
			pBrowser.reset( NObjectBrowser::CreateWxIn( this, GetMfcOwner(), this, IObjectBrowser::KIND_LINK, &pBrowserWindow ) );

			wxBoxSizer *const pLeft = new wxBoxSizer( wxVERTICAL );
			pLeft->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Objects:" ), labelFlags );
			pLeft->Add( pBrowserWindow, wxSizerFlags( 1 ).Expand() );

			wxBoxSizer *const pRight = new wxBoxSizer( wxVERTICAL );
			pRight->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Properties:" ), labelFlags );
			pRight->Add( pGrid->GetWindow(), wxSizerFlags( 1 ).Expand() );
			pRight->Add( pTreeStatus, wxSizerFlags().Expand().Border( wxTOP, FromDIP( 1 ) ) );

			// A fixed width keeps the list that wide as the picker grows, as
			// CPCDBLinkDialog anchored it; otherwise the two halves share the width.
			wxBoxSizer *const pColumns = new wxBoxSizer( wxHORIZONTAL );
			if ( request.nFixedWidth > 0 )
			{
				pBrowserWindow->SetMinSize( wxSize( request.nFixedWidth, -1 ) );
				pColumns->Add( pLeft, wxSizerFlags().Expand().Border( wxRIGHT, nGap ) );
				pColumns->Add( pRight, wxSizerFlags( 1 ).Expand() );
			}
			else
			{
				pColumns->Add( pLeft, wxSizerFlags( 1 ).Expand().Border( wxRIGHT, nGap ) );
				pColumns->Add( pRight, wxSizerFlags( 1 ).Expand() );
			}

			const bool bLink = ( request.eType == NDBLink::TYPE_LINK );
			// IDC_PC_DBL_CUR_SEL_LABEL_LEFT is 62 dialog units wide, and its value
			// starts 3 units after it.
			wxFlexGridSizer *const pSelections = new wxFlexGridSizer( 2, wxSize( ConvertDialogToPixels( wxSize( 3, 0 ) ).x, FromDIP( 3 ) ) );
			pSelections->AddGrowableCol( 1 );
			wxStaticText *const pCurrentLabel = NWx::Child<wxStaticText>( this, wxID_ANY, "Current Selection:" );
			pCurrentLabel->SetMinSize( wxSize( ConvertDialogToPixels( wxSize( 62, 0 ) ).x, -1 ) );
			pSelections->Add( pCurrentLabel );
			pCurrentSelection = NWx::Child<wxStaticText>( this, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize,
																										wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END );
			pSelections->Add( pCurrentSelection, wxSizerFlags().Expand() );
			wxStaticText *const pPreviousLabel = NWx::Child<wxStaticText>( this, wxID_ANY, "Previous Selection:" );
			pSelections->Add( pPreviousLabel );
			pPreviousSelection = NWx::Child<wxStaticText>( this, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize,
																										 wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END );
			pSelections->Add( pPreviousSelection, wxSizerFlags().Expand() );
			// Opening an object has no previous selection to show.
			pPreviousLabel->Show( bLink );
			pPreviousSelection->Show( bLink );

			wxBoxSizer *const pBottomRow = new wxBoxSizer( wxHORIZONTAL );
			wxButton *const pSetEmpty = NWx::Child<wxButton>( this, wxID_ANY, "Set Empty" );
			pSetEmpty->Show( bLink );
			pBottomRow->Add( pSetEmpty, wxSizerFlags().CentreVertical() );
			pBottomRow->AddStretchSpacer();
			pBottomRow->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ), wxSizerFlags().CentreVertical() );
			// The template's buttons are 50 by 14 dialog units, larger than wx's
			// standard ones.
			const wxSize buttonSize = ConvertDialogToPixels( wxSize( 50, 14 ) );
			for ( wxWindow *pButton : { FindWindow( wxID_OK ), FindWindow( wxID_CANCEL ), static_cast<wxWindow*>( pSetEmpty ) } )
			{
				if ( pButton != nullptr )
				{
					pButton->SetMinSize( buttonSize );
				}
			}

			wxBoxSizer *const pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pColumns, wxSizerFlags( 1 ).Expand().Border( wxLEFT | wxRIGHT | wxTOP, nBorder ) );
			pSizer->Add( pSelections, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT | wxTOP, nBorder ) );
			pSizer->Add( pBottomRow, wxSizerFlags().Expand().Border( wxALL, nBorder ) );
			SetSizerAndFit( pSizer );
			// CPCDBLinkDialog's GetMinimumXDimension and GetMinimumYDimension.
			SetMinSize( wxSize( 400, 300 ) );
			// The template's size for a first open, then wherever it was left.
			SetSize( ConvertDialogToPixels( wxSize( 512, 379 ) ) + ( GetSize() - GetClientSize() ) );
			placement.Restore( this );

			pSetEmpty->Bind( wxEVT_BUTTON, [this]( wxCommandEvent & )
			{
				bEmpty = true;
				EndModal( wxID_OK );
			} );
		}

		virtual ~CDBLinkWxDialog()
		{
			Unregister();
		}

		bool WasPlaced() const
		{
			return placement.WasPlaced();
		}

		// CPCDBLinkDialog::OnInitDialog from where the controls exist: the picker
		// becomes CHID_PC_DB_LINK_DIALOG, every chosen table is added in the
		// order the database lists them, and the one asked for -- or the first --
		// is chosen, which fills its tree.
		void Open()
		{
			// Also suppress selection callbacks if a failed Cancel reopens us.
			bCreateControls = true;
			ICommandHandlerContainer *const pContainer = Singleton<ICommandHandlerContainer>();
			pPreviousCommandHandler = pContainer->Get( CHID_PC_DB_LINK_DIALOG );
			pContainer->Set( CHID_PC_DB_LINK_DIALOG, this );
			bRegistered = true;

			pGrid->EnableEdit( request.bEnableEdit );
			pBrowser->EnableEdit( request.bEnableEdit );
			pBrowser->RemoveAllTables();
			IObjectTree *pActive = nullptr;
			if ( CPtr<IManipulator> pTableManipulator = Singleton<IResourceManager>()->CreateTableManipulator() )
			{
				if ( CPtr<IManipulatorIterator> pIterator = pTableManipulator->Iterate( true, ECT_NO_CACHE ) )
				{
					std::string szName;
					for ( ; !pIterator->IsEnd(); pIterator->Next() )
					{
						pIterator->GetName( &szName );
						if ( request.selectedTables.find( szName ) == request.selectedTables.end() )
						{
							continue;
						}
						if ( IObjectTree *const pTree = pBrowser->AddTable( szName ) )
						{
							if ( ( szName == szCurrentTable ) || ( pActive == nullptr ) )
							{
								pActive = pTree;
							}
							pTree->SetPCDialogCommandHandlerID( CHID_PC_DB_LINK_DIALOG, false );
						}
					}
				}
			}
			bCreateControls = false;
			if ( pActive != nullptr )
			{
				pBrowser->ActivateTable( pActive );
			}
			UpdateTitle();
			UpdateSelections();
			UpdateOkButton();
		}

		// OnDestroy's order: the widths and the placement, the tables, then the
		// previous handler back.
		void Close()
		{
			int nWidths[N_COLUMN_COUNT] = { 0, 0, 0 };
			if ( pGrid->GetColumnWidths( nWidths ) )
			{
				for ( int nColumn = 0; nColumn < N_COLUMN_COUNT; ++nColumn )
				{
					placement.State().SetIntParameter( nColumn, nWidths[nColumn] );
				}
			}
			placement.Save( this );
			pBrowser->RemoveAllTables();
			pGrid->GetView()->RemoveViewManipulator();
			Unregister();
		}

		void GetResult( NDBLink::SResult *pResult ) const
		{
			pResult->szTable = szCurrentTable;
			pResult->szObject = szCurrentObject;
			pResult->bEmpty = bEmpty;
		}

		// CPCBaseDialog
		virtual IView* GetView()
		{
			return pGrid->GetView();
		}

		virtual ICommandHandler* GetCommandHandler()
		{
			return pGrid->GetCommandHandler();
		}

		virtual void CreateTree()
		{
			pGrid->BuildTree();
		}

		virtual void UpdateValues()
		{
			pGrid->UpdateValues();
		}

		// IObjectBrowser::IListener. CPCDBLinkDialog::OnTabSelected: a table's
		// tree is pointed at its folder the first time it is chosen, starting on
		// the object the picker opened on; after that it hands its selection to
		// the grid again.
		virtual void OnTableSelected()
		{
			if ( bCreateControls || !pBrowser )
			{
				return;
			}
			pBrowser->ShowActiveTable();
			pBrowser->GetActiveTableName( &szCurrentTable );
			IObjectTree *const pTree = pBrowser->GetActiveTable();
			if ( pTree == nullptr )
			{
				return;
			}
			if ( !pTree->IsTreeCreated() )
			{
				SObjectSet collectionObjectSet;
				collectionObjectSet.szObjectTypeName = szCurrentTable;
				InsertHashSetElement( &( collectionObjectSet.objectNameSet ), CDBID( VIEW_COLLECTION_ID ) );
				pTree->GetView()->SetViewManipulator( Singleton<IResourceManager>()->CreateFolderManipulator( szCurrentTable ),
																							collectionObjectSet, std::string() );
				pTree->SetCurrentTreeItemName( request.szObject, false );
				pTree->CreateTree();
			}
			else
			{
				pTree->UpdateSelectionManipulator( true );
			}
			UpdateTitle();
			UpdateSelections();
			UpdateOkButton();
		}

		// OnMessageTreeGDBBrowser's TREE_GDB_BROWSER_CHANGE_SELECTION.
		virtual void OnTreeSelectionChanged( IObjectTree *pTree )
		{
			std::string szName;
			if ( !pBrowser || !pTree->GetCurrentTreeItemName( &szName ) )
			{
				return;
			}
			szCurrentObject = szName;
			pBrowser->GetActiveTableName( &szCurrentTable );
			UpdateTitle();
			UpdateSelections();
			UpdateOkButton();
		}

		// And its TREE_GDB_BROWSER_LOAD: OK.
		virtual void OnTreeLoad( IObjectTree *pTree )
		{
			if ( IsModal() )
			{
				EndModal( wxID_OK );
			}
		}

	private:
		void Unregister()
		{
			if ( bRegistered )
			{
				Singleton<ICommandHandlerContainer>()->Set( CHID_PC_DB_LINK_DIALOG, pPreviousCommandHandler );
				bRegistered = false;
			}
		}

		// CPCDBLinkDialog::UpdateDialogTitle.
		void UpdateTitle()
		{
			const bool bNoTable = szCurrentTable.empty();
			const UINT nFormatID = ( request.eType == NDBLink::TYPE_OPEN )
															 ? ( bNoTable ? IDS_PC_OPEN_DIALOG_EMPTY_TITLE : IDS_PC_OPEN_DIALOG_TITLE )
															 : ( bNoTable ? IDS_PC_LINK_DIALOG_EMPTY_TITLE : IDS_PC_LINK_DIALOG_TITLE );
			SetTitle( FromNarrow( fmt::sprintf( LoadResourceString( nFormatID ), szCurrentTable.c_str() ) ) );
		}

		std::string SelectionText( const std::string &rszTable, const std::string &rszObject ) const
		{
			if ( request.bMultiRef )
			{
				return fmt::format( "{}{:c}{}", rszTable, TYPE_SEPARATOR_CHAR, rszObject );
			}
			return rszObject;
		}

		// UpdateCurrentObjectLabel and UpdatePreviousObjectLabel.
		void UpdateSelections()
		{
			pCurrentSelection->SetLabelText( FromNarrow( SelectionText( szCurrentTable, szCurrentObject ) ) );
			pPreviousSelection->SetLabelText( ( request.eType == NDBLink::TYPE_LINK )
																					? FromNarrow( SelectionText( request.szTable, request.szObject ) )
																					: wxString() );
		}

		// CPCDBLinkDialog::UpdateOK: an object, not a folder, is selected.
		void UpdateOkButton()
		{
			if ( wxWindow *const pOk = FindWindow( wxID_OK ) )
			{
				pOk->Enable( !szCurrentObject.empty() && ( szCurrentObject[szCurrentObject.size() - 1] != PATH_SEPARATOR_CHAR ) );
			}
		}
	};
}


namespace NDBLink
{
	bool RunWx( IWidget *pParent, const SRequest &rRequest, SResult *pResult )
	{
		// Property buttons edit the shared database immediately. Give this
		// picker an isolated history so Cancel restores its in-memory edits,
		// including changes accepted by nested reference pickers.
		CControllerContainer::CEditSession editSession( checked_cast<CControllerContainer*>( Singleton<IControllerContainer>() ) );
		CDBLinkWxDialog dialog( rRequest );
		if ( !dialog.WasPlaced() )
		{
			NWxModal::CentreOver( &dialog, pParent );
		}
		bool bAccepted = false;
		do
		{
			dialog.Open();
			bAccepted = ( NWxModal::ShowModalOver( &dialog, pParent ) == wxID_OK );
			// Detach the grid before rollback so losing focus or destroying its
			// editor cannot commit a pending value after the undo has run.
			dialog.Close();
			if ( editSession.Finish( bAccepted ) )
			{
				break;
			}
			wxMessageBox( "Some property changes could not be undone. The picker will reopen so you can review them.",
									"Unable to cancel changes", wxOK | wxICON_ERROR, &dialog );
		} while ( true );
		if ( bAccepted && ( pResult != 0 ) )
		{
			dialog.GetResult( pResult );
		}
		return bAccepted;
	}
}

