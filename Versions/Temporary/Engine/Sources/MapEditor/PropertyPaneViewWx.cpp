#include "stdafx.h"

#include "PropertyPaneView.h"

#ifdef OBK2_WITH_WX

#include "PC_BaseDialog.h"
#include "PropertyButtons.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/DialogState.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/ObjectController.h"
#include "MapEditorLib/PCIEMnemonics.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/propgrid/editors.h>
#include <wx/propgrid/manager.h>
#include <wx/propgrid/props.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <functional>
#include <vector>

// Selection Properties in wx: a wxPropertyGrid, first slice -- the tree as the
// MFC one shows it, without editing.
//
// What this slice does, each the MFC tree's behaviour:
//
//   * builds the rows from the manipulator's names, a row per path segment, with
//     the value formatted as its editor formats it and the description in a
//     third column;
//   * greys the rows the database marks read-only, and everything in the
//     editor's read-only mode;
//   * remembers which rows are open, per object type, in the user data, and
//     tells the other views through an expand controller as the tree did;
//   * remembers the selected row, and shows the object's name in the status
//     line under the grid;
//   * follows undo and redo: a changed value is re-read, an array that gained or
//     lost an element is rebuilt, an expand is replayed;
//   * answers expand, collapse, refresh and show hidden;
//   * keeps the three column widths in the pane's state file.
//
// Editing, the second slice. A row is the property class its MFC editor
// corresponds to -- text for the inputs, sliders, int colours and GUIDs, a list
// for the combos and the bool combo and switcher, a check box for the bool
// check box -- and a change goes through NPropertyPane: parsed by the type's
// rules, written through a change controller onto the undo list, or put back
// when it does not parse or changes nothing.
//
// The buttons, the third slice. A row whose MFC editor had "...", "New" or
// "Edit" beside its box -- references, files, folders, bit fields, text files,
// long strings, colours -- is a text row with those buttons, and a button runs
// NPropertyButton, the code the MFC editors now run too. What it answers is
// committed like typed text. A long string's box stays read-only, since the row
// shows only its first line; its button edits the whole text.
//
// Copy and paste, the array commands and the context menu come after. So do
// buttons on read-only rows: the MFC tree gave those a browse button for
// looking, and here they have none.
//
// **Names.** A wxPropertyGrid property's GetName() is its path through its
// non-category parents joined with '.', and LEVEL_SEPARATOR_CHAR is '.'. So each
// row is named after its own segment and the grid's full name for it is the
// manipulator's name, with no map between the two.

namespace
{
	const unsigned N_COLUMN_COUNT = 3;
	const char *const PSZ_COLUMN_TITLES[N_COLUMN_COUNT] = { "Property", "Value", "Description" };
	// CPCDialog's PC_TREE_COLUMN_WIDTH.
	const int N_DEFAULT_COLUMN_WIDTH[N_COLUMN_COUNT] = { 100, 50, 100 };


	std::string ToNarrow( const wxString &rText )
	{
		return std::string( rText.utf8_str() );
	}


	// Narrow text here is the process code page, UTF-8 in this build; anything
	// that is not valid UTF-8 is read in the local code page instead of dropped.
	wxString FromNarrow( const std::string &rszText )
	{
		const wxString text = wxString::FromUTF8( rszText.c_str(), rszText.size() );
		if ( text.empty() && !rszText.empty() )
		{
			return wxString( rszText.c_str(), wxConvLocal, rszText.size() );
		}
		return text;
	}


	// A value row with the buttons its MFC editor had beside the box. It knows
	// which buttons, and whom to tell when one is pressed; CButtonEditor draws
	// them.
	class CButtonProperty : public wxStringProperty
	{
	public:
		typedef std::function<void( wxPGProperty*, NPropertyButton::EButton, const std::string& )> TPressed;

	private:
		std::vector<NPropertyButton::EButton> buttons;
		TPressed pressed;

	public:
		CButtonProperty( const wxString &rName, const std::vector<NPropertyButton::EButton> &rButtons, const TPressed &rPressed )
			: wxStringProperty( rName, rName, wxString() ), buttons( rButtons ), pressed( rPressed ) {}

		const std::vector<NPropertyButton::EButton>& GetButtons() const
		{
			return buttons;
		}

		// rszText is what the box holds now, typed but perhaps not committed.
		void Press( size_t nButton, const std::string &rszText )
		{
			if ( ( nButton < buttons.size() ) && pressed )
			{
				pressed( this, buttons[nButton], rszText );
			}
		}
	};


	// The text box with a row of buttons after it, wx's wxPGMultiButton pattern:
	// the buttons take their width from the right, the box keeps the rest.
	class CButtonEditor : public wxPGTextCtrlEditor
	{
	public:
		virtual wxString GetName() const override
		{
			return wxS( "OBK2PropertyButtons" );
		}

		virtual wxPGWindowList CreateControls( wxPropertyGrid *pGrid, wxPGProperty *pProperty,
																					 const wxPoint &rPosition, const wxSize &rSize ) const override
		{
			const CButtonProperty *const pButtonProperty = dynamic_cast<const CButtonProperty*>( pProperty );
			if ( ( pButtonProperty == nullptr ) || pButtonProperty->GetButtons().empty() )
			{
				return wxPGTextCtrlEditor::CreateControls( pGrid, pProperty, rPosition, rSize );
			}
			wxPGMultiButton *const pButtons = new wxPGMultiButton( pGrid, rSize );
			const std::vector<NPropertyButton::EButton> &rButtons = pButtonProperty->GetButtons();
			for ( std::vector<NPropertyButton::EButton>::const_iterator itButton = rButtons.begin(); itButton != rButtons.end(); ++itButton )
			{
				pButtons->Add( FromNarrow( NPropertyButton::GetTitle( *itButton ) ) );
			}
			wxPGWindowList windows = wxPGTextCtrlEditor::CreateControls( pGrid, pProperty, rPosition, pButtons->GetPrimarySize() );
			pButtons->Finalize( pGrid, rPosition );
			windows.SetSecondary( pButtons );
			return windows;
		}

		virtual bool OnEvent( wxPropertyGrid *pGrid, wxPGProperty *pProperty, wxWindow *pPrimary, wxEvent &rEvent ) const override
		{
			if ( rEvent.GetEventType() == wxEVT_BUTTON )
			{
				CButtonProperty *const pButtonProperty = dynamic_cast<CButtonProperty*>( pProperty );
				const wxPGMultiButton *const pButtons = dynamic_cast<const wxPGMultiButton*>( pGrid->GetEditorControlSecondary() );
				if ( ( pButtonProperty != nullptr ) && ( pButtons != nullptr ) )
				{
					for ( unsigned nButton = 0; nButton < pButtons->GetCount(); ++nButton )
					{
						if ( rEvent.GetId() == pButtons->GetButtonId( nButton ) )
						{
							const wxTextCtrl *const pText = wxDynamicCast( pPrimary, wxTextCtrl );
							pButtonProperty->Press( nButton, ToNarrow( ( pText != nullptr ) ? pText->GetValue() : pProperty->GetValueAsString() ) );
							// The value is not changed through the box: a pressed button
							// commits what it answers itself.
							return false;
						}
					}
				}
			}
			return wxPGTextCtrlEditor::OnEvent( pGrid, pProperty, pPrimary, rEvent );
		}
	};


	// Registered with wx once, which owns it from then on.
	wxPGEditor* ButtonEditor()
	{
		static wxPGEditor *s_pButtonEditor = nullptr;
		if ( s_pButtonEditor == nullptr )
		{
			s_pButtonEditor = wxPropertyGrid::RegisterEditorClass( new CButtonEditor() );
		}
		return s_pButtonEditor;
	}


	// The IView and command handler the pane hands out: CPCMainTreeControl's
	// place.
	class CPropertyGridView : public CDefaultView, public ICommandHandler
	{
		enum EExpandMode
		{
			EXPAND_USER_DEFINED,
			EXPAND_ALWAYS,
			COLLAPSE_ALWAYS,
		};

		wxPropertyGridManager *pManager = nullptr;
		wxStaticText *pStatus = nullptr;
		// The pane's host window, which the buttons' dialogs belong to. It lives
		// as long as the pane, which outlives this view's use of it.
		IWidget *pOwner = nullptr;
		std::string szOptionsLabel;
		// Set while this view changes the grid itself, so the grid's own events
		// are not taken for the user's.
		bool bCreateControls = false;
		bool bShowHidden = false;
		bool bEnableEdit = true;
		bool bColumnsRestored = false;
		// CPCMainTreeControl's newElementExpandMode: Expand All and Collapse All
		// decide for rows added afterwards, until the next full build.
		EExpandMode eExpandMode = EXPAND_USER_DEFINED;

	public:
		void Attach( wxPropertyGridManager *_pManager, wxStaticText *_pStatus, IWidget *_pOwner, const std::string &rszOptionsLabel )
		{
			pManager = _pManager;
			pStatus = _pStatus;
			pOwner = _pOwner;
			szOptionsLabel = rszOptionsLabel;
			pManager->Bind( wxEVT_PG_SELECTED, &CPropertyGridView::OnSelected, this );
			pManager->Bind( wxEVT_PG_CHANGED, &CPropertyGridView::OnChanged, this );
			pManager->Bind( wxEVT_PG_ITEM_EXPANDED, &CPropertyGridView::OnExpanded, this );
			pManager->Bind( wxEVT_PG_ITEM_COLLAPSED, &CPropertyGridView::OnCollapsed, this );
			pManager->Bind( wxEVT_PG_COL_END_DRAG, &CPropertyGridView::OnColumnDragged, this );
			pManager->Bind( wxEVT_DESTROY, &CPropertyGridView::OnDestroyed, this );
			pManager->GetGrid()->Bind( wxEVT_SET_FOCUS, &CPropertyGridView::OnFocus, this );
			pManager->GetGrid()->Bind( wxEVT_SIZE, &CPropertyGridView::OnGridSize, this );
		}

		// CPCMainTreeControl::CreateTree( TVI_ROOT, true, ... ).
		void BuildTree()
		{
			if ( pManager == nullptr )
			{
				return;
			}
			bCreateControls = true;
			eExpandMode = EXPAND_USER_DEFINED;
			pManager->Freeze();
			pManager->ClearPage( 0 );
			const bool bHaveManipulator = ( GetViewManipulator() != 0 );
			if ( bHaveManipulator )
			{
				AddProperties( std::string() );
				ApplyExpandState( pManager->GetGrid()->GetRoot() );
			}
			pManager->Thaw();
			if ( bHaveManipulator )
			{
				SelectByName( CurrentPropertyName() );
			}
			bCreateControls = false;
			UpdateStatus();
		}

		// CPCMainTreeControl::CreateTree( TVI_ROOT, false, ... ): every value
		// read again, the rows left as they are.
		void UpdateValues()
		{
			if ( pManager == nullptr || GetViewManipulator() == 0 )
			{
				return;
			}
			bCreateControls = true;
			RefreshTexts( pManager->GetGrid()->GetRoot() );
			bCreateControls = false;
		}

		void EnableEdit( bool bEnable )
		{
			const bool bChanged = ( bEnableEdit != bEnable );
			bEnableEdit = bEnable;
			if ( pManager == nullptr )
			{
				return;
			}
			// Which rows can be edited is decided as they are made.
			if ( bChanged && GetViewManipulator() != 0 )
			{
				BuildTree();
			}
			wxPropertyGrid *const pGrid = pManager->GetGrid();
			if ( bEnable )
			{
				pGrid->ResetColours();
			}
			else
			{
				// COLOR_3DFACE behind the rows, and grey text on all of them, as
				// CPCMainTreeControl::EnableEdit and PickTextColors had it.
				const wxColour face = wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE );
				pGrid->SetCellBackgroundColour( face );
				pGrid->SetEmptySpaceColour( face );
				pGrid->SetCellTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
			}
		}

		// IView, through CDefaultView.
		virtual void Undo( IController *pController )
		{
			Replay( pController, true );
		}

		virtual void Redo( IController *pController )
		{
			Replay( pController, false );
		}

		// ICommandHandler, registered as CHID_PROPERTY_CONTROL and CHID_SELECTION
		// when the grid takes the focus, as the tree registered itself.
		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			if ( GetViewManipulator() == 0 || pManager == nullptr )
			{
				return false;
			}
			switch ( nCommandID )
			{
				case ID_PC_EXPAND_ALL:
					SetExpandedBelow( pManager->GetGrid()->GetRoot(), true );
					eExpandMode = EXPAND_ALWAYS;
					return true;
				case ID_PC_EXPAND:
					if ( wxPGProperty *pSelected = pManager->GetSelection() )
					{
						SetExpanded( pSelected, true );
						SetExpandedBelow( pSelected, true );
					}
					return true;
				case ID_PC_COLLAPSE:
					if ( wxPGProperty *pSelected = pManager->GetSelection() )
					{
						SetExpandedBelow( pSelected, false );
						SetExpanded( pSelected, false );
					}
					return true;
				case ID_PC_COLLAPSE_ALL:
					SetExpandedBelow( pManager->GetGrid()->GetRoot(), false );
					eExpandMode = COLLAPSE_ALWAYS;
					return true;
				case ID_PC_OPTIMAL_WIDTH:
					return true;
				case ID_PC_REFRESH:
					if ( wxPGProperty *pSelected = pManager->GetSelection() )
					{
						const std::string szName = FullName( pSelected );
						if ( pSelected->GetChildCount() > 0 )
						{
							RebuildChildren( szName );
						}
						else
						{
							UpdateValue( szName );
						}
					}
					return true;
				case ID_PC_SHOW_HIDDEN:
					bShowHidden = !bShowHidden;
					BuildTree();
					return true;
				default:
					return false;
			}
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			if ( GetViewManipulator() == 0 || pManager == nullptr || pbEnable == 0 || pbCheck == 0 )
			{
				return false;
			}
			const wxPGProperty *const pSelected = pManager->GetSelection();
			switch ( nCommandID )
			{
				case ID_PC_EXPAND_ALL:
				case ID_PC_COLLAPSE_ALL:
					( *pbEnable ) = true;
					( *pbCheck ) = false;
					return true;
				case ID_PC_EXPAND:
				case ID_PC_COLLAPSE:
					if ( pSelected != nullptr && pSelected->GetChildCount() > 0 )
					{
						( *pbEnable ) = true;
						( *pbCheck ) = false;
						return true;
					}
					return false;
				case ID_PC_OPTIMAL_WIDTH:
					( *pbEnable ) = false;
					( *pbCheck ) = false;
					return true;
				case ID_PC_REFRESH:
					( *pbEnable ) = ( pSelected != nullptr );
					( *pbCheck ) = false;
					return true;
				case ID_PC_SHOW_HIDDEN:
					( *pbEnable ) = true;
					( *pbCheck ) = bShowHidden;
					return true;
				default:
					return false;
			}
		}

	private:
		static std::string FullName( const wxPGProperty *pProperty )
		{
			return ToNarrow( pProperty->GetName() );
		}

		wxPGProperty* Find( const std::string &rszName ) const
		{
			return pManager->GetPropertyByName( FromNarrow( rszName ) );
		}

		std::string& CurrentPropertyName()
		{
			return Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].szCurrentProperty;
		}

		// Every name the manipulator lists, or only those under rszParentName --
		// CreateTree with a parent, which the tree used for a subtree whose
		// elements had changed. The names come in tree order, parent first.
		void AddProperties( const std::string &rszParentName )
		{
			IManipulator *const pManipulator = GetViewManipulator();
			CPtr<IManipulatorIterator> pIterator = pManipulator->Iterate( bShowHidden, ECT_NO_CACHE );
			if ( !pIterator )
			{
				return;
			}
			const std::string szPrefix = rszParentName.empty() ? std::string() : rszParentName + LEVEL_SEPARATOR_CHAR;
			bool bInside = false;
			for ( ; !pIterator->IsEnd(); pIterator->Next() )
			{
				std::string szName;
				pIterator->GetName( &szName );
				if ( szName.empty() )
				{
					continue;
				}
				if ( !szPrefix.empty() )
				{
					if ( szName.compare( 0, szPrefix.size(), szPrefix ) != 0 )
					{
						// The subtree's names are contiguous: past it, nothing more.
						if ( bInside )
						{
							break;
						}
						continue;
					}
					bInside = true;
				}
				if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pManipulator->GetDesc( szName ) ) )
				{
					AddProperty( szName, pDesc );
				}
			}
		}

		// CPCMainTreeControl::AddTreeItem: a row for the last segment, under the
		// rows for the ones before it, making any that are missing -- which the
		// tree made as folders.
		wxPGProperty* AddProperty( const std::string &rszName, const SPropertyDesc *pDesc )
		{
			wxPGProperty *pParent = nullptr;
			size_t nStart = 0;
			for ( size_t nSeparator = rszName.find( LEVEL_SEPARATOR_CHAR ); nSeparator != std::string::npos;
						nSeparator = rszName.find( LEVEL_SEPARATOR_CHAR, nStart ) )
			{
				wxPGProperty *pNode = Find( rszName.substr( 0, nSeparator ) );
				if ( pNode == nullptr )
				{
					pNode = AppendRow( pParent, rszName.substr( nStart, nSeparator - nStart ) );
				}
				pParent = pNode;
				nStart = nSeparator + 1;
			}
			wxPGProperty *pProperty = Find( rszName );
			if ( pProperty == nullptr )
			{
				pProperty = AppendProperty( pParent, rszName.substr( nStart ), rszName, pDesc );
			}
			ShowProperty( pProperty, rszName, pDesc );
			return pProperty;
		}

		wxPGProperty* AppendRow( wxPGProperty *pParent, const std::string &rszShortName )
		{
			const wxString name = FromNarrow( rszShortName );
			wxPGProperty *const pRow = new wxStringProperty( name, name, wxString() );
			// A folder the tree made for a missing parent: nothing to edit.
			pRow->ChangeFlag( wxPGFlags::ReadOnly, true );
			return ( pParent != nullptr ) ? pManager->AppendIn( pParent, pRow ) : pManager->Append( pRow );
		}

		// The types edited as text: CPCMainTreeControl::CreatePCItemEditor gives
		// these an edit box, or an edit box beside a slider or buttons. Not the
		// long string, whose row shows only its first line.
		static bool IsTextEdited( EPCIEType nType )
		{
			switch ( nType )
			{
				case PCIE_INT_INPUT:
				case PCIE_INT_SLIDER:
				case PCIE_INT_COLOR:
				case PCIE_INT_COLOR_WITH_ALPHA:
				case PCIE_VEC3_COLOR:
				case PCIE_FLOAT_INPUT:
				case PCIE_FLOAT_SLIDER:
				case PCIE_STRING_INPUT:
				case PCIE_STRING_REF:
				case PCIE_STRING_MULTI_REF:
				case PCIE_STRING_NEW_REF:
				case PCIE_STRING_NEW_MULTI_REF:
				case PCIE_STRING_FILE_REF:
				case PCIE_STRING_DIR_REF:
				case PCIE_TEXT_FILE:
				case PCIE_NEW_TEXT_FILE:
				case PCIE_BINARY_BIT_FIELD:
				case PCIE_GUID:
					return true;
				default:
					return false;
			}
		}

		// Whether the row may be edited at all: the editor's mode, the
		// database's read-only mark, and a value the objects agree on --
		// CreatePCItemEditor opens nothing over a multivariant.
		bool CanEdit( const std::string &rszName, EPCIEType nType )
		{
			if ( !bEnableEdit || !typePCIEMnemonics.IsLeaf( nType ) ||
					 NPropertyPane::IsReadOnly( GetViewManipulator(), rszName ) )
			{
				return false;
			}
			CVariant value;
			return NPropertyPane::GetValue( GetViewManipulator(), rszName, &value ) &&
						 ( value.GetType() != CVariant::VT_MULTIVARIANT );
		}

		// A leaf's row, of the class its MFC editor corresponds to.
		wxPGProperty* AppendProperty( wxPGProperty *pParent, const std::string &rszShortName,
																	const std::string &rszName, const SPropertyDesc *pDesc )
		{
			const wxString name = FromNarrow( rszShortName );
			const EPCIEType nType = typePCIEMnemonics.Get( pDesc, rszName );
			bool bEditable = CanEdit( rszName, nType );
			bool bCheckBox = false;
			std::vector<std::string> choices;
			std::vector<NPropertyButton::EButton> buttons;
			if ( bEditable )
			{
				NPropertyButton::GetButtons( nType, &buttons );
			}
			wxPGProperty *pRow = nullptr;
			if ( bEditable && nType == PCIE_BOOL_CHECKBOX )
			{
				pRow = new wxBoolProperty( name, name, false );
				bCheckBox = true;
			}
			else if ( !buttons.empty() )
			{
				pRow = new CButtonProperty( name, buttons,
					[this]( wxPGProperty *pProperty, NPropertyButton::EButton eButton, const std::string &rszText )
					{
						OnButton( pProperty, eButton, rszText );
					} );
				pRow->SetEditor( ButtonEditor() );
				// A read-only row still gets its editor in wx, with the box
				// read-only and the buttons live -- which is what the long string
				// wants, and only that type is not typed into.
				bEditable = IsTextEdited( nType );
			}
			else if ( bEditable && NPropertyPane::GetChoices( pDesc, nType, &choices ) )
			{
				wxArrayString labels;
				for ( std::vector<std::string>::const_iterator itChoice = choices.begin(); itChoice != choices.end(); ++itChoice )
				{
					labels.Add( FromNarrow( *itChoice ) );
				}
				// True and false are the whole list. Every other list is a combo box
				// the stored value may not be in, which an enum could not show.
				if ( nType == PCIE_BOOL_COMBO || nType == PCIE_BOOL_SWITCHER )
				{
					pRow = new wxEnumProperty( name, name, labels );
				}
				else
				{
					pRow = new wxEditEnumProperty( name, name, labels, wxArrayInt(), wxString() );
				}
			}
			else
			{
				pRow = new wxStringProperty( name, name, wxString() );
				bEditable = bEditable && IsTextEdited( nType );
			}
			pRow->ChangeFlag( wxPGFlags::ReadOnly, !bEditable );
			wxPGProperty *const pAdded = ( pParent != nullptr ) ? pManager->AppendIn( pParent, pRow ) : pManager->Append( pRow );
			if ( bCheckBox )
			{
				pManager->SetPropertyAttribute( pAdded, wxPG_BOOL_USE_CHECKBOX, true );
			}
			return pAdded;
		}

		// The user changed a row. CPCMainTreeControl read the editor back on
		// IC_KILL_FOCUS and IC_VALUE_CHANGED; the grid reports a finished edit
		// here, so this is that moment.
		void OnChanged( wxPropertyGridEvent &rEvent )
		{
			rEvent.Skip();
			wxPGProperty *const pProperty = rEvent.GetProperty();
			if ( bCreateControls || pProperty == nullptr || GetViewManipulator() == 0 )
			{
				return;
			}
			const std::string szName = FullName( pProperty );
			CVariant newValue;
			bool bParsed = false;
			if ( dynamic_cast<wxBoolProperty*>( pProperty ) != nullptr )
			{
				newValue = pProperty->GetValue().GetBool();
				bParsed = true;
			}
			else
			{
				bParsed = NPropertyPane::ParseValueText( GetViewManipulator(), szName,
																								 ToNarrow( pProperty->GetValueAsString() ), &newValue );
			}
			// A commit redoes the change into every view on the object, this one
			// included, which rewrites the row in its canonical form. Anything
			// else puts the stored value back.
			if ( !bParsed || !NPropertyPane::CommitValue( this, szName, newValue ) )
			{
				bCreateControls = true;
				RefreshText( pProperty, szName );
				bCreateControls = false;
			}
		}

		// A button beside a value was pressed. What it opens runs once the grid
		// is done with the click: New can make an object and move the editor on
		// to it, which rebuilds this grid, and a row and its editor must not be
		// deleted from inside their own event. So the row is carried by name.
		void OnButton( wxPGProperty *pProperty, NPropertyButton::EButton eButton, const std::string &rszText )
		{
			if ( ( pManager == nullptr ) || ( pProperty == nullptr ) )
			{
				return;
			}
			const std::string szName = FullName( pProperty );
			const std::string szText = rszText;
			pManager->CallAfter( [this, szName, eButton, szText]()
			{
				RunButton( szName, eButton, szText );
			} );
		}

		void RunButton( const std::string &rszName, NPropertyButton::EButton eButton, const std::string &rszText )
		{
			CPtr<IManipulator> pManipulator = GetViewManipulator();
			if ( ( pManager == nullptr ) || !pManipulator )
			{
				return;
			}
			const SPropertyDesc *const pDesc = dynamic_cast<const SPropertyDesc*>( pManipulator->GetDesc( rszName ) );
			if ( pDesc == 0 )
			{
				return;
			}
			const SObjectSet objectSet = GetObjectSet();
			NPropertyButton::SContext context;
			context.szName = rszName;
			context.nType = typePCIEMnemonics.Get( pDesc, rszName );
			context.pDesc = pDesc;
			context.pObjectSet = &objectSet;
			context.pOwner = pOwner;
			context.bEditable = CanEdit( rszName, context.nType );
			// The text the MFC editor's box would have held: what was typed, if it
			// parses, and otherwise what is stored, which is what the editors put
			// back over text that did not parse. A reference the grid shows as
			// "null" was an empty box.
			std::string szText = rszText;
			CVariant boxValue;
			if ( !IsTextEdited( context.nType ) || !NPropertyPane::ParseValueText( pManipulator, rszName, szText, &boxValue ) )
			{
				szText.clear();
				NPropertyPane::GetValueText( pManipulator, rszName, &szText, true );
				NPropertyPane::ParseValueText( pManipulator, rszName, szText, &boxValue );
			}
			if ( boxValue.GetType() == CVariant::VT_NULL )
			{
				szText.clear();
			}
			std::string szNewText;
			const bool bAnswered = NPropertyButton::Press( eButton, context, szText, &szNewText );
			// The dialog ran a message loop: the pane may be gone, or showing
			// another object, and then there is nothing here to write to.
			if ( ( pManager == nullptr ) || ( GetViewManipulator() != pManipulator.GetPtr() ) )
			{
				return;
			}
			CVariant newValue;
			if ( !bAnswered || !NPropertyPane::ParseValueText( pManipulator, rszName, szNewText, &newValue ) ||
					 !NPropertyPane::CommitValue( this, rszName, newValue ) )
			{
				// Whatever was typed and not committed goes back to what is stored,
				// as it does when an edit does not parse.
				if ( wxPGProperty *const pProperty = Find( rszName ) )
				{
					bCreateControls = true;
					RefreshText( pProperty, rszName );
					bCreateControls = false;
				}
			}
		}

		// CPCMainTreeControl::SetPCItemView, and PickTextColors' grey.
		void ShowProperty( wxPGProperty *pProperty, const std::string &rszName, const SPropertyDesc *pDesc )
		{
			// An array element has no description of its own, and neither does a
			// field whose descriptor has none. Every row gets a cell of its own for
			// the column, empty or not, for two reasons read out of wx's source:
			//
			//   * a property added under a parent starts with a copy of the
			//     parent's cells (wxPGProperty::InitAfterAdded), so a row left
			//     alone showed its parent's description;
			//   * SetPropertyCell ignores empty text, so it cannot clear that copy.
			//
			// The colours are the grid's default cell's, given explicitly: a
			// wxPGCell made from text alone has none, and painted black.
			const bool bHasDescription = ( pDesc != 0 ) && ( rszName[rszName.size() - 1] != ARRAY_NODE_END_CHAR );
			const wxPGCell &rDefaultCell = pManager->GetGrid()->GetPropertyDefaultCell();
			pProperty->SetCell( 2, wxPGCell( bHasDescription ? FromNarrow( pDesc->szDesc ) : wxString(), wxBitmapBundle(),
																			 rDefaultCell.GetFgCol(), rDefaultCell.GetBgCol() ) );
			RefreshText( pProperty, rszName );
			if ( NPropertyPane::IsReadOnly( GetViewManipulator(), rszName ) )
			{
				pManager->SetPropertyTextColour( pProperty, wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ),
																				 wxPGPropertyValuesFlags::DontRecurse );
			}
		}

		// The row's value from the manipulator: as a bool for a check box, as
		// its text for everything else, a list choosing the entry with that text.
		void RefreshText( wxPGProperty *pProperty, const std::string &rszName )
		{
			if ( dynamic_cast<wxBoolProperty*>( pProperty ) != nullptr )
			{
				CVariant value;
				if ( NPropertyPane::GetValue( GetViewManipulator(), rszName, &value ) &&
						 ( value.GetType() != CVariant::VT_MULTIVARIANT ) )
				{
					pManager->SetPropertyValue( pProperty, (bool)value );
				}
				return;
			}
			std::string szText;
			if ( NPropertyPane::GetValueText( GetViewManipulator(), rszName, &szText ) )
			{
				pManager->SetPropertyValueString( pProperty, FromNarrow( szText ) );
			}
		}

		void RefreshTexts( wxPGProperty *pParent )
		{
			for ( unsigned nChild = 0; nChild < pParent->GetChildCount(); ++nChild )
			{
				wxPGProperty *const pChild = pParent->Item( nChild );
				RefreshText( pChild, FullName( pChild ) );
				RefreshTexts( pChild );
			}
		}

		// CPCMainTreeControl::UpdateValue: the row, and a vec3_color above it,
		// whose text is made of this row's value.
		void UpdateValue( const std::string &rszName )
		{
			wxPGProperty *const pProperty = Find( rszName );
			if ( pProperty == nullptr )
			{
				return;
			}
			RefreshText( pProperty, rszName );
			const std::string szParentName = NPropertyPane::ParentName( rszName );
			if ( szParentName.empty() )
			{
				return;
			}
			const SPropertyDesc *pParentDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szParentName ) );
			if ( ( pParentDesc != 0 ) && ( typePCIEMnemonics.Get( pParentDesc, szParentName ) == PCIE_VEC3_COLOR ) )
			{
				if ( wxPGProperty *const pParent = Find( szParentName ) )
				{
					RefreshText( pParent, szParentName );
				}
			}
		}

		// An array that gained or lost elements: its rows made again from the
		// manipulator. The tree shifted its [n] labels by hand; reading them back
		// is the same answer and cannot drift from it.
		void RebuildChildren( const std::string &rszName )
		{
			wxPGProperty *const pProperty = Find( rszName );
			if ( pProperty == nullptr )
			{
				return;
			}
			const std::string szSelected = ( pManager->GetSelection() != nullptr ) ? FullName( pManager->GetSelection() ) : std::string();
			bCreateControls = true;
			pManager->Freeze();
			pManager->GetGrid()->ClearSelection( false );
			pProperty->DeleteChildren();
			AddProperties( rszName );
			RefreshText( pProperty, rszName );
			ApplyExpandState( pProperty );
			pManager->GetGrid()->RefreshGrid();
			pManager->Thaw();
			if ( !szSelected.empty() )
			{
				SelectByName( szSelected );
			}
			bCreateControls = false;
		}

		bool MustBeExpanded( const std::string &rszName )
		{
			switch ( eExpandMode )
			{
				case EXPAND_ALWAYS:
					return true;
				case COLLAPSE_ALWAYS:
					return false;
				default:
				{
					const SUserData::SObjectTypeData::CExpandedPropertySet &rExpanded =
						Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].expandedPropertySet;
					return rExpanded.find( rszName ) != rExpanded.end();
				}
			}
		}

		// New rows open as they were left, as ItemMustBeExpand decided.
		void ApplyExpandState( wxPGProperty *pParent )
		{
			for ( unsigned nChild = 0; nChild < pParent->GetChildCount(); ++nChild )
			{
				wxPGProperty *const pChild = pParent->Item( nChild );
				if ( pChild->GetChildCount() == 0 )
				{
					continue;
				}
				if ( MustBeExpanded( FullName( pChild ) ) )
				{
					pManager->Expand( pChild );
				}
				else
				{
					pManager->Collapse( pChild );
				}
				ApplyExpandState( pChild );
			}
		}

		// Expanding from a command. The tree's ExpandCompletely raised the same
		// notification a click does, so it was remembered like one; wx's Expand
		// raises nothing, so it is remembered here.
		void SetExpanded( wxPGProperty *pProperty, bool bExpand )
		{
			if ( pProperty->GetChildCount() == 0 || pProperty->IsExpanded() == bExpand )
			{
				return;
			}
			bCreateControls = true;
			if ( bExpand )
			{
				pManager->Expand( pProperty );
			}
			else
			{
				pManager->Collapse( pProperty );
			}
			bCreateControls = false;
			NoteExpanded( FullName( pProperty ), bExpand );
		}

		void SetExpandedBelow( wxPGProperty *pParent, bool bExpand )
		{
			for ( unsigned nChild = 0; nChild < pParent->GetChildCount(); ++nChild )
			{
				wxPGProperty *const pChild = pParent->Item( nChild );
				SetExpanded( pChild, bExpand );
				SetExpandedBelow( pChild, bExpand );
			}
		}

		// CPCMainTreeControl::OnItemExpanded: the user data, and the other views
		// on the same object, which an expand controller replays without adding
		// anything to the undo list.
		void NoteExpanded( const std::string &rszName, bool bExpanded )
		{
			SUserData::SObjectTypeData::CExpandedPropertySet &rExpanded =
				Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].expandedPropertySet;
			if ( bExpanded )
			{
				InsertHashSetElement( &rExpanded, rszName );
			}
			else
			{
				SUserData::SObjectTypeData::CExpandedPropertySet::iterator posExpanded = rExpanded.find( rszName );
				if ( posExpanded != rExpanded.end() )
				{
					rExpanded.erase( posExpanded );
				}
			}
			bCreateControls = true;
			CPtr<CObjectBaseController> pController = CreateController<CObjectController>( static_cast<CObjectController*>( 0 ) );
			if ( pController->AddExpandOperation( rszName, bExpanded, GetViewManipulator() ) )
			{
				pController->Redo( false, true, this );
			}
			bCreateControls = false;
		}

		void SelectByName( const std::string &rszName )
		{
			if ( wxPGProperty *const pProperty = Find( rszName ) )
			{
				pManager->SelectProperty( pProperty );
				pManager->GetGrid()->EnsureVisible( pProperty );
			}
		}

		// CPCMainTreeControl::UpdateStatusStringWindow: the object's name, while
		// a row is selected and the pane shows a single object.
		void UpdateStatus()
		{
			if ( pStatus == nullptr )
			{
				return;
			}
			std::string szText;
			if ( ( GetViewManipulator() != 0 ) && ( pManager->GetSelection() != nullptr ) &&
					 ( GetObjectSet().objectNameSet.size() == 1 ) )
			{
				szText = GetObjectSet().objectNameSet.begin()->first.ToString();
			}
			pStatus->SetLabelText( FromNarrow( szText ) );
		}

		// CPCMainTreeControl::Undo and Redo, in the controller's order.
		void Replay( IController *pController, bool bUndo )
		{
			CObjectBaseController *const pObjectController = dynamic_cast<CObjectBaseController*>( pController );
			if ( pObjectController == 0 || pObjectController->undoDataList.empty() ||
					 pManager == nullptr || GetViewManipulator() == 0 )
			{
				return;
			}
			GetViewManipulator()->ClearCache();
			IManipulator::CNameMap manipulatorNameMap;
			GetViewManipulator()->GetNameList( &manipulatorNameMap );

			std::vector<const CObjectBaseController::SUndoData*> steps;
			for ( CObjectController::CUndoDataList::const_iterator posUndoData = pObjectController->undoDataList.begin();
						posUndoData != pObjectController->undoDataList.end(); ++posUndoData )
			{
				steps.push_back( &( *posUndoData ) );
			}
			if ( bUndo )
			{
				std::reverse( steps.begin(), steps.end() );
			}

			for ( size_t nStep = 0; nStep < steps.size(); ++nStep )
			{
				const CObjectBaseController::SUndoData &rStep = *steps[nStep];
				IManipulator::CNameMap nameMap;
				pObjectController->GetNameListToUpdate( &nameMap, manipulatorNameMap, rStep.szName );
				for ( IManipulator::CNameMap::const_iterator posName = nameMap.begin(); posName != nameMap.end(); ++posName )
				{
					switch ( rStep.eType )
					{
						case CObjectBaseController::SUndoData::TYPE_INSERT:
						case CObjectBaseController::SUndoData::TYPE_REMOVE:
							RebuildChildren( posName->first );
							break;
						case CObjectBaseController::SUndoData::TYPE_CHANGE:
							UpdateValue( posName->first );
							break;
						case CObjectBaseController::SUndoData::TYPE_EXPAND:
						{
							bool bExpand = (bool)( rStep.newValue );
							if ( bUndo )
							{
								bExpand = !bExpand;
							}
							if ( wxPGProperty *const pProperty = Find( posName->first ) )
							{
								bCreateControls = true;
								if ( bExpand )
								{
									pManager->Expand( pProperty );
								}
								else
								{
									pManager->Collapse( pProperty );
								}
								bCreateControls = false;
							}
							break;
						}
						default:
							break;
					}
				}
			}
			pManager->Refresh();
		}

		// False while the grid is too narrow for the first two columns: the grid
		// clamps a splitter to its width, and then gives later growth to the last
		// column, so widths set into a pane still being laid out stay squashed.
		bool RestoreColumnWidths( int nGridWidth )
		{
			SDialogState state;
			NDialogState::Load( szOptionsLabel, &state );
			int nWidth[N_COLUMN_COUNT];
			for ( unsigned nColumn = 0; nColumn < N_COLUMN_COUNT; ++nColumn )
			{
				const int nSaved = state.GetIntParameter( nColumn );
				nWidth[nColumn] = ( nSaved > 0 ) ? nSaved : N_DEFAULT_COLUMN_WIDTH[nColumn];
			}
			if ( nGridWidth < nWidth[0] + nWidth[1] + 16 )
			{
				return false;
			}
			wxPropertyGrid *const pGrid = pManager->GetGrid();
			pGrid->SetSplitterPosition( nWidth[0], 0 );
			pGrid->SetSplitterPosition( nWidth[0] + nWidth[1], 1 );
			return true;
		}

		// Into the same nParameters CPCDialog::OnDestroy wrote, leaving the rest
		// of the file as it was.
		void SaveColumnWidths()
		{
			SDialogState state;
			NDialogState::Load( szOptionsLabel, &state );
			wxPropertyGrid *const pGrid = pManager->GetGrid();
			const int nFirst = pGrid->GetSplitterPosition( 0 );
			const int nSecond = pGrid->GetSplitterPosition( 1 );
			state.SetIntParameter( 0, nFirst );
			state.SetIntParameter( 1, nSecond - nFirst );
			state.SetIntParameter( 2, pGrid->GetClientSize().x - nSecond );
			NDialogState::Save( szOptionsLabel, &state );
		}

		void OnSelected( wxPropertyGridEvent &rEvent )
		{
			rEvent.Skip();
			if ( bCreateControls || GetViewManipulator() == 0 )
			{
				return;
			}
			if ( const wxPGProperty *const pProperty = rEvent.GetProperty() )
			{
				CurrentPropertyName() = FullName( pProperty );
			}
			UpdateStatus();
		}

		void OnExpanded( wxPropertyGridEvent &rEvent )
		{
			rEvent.Skip();
			if ( !bCreateControls && GetViewManipulator() != 0 && rEvent.GetProperty() != nullptr )
			{
				NoteExpanded( FullName( rEvent.GetProperty() ), true );
			}
		}

		void OnCollapsed( wxPropertyGridEvent &rEvent )
		{
			rEvent.Skip();
			if ( !bCreateControls && GetViewManipulator() != 0 && rEvent.GetProperty() != nullptr )
			{
				NoteExpanded( FullName( rEvent.GetProperty() ), false );
			}
		}

		void OnColumnDragged( wxPropertyGridEvent &rEvent )
		{
			rEvent.Skip();
			SaveColumnWidths();
		}

		// The widths are only worth setting once the grid has a width to put
		// them in.
		void OnGridSize( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			if ( !bColumnsRestored && pManager != nullptr )
			{
				bColumnsRestored = RestoreColumnWidths( rEvent.GetSize().x );
			}
		}

		void OnFocus( wxFocusEvent &rEvent )
		{
			rEvent.Skip();
			Singleton<ICommandHandlerContainer>()->Set( CHID_PROPERTY_CONTROL, this );
			Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
		}

		// A destroy event propagates up from every child the grid makes and
		// throws away, so only the manager's own ends this view's hold on it.
		void OnDestroyed( wxWindowDestroyEvent &rEvent )
		{
			rEvent.Skip();
			if ( rEvent.GetEventObject() == pManager )
			{
				pManager = nullptr;
				pStatus = nullptr;
			}
		}
	};


	// The pane's contents: the grid over its status line, in a host window the
	// MFC pane can hold, registered as CHID_PC_DIALOG as CPCDialog registers
	// itself.
	class CWxPropertyPane : public IPropertyPane, public CPCBaseDialog
	{
		CWxHostWindow host;
		CPropertyGridView view;
		ICommandHandler *pPreviousCommandHandler = nullptr;
		bool bRegistered = false;

	public:
		virtual ~CWxPropertyPane()
		{
			if ( bRegistered )
			{
				ICommandHandlerContainer *const pContainer = Singleton<ICommandHandlerContainer>();
				pContainer->Set( CHID_PC_DIALOG, pPreviousCommandHandler );
				pContainer->Remove( CHID_PROPERTY_CONTROL, &view );
				pContainer->Remove( CHID_SELECTION, &view );
			}
		}

		virtual bool Create( IWidget *pParentPane, const std::string &rszOptionsLabel )
		{
			if ( !host.CreateHost( ToCWnd( pParentPane ) ) )
			{
				return false;
			}
			wxWindow *const pRoot = host.Root();

			// IDD_PC: the tree filling the pane, with a client edge, over a sunken
			// status line.
			wxPropertyGridManager *const pManager = NWx::Child<wxPropertyGridManager>(
				pRoot, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPGMAN_DEFAULT_STYLE | wxBORDER_SUNKEN );
			pManager->AddPage();
			pManager->SetColumnCount( N_COLUMN_COUNT );
			for ( unsigned nColumn = 0; nColumn < N_COLUMN_COUNT; ++nColumn )
			{
				pManager->SetColumnTitle( nColumn, PSZ_COLUMN_TITLES[nColumn] );
			}
			pManager->ShowHeader();

			wxStaticText *const pStatus = NWx::Child<wxStaticText>( pRoot, wxID_ANY, wxString(), wxDefaultPosition,
																															wxSize( -1, pRoot->FromDIP( 18 ) ),
																															wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END | wxBORDER_SUNKEN );
			wxBoxSizer *const pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pManager, wxSizerFlags( 1 ).Expand() );
			pSizer->Add( pStatus, wxSizerFlags().Expand().Border( wxTOP, pRoot->FromDIP( 2 ) ) );
			pRoot->SetSizer( pSizer );

			view.Attach( pManager, pStatus, &host, rszOptionsLabel );

			ICommandHandlerContainer *const pContainer = Singleton<ICommandHandlerContainer>();
			pPreviousCommandHandler = pContainer->Get( CHID_PC_DIALOG );
			pContainer->Set( CHID_PC_DIALOG, this );
			bRegistered = true;
			return true;
		}

		virtual bool IsCreated() const
		{
			return host.GetSafeHwnd() != 0;
		}

		virtual void SetBounds( const CTRect<int> &rBounds )
		{
			host.MoveWindow( rBounds.left, rBounds.top, rBounds.Width(), rBounds.Height() );
		}

		virtual void Show( bool bShow )
		{
			host.ShowWindow( bShow ? SW_SHOW : SW_HIDE );
		}

		virtual void EnableEdit( bool bEnable )
		{
			view.EnableEdit( bEnable );
		}

		// CPCBaseDialog
		virtual IView* GetView()
		{
			return &view;
		}

		virtual ICommandHandler* GetCommandHandler()
		{
			return &view;
		}

		virtual void CreateTree()
		{
			view.BuildTree();
		}

		virtual void UpdateValues()
		{
			view.UpdateValues();
		}
	};
}


namespace NPropertyPane
{
	IPropertyPane* CreateWx()
	{
		return new CWxPropertyPane();
	}
}

#endif // OBK2_WITH_WX
