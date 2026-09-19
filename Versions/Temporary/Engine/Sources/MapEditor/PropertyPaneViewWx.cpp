#include "stdafx.h"
#include "MapEditorLib/Resources.h"

#include "PropertyPaneView.h"


#include "PC_BaseDialog.h"
#include "PropertyButtons.h"
#include "ResourceDefines.h"

#include "Image/ImageColor.h"

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
#include "MapEditorLib/WxResourceImages.h"

#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/headerctrl.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/propgrid/editors.h>
#include <wx/propgrid/manager.h>
#include <wx/propgrid/props.h>
#include <wx/renderer.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <algorithm>
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
// The commands, the fourth slice, each CPCMainTreeControl's:
//
//   * several rows can be selected, and Select All selects every row on show;
//   * Copy puts the selected rows and everything under them on the clipboard,
//     a line each of name, value and description split by tabs, and keeps
//     their values for Paste, which writes those the grid shows as one change;
//   * Add and Delete All on an array, Insert and Delete on an element, through
//     insert and remove operations on the undo list, the deletes asked first;
//   * a text box open over a row takes Cut, Copy, Paste, Clear and Select All
//     instead, as the MFC editors did while they had the focus;
//   * the right-click menu is IDM_PC_CONTEXT_MENU's, its variant picked as the
//     tree picked it, and its commands go where the frame would send them.
//
// Still to come: buttons on read-only rows, which the MFC tree gave a browse
// button for looking and which here have none.
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


	// Where wxPropertyGrid puts a cell's text and icon: wxPG_XBEFORETEXT on MSW
	// and wxCC_CUSTOM_IMAGE_MARGIN1, from wx/propgrid/private.h, which may not be
	// included outside wx.
	const int N_PG_TEXT_BEFORE = 4;
	const int N_PG_IMAGE_BEFORE = 4;


	// The tree cut a name or a value that did not fit its column off with an
	// ellipsis; the grid clips it. This renderer draws a cell's text cut to the
	// cell the same way, and leaves every cell with drawing of its own -- a check
	// box, a custom image, a value the objects do not agree on, a list's popup --
	// to wx's default renderer, which it is.
	class CEllipsisRenderer : public wxPGDefaultRenderer
	{
	public:
		virtual bool Render( wxDC &rDC, const wxRect &rRect, const wxPropertyGrid *pGrid, wxPGProperty *pProperty,
												 int nColumn, int nItem, int nFlags ) const override
		{
			if ( ( nItem != -1 ) || ( nColumn == 1 && ( pProperty->IsValueUnspecified() || ( pProperty->GetCommonValue() >= 0 ) ||
																								 ( pGrid->GetImageSize( pProperty, nItem ).x > 0 ) ||
																								 ( ( pProperty->GetColumnEditor( 1 ) != nullptr ) &&
																									 ( pProperty->GetColumnEditor( 1 )->GetName() == wxS( "CheckBox" ) ) ) ) ) )
			{
				return wxPGDefaultRenderer::Render( rDC, rRect, pGrid, pProperty, nColumn, nItem, nFlags );
			}
			wxString text;
			wxPGCell cell;
			pProperty->GetDisplayInfo( nColumn, -1, nFlags, &text, &cell );
			const int nImageOffset = pProperty->GetImageOffset( PreDrawCell( rDC, rRect, pGrid, cell, nFlags ) );
			// wxPGCellRenderer::DrawText puts the text this far into the cell.
			const int nRoom = rRect.width - nImageOffset - 2 * N_PG_TEXT_BEFORE;
			if ( nRoom > 0 )
			{
				DrawText( rDC, rRect, nImageOffset, wxControl::Ellipsize( text, rDC, wxELLIPSIZE_END, nRoom ) );
			}
			PostDrawCell( rDC, pGrid, cell, nFlags );
			return !text.empty();
		}
	};


	// One renderer for every row, made once and never freed: wx does not free
	// what a property's GetCellRenderer returns, and it is used until the editor
	// closes.
	wxPGCellRenderer* EllipsisRenderer()
	{
		static CEllipsisRenderer *s_pRenderer = new CEllipsisRenderer();
		return s_pRenderer;
	}


	// A wx property class whose cells draw through EllipsisRenderer.
	template <class TProperty>
	class CEllipsized : public TProperty
	{
	public:
		using TProperty::TProperty;

		virtual wxPGCellRenderer* GetCellRenderer( int nColumn ) const override
		{
			return EllipsisRenderer();
		}
	};


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

		virtual wxPGCellRenderer* GetCellRenderer( int nColumn ) const override
		{
			return EllipsisRenderer();
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


	// An icon strip, loaded once and never freed: the grids that draw its icons
	// live until the editor closes, and a wxBitmap left to a static destructor
	// would outlive wx.
	const std::vector<wxBitmap>& Icons( UINT nResourceID )
	{
		static std::vector<wxBitmap> *s_pTypeIcons = nullptr;
		static std::vector<wxBitmap> *s_pHeaderIcons = nullptr;
		std::vector<wxBitmap> *&rpIcons = ( nResourceID == IDB_PC_TYPES_IMAGE_LIST ) ? s_pTypeIcons : s_pHeaderIcons;
		if ( rpIcons == nullptr )
		{
			rpIcons = new std::vector<wxBitmap>( NWxResourceImages::LoadIcons( nResourceID ) );
		}
		return *rpIcons;
	}


	// pc_types.bmp's icon for a row of nType, the image CPCMainTreeControl gave
	// InsertTreeItem. The strip holds a second icon per type after PCIE_COUNT,
	// the tree's selected image, which a wx grid cell has no place for.
	wxBitmapBundle TypeIcon( EPCIEType nType )
	{
		const std::vector<wxBitmap> &rIcons = Icons( IDB_PC_TYPES_IMAGE_LIST );
		return ( ( nType >= 0 ) && ( static_cast<size_t>( nType ) < rIcons.size() ) ) ? wxBitmapBundle( rIcons[nType] ) : wxBitmapBundle();
	}


	// The types CPCMainTreeControl::PickTextColors painted the value cell of in
	// the value's colour.
	bool IsColourType( EPCIEType nType )
	{
		return ( nType == PCIE_INT_COLOR ) || ( nType == PCIE_INT_COLOR_WITH_ALPHA ) || ( nType == PCIE_VEC3_COLOR );
	}


	// A grid that draws its expand buttons as the MFC tree did, a box with a
	// plus or a minus, rather than the platform's arrows: wx's generic renderer
	// draws exactly that box.
	//
	// It also keeps a selected row visible while the grid has no focus. wx paints
	// such a row in the margin's colour, and ApplyClassicLook makes the margin
	// the rows' own colour, which would hide the selection. So an unfocused grid
	// is painted as a focused one, with the selection in COLOR_3DFACE -- what an
	// unfocused tree shows its selection in.
	class CClassicPropertyGrid : public wxPropertyGrid
	{
	public:
		CClassicPropertyGrid()
		{
			// Bound, so it runs before wxPropertyGrid's own OnPaint in its event table.
			Bind( wxEVT_PAINT, &CClassicPropertyGrid::OnClassicPaint, this );
		}

	private:
		void OnClassicPaint( wxPaintEvent &rEvent )
		{
			if ( ( m_iFlags & wxPG_FL_FOCUSED ) != 0 )
			{
				wxPropertyGrid::OnPaint( rEvent );
			}
			else
			{
				const wxColour selectionBack = m_colSelBack;
				const wxColour selectionFore = m_colSelFore;
				m_iFlags |= wxPG_FL_FOCUSED;
				m_colSelBack = wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE );
				m_colSelFore = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT );
				wxPropertyGrid::OnPaint( rEvent );
				m_colSelFore = selectionFore;
				m_colSelBack = selectionBack;
				m_iFlags &= ~wxPG_FL_FOCUSED;
			}
			DrawTreeLines();
		}

		// A dot every other pixel, on the even ones of the grid's unscrolled
		// coordinates so the dots stay put as the rows scroll: the tree's lines.
		void DrawDots( wxDC &rDC, int nX0, int nY0, int nX1, int nY1, int nScrollY ) const
		{
			for ( int nX = nX0; nX <= nX1; ++nX )
			{
				for ( int nY = nY0; nY <= nY1; ++nY )
				{
					if ( ( ( nX + nY + nScrollY ) % 2 ) == 0 )
					{
						rDC.DrawPoint( nX, nY );
					}
				}
			}
		}

		// TVS_HASLINES and TVS_LINESATROOT: from each row's button column a line
		// across to its icon, a line up to the row above at its level unless it is
		// the first row of all, a line down while it has a sibling after it, and a
		// line through the row at every level above whose row has a sibling after
		// it. The box buttons are left clear. Drawn over the painted rows, in the
		// margin, where nothing else is.
		void DrawTreeLines()
		{
			wxClientDC dc( this );
			dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNSHADOW ) ) );
			const int nSide = FromDIP( 9 );
			const int nHalfSide = nSide / 2;
			const int nClientHeight = GetClientSize().y;
			int nUnused = 0;
			int nScrollY = 0;
			CalcUnscrolledPosition( 0, 0, &nUnused, &nScrollY );
			for ( wxPGVIterator it = GetVIterator( wxPG_ITERATE_VISIBLE ); !it.AtEnd(); it.Next() )
			{
				const wxPGProperty *const pProperty = it.GetProperty();
				const int nTop = pProperty->GetY() - nScrollY;
				if ( nTop + m_lineHeight <= 0 )
				{
					continue;
				}
				if ( nTop >= nClientHeight )
				{
					break;
				}
				const int nBottom = nTop + m_lineHeight - 1;
				const int nCentreY = nTop + m_lineHeight / 2;
				const unsigned nDepth = pProperty->GetDepth();
				// DrawItems' butRect, and the adjustment it makes below the top level.
				const auto centreX = [this]( unsigned nLevel )
				{
					const int nLeft = static_cast<int>( nLevel - 1 ) * m_subgroup_extramargin;
					return nLeft + ( ( nLeft > 0 ) ? IN_CELL_BUTTON_X_ADJUST : 0 ) + m_marginWidth / 2;
				};
				const int nCentreX = centreX( nDepth );
				const bool bButton = pProperty->HasVisibleChildren();
				const int nIconLeft = m_marginWidth + 1 + static_cast<int>( nDepth - 1 ) * m_subgroup_extramargin + N_PG_IMAGE_BEFORE;
				DrawDots( dc, nCentreX + ( bButton ? nHalfSide + 1 : 0 ), nCentreY, nIconLeft - 2, nCentreY, nScrollY );
				const wxPGProperty *const pParent = pProperty->GetParent();
				const bool bFirstOfAll = ( nDepth == 1 ) && ( pProperty->GetIndexInParent() == 0 );
				if ( !bFirstOfAll )
				{
					DrawDots( dc, nCentreX, nTop, nCentreX, nCentreY - ( bButton ? nHalfSide + 1 : 0 ), nScrollY );
				}
				if ( ( pParent != nullptr ) && ( pProperty->GetIndexInParent() + 1 < pParent->GetChildCount() ) )
				{
					DrawDots( dc, nCentreX, nCentreY + ( bButton ? nHalfSide + 1 : 0 ), nCentreX, nBottom, nScrollY );
				}
				unsigned nLevel = nDepth;
				for ( const wxPGProperty *pAncestor = pParent; ( pAncestor != nullptr ) && ( nLevel > 1 ); pAncestor = pAncestor->GetParent() )
				{
					--nLevel;
					const wxPGProperty *const pAncestorParent = pAncestor->GetParent();
					if ( ( pAncestorParent != nullptr ) && ( pAncestor->GetIndexInParent() + 1 < pAncestorParent->GetChildCount() ) )
					{
						const int nAncestorX = centreX( nLevel );
						DrawDots( dc, nAncestorX, nTop, nAncestorX, nBottom, nScrollY );
					}
				}
			}
		}

		// wxPropertyGrid's IN_CELL_EXPANDER_BUTTON_X_ADJUST, private to its source.
		static const int IN_CELL_BUTTON_X_ADJUST = 2;

	protected:
		virtual void DrawExpanderButton( wxDC &rDC, const wxRect &rRect, wxPGProperty *pProperty ) const override
		{
			// The tree's box was 9 pixels square, centred in the margin the grid
			// gives the button; the grid's own button is the native one's size.
			const int nSide = FromDIP( 9 );
			const wxRect button( rRect.x + ( rRect.width - nSide ) / 2, rRect.y + ( rRect.height - nSide ) / 2, nSide, nSide );
			wxRendererNative::GetGeneric().DrawTreeItemButton( const_cast<CClassicPropertyGrid*>( this ), rDC, button,
																												 pProperty->IsExpanded() ? wxCONTROL_EXPANDED : wxCONTROL_NONE );
		}
	};


	// A grid manager whose grid is a CClassicPropertyGrid. wxPropertyGridManager
	// asks CreatePropertyGrid from Create, so the window is created from this
	// constructor's body, where the call reaches this class's override.
	class CClassicGridManager : public wxPropertyGridManager
	{
	public:
		CClassicGridManager( wxWindow *pParent, long nStyle )
		{
			Create( pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, nStyle );
		}

	protected:
		virtual wxPropertyGrid* CreatePropertyGrid() const override
		{
			return new CClassicPropertyGrid();
		}
	};


	// wxHeaderCtrlBase::GetColumn is protected, and the manager's header class
	// that makes it public is private to wx. A class derived from the base may
	// name the member, and a pointer to it may be applied to any header; this is
	// never instantiated.
	struct CHeaderColumnAccess : public wxHeaderCtrl
	{
		static const wxHeaderColumn& Column( const wxHeaderCtrl *pHeader, unsigned nColumn )
		{
			const wxHeaderColumn& ( wxHeaderCtrlBase::*pGetColumn )( unsigned int ) const = &CHeaderColumnAccess::GetColumn;
			return ( pHeader->*pGetColumn )( nColumn );
		}
	};


	// No lines between the rows or the columns and no grey margin, as the MFC
	// tree drew its rows -- CPCDialog left LVXS_LINESBETWEENCOLUMNS and
	// LVXS_LINESBETWEENITEMS commented out -- in the colour the rows are on.
	void ApplyClassicLook( wxPropertyGrid *pGrid, const wxColour &rBackground )
	{
		pGrid->SetLineColour( rBackground );
		pGrid->SetMarginColour( rBackground );
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
		// The first two columns' widths the grid keeps through resizes: the saved
		// ones, until the user drags a column.
		int nKeptColumnWidth[2] = { 0, 0 };
		bool bColumnWidthsLoaded = false;
		// Set while the grid is too narrow for the kept widths and shows them cut
		// down: what is kept is still the widths, not what shows.
		bool bColumnsClipped = false;
		// The dialog around the grid, told after undo and redo.
		std::function<void()> changeCallback;
		// The dialog around the grid, told which row is selected.
		std::function<void( const std::string& )> selectionCallback;
		// CPCMainTreeControl's newElementExpandMode: Expand All and Collapse All
		// decide for rows added afterwards, until the next full build.
		EExpandMode eExpandMode = EXPAND_USER_DEFINED;

	public:
		virtual ~CPropertyGridView()
		{
			ICommandHandlerContainer *const pContainer = Singleton<ICommandHandlerContainer>();
			pContainer->Remove( CHID_PROPERTY_CONTROL, this );
			pContainer->Remove( CHID_SELECTION, this );
			// The grid calls into this view from its events, its destroy event
			// last of all, so a view that goes first takes the grid with it. The
			// destroy event clears pManager.
			if ( pManager != nullptr )
			{
				pManager->Destroy();
			}
		}

		void SetChangeCallback( const std::function<void()> &rCallback )
		{
			changeCallback = rCallback;
		}

		void SetSelectionCallback( const std::function<void( const std::string& )> &rCallback )
		{
			selectionCallback = rCallback;
		}

		// First, second and third, as CPCDialog::OnDestroy measured them. False
		// until the widths have been put in, since the grid squashes them before.
		bool GetColumnWidths( int *pnWidths ) const
		{
			if ( ( pManager == nullptr ) || ( pnWidths == 0 ) || !bColumnsRestored )
			{
				return false;
			}
			const wxPropertyGrid *const pGrid = pManager->GetGrid();
			if ( bColumnsClipped )
			{
				pnWidths[0] = nKeptColumnWidth[0];
				pnWidths[1] = nKeptColumnWidth[1];
				pnWidths[2] = N_DEFAULT_COLUMN_WIDTH[2];
				return true;
			}
			const int nFirst = pGrid->GetSplitterPosition( 0 );
			const int nSecond = pGrid->GetSplitterPosition( 1 );
			pnWidths[0] = nFirst;
			pnWidths[1] = nSecond - nFirst;
			pnWidths[2] = pGrid->GetClientSize().x - nSecond;
			return true;
		}

		wxPropertyGridManager* GetManager() const
		{
			return pManager;
		}

		void Attach( wxPropertyGridManager *_pManager, wxStaticText *_pStatus, IWidget *_pOwner, const std::string &rszOptionsLabel )
		{
			pManager = _pManager;
			pStatus = _pStatus;
			pOwner = _pOwner;
			szOptionsLabel = rszOptionsLabel;
			pManager->Bind( wxEVT_PG_SELECTED, &CPropertyGridView::OnSelected, this );
			pManager->Bind( wxEVT_PG_CHANGED, &CPropertyGridView::OnChanged, this );
			pManager->Bind( wxEVT_PG_RIGHT_CLICK, &CPropertyGridView::OnRightClick, this );
			// The tree took Ctrl and Shift clicks, and Copy works on all it selected.
			pManager->SetExtraStyle( pManager->GetExtraStyle() | wxPG_EX_MULTIPLE_SELECTION );
			pManager->Bind( wxEVT_PG_ITEM_EXPANDED, &CPropertyGridView::OnExpanded, this );
			pManager->Bind( wxEVT_PG_ITEM_COLLAPSED, &CPropertyGridView::OnCollapsed, this );
			pManager->Bind( wxEVT_PG_COL_END_DRAG, &CPropertyGridView::OnColumnDragged, this );
			pManager->Bind( wxEVT_DESTROY, &CPropertyGridView::OnDestroyed, this );
			pManager->GetGrid()->Bind( wxEVT_SET_FOCUS, &CPropertyGridView::OnFocus, this );
			pManager->Bind( wxEVT_CHILD_FOCUS, &CPropertyGridView::OnChildFocus, this );
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
			// The selection above raised no event the callback hears, so the
			// dialog is told here what the rebuilt tree has selected.
			NotifySelection();
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
				ApplyClassicLook( pGrid, pGrid->GetCellBackgroundColour() );
			}
			else
			{
				// COLOR_3DFACE behind the rows, and grey text on all of them, as
				// CPCMainTreeControl::EnableEdit and PickTextColors had it.
				const wxColour face = wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE );
				pGrid->SetCellBackgroundColour( face );
				pGrid->SetEmptySpaceColour( face );
				pGrid->SetCellTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
				ApplyClassicLook( pGrid, face );
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
			// A text box open over a row takes the clipboard commands, as the MFC
			// editors did by registering themselves as CHID_SELECTION while focused.
			if ( HandleEditorTextCommand( nCommandID ) )
			{
				return true;
			}
			switch ( nCommandID )
			{
				case ID_SELECTION_CUT:
				case ID_SELECTION_RENAME:
				case ID_SELECTION_FIND:
				case ID_SELECTION_PROPERTIES:
					return true;
				case ID_SELECTION_COPY:
					CopySelection();
					return true;
				case ID_SELECTION_PASTE:
					NPropertyPane::PasteValues( this, [this]( const std::string &rszName ) { return Find( rszName ) != nullptr; } );
					return true;
				// Insert on the keyboard: add to an array, insert before an element.
				case ID_SELECTION_NEW:
					if ( bEnableEdit )
					{
						if ( IsArrayRow( pManager->GetSelection() ) )
						{
							AddNode();
						}
						else if ( IsArrayElementRow( pManager->GetSelection() ) )
						{
							InsertNode();
						}
					}
					return true;
				// Delete on the keyboard: empty an array, delete an element.
				case ID_SELECTION_CLEAR:
					if ( bEnableEdit )
					{
						const wxPGProperty *const pSelected = pManager->GetSelection();
						if ( IsArrayRow( pSelected ) && ( pSelected->GetChildCount() > 0 ) )
						{
							DeleteAllNodes();
						}
						else if ( IsArrayElementRow( pSelected ) )
						{
							DeleteNode();
						}
					}
					return true;
				case ID_SELECTION_SELECT_ALL:
					SelectAll();
					return true;
				case ID_PC_ADD_NODE:
					if ( bEnableEdit )
					{
						AddNode();
					}
					return true;
				case ID_PC_DELETE_ALL_NODES:
					if ( bEnableEdit )
					{
						DeleteAllNodes();
					}
					return true;
				case ID_PC_INSERT_NODE:
					if ( bEnableEdit )
					{
						InsertNode();
					}
					return true;
				case ID_PC_DELETE_NODE:
					if ( bEnableEdit )
					{
						DeleteNode();
					}
					return true;
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
			if ( UpdateEditorTextCommand( nCommandID, pbEnable, pbCheck ) )
			{
				return true;
			}
			const wxPGProperty *const pSelected = pManager->GetSelection();
			switch ( nCommandID )
			{
				case ID_SELECTION_CUT:
				case ID_SELECTION_RENAME:
				case ID_SELECTION_FIND:
				case ID_SELECTION_PROPERTIES:
					return false;
				case ID_SELECTION_COPY:
					( *pbEnable ) = !pManager->GetGrid()->GetSelectedProperties().empty();
					( *pbCheck ) = false;
					return true;
				case ID_SELECTION_PASTE:
				{
					const SUserData *const pUserData = Singleton<IUserDataContainer>()->Get();
					( *pbEnable ) = ( pUserData != 0 ) && !pUserData->pcSelection.IsEmpty();
					( *pbCheck ) = false;
					return true;
				}
				case ID_SELECTION_NEW:
					if ( !bEnableEdit || ( pSelected == nullptr ) )
					{
						return false;
					}
					( *pbEnable ) = IsArrayRow( pSelected ) || IsArrayElementRow( pSelected );
					( *pbCheck ) = false;
					return true;
				case ID_SELECTION_CLEAR:
					if ( !bEnableEdit )
					{
						return false;
					}
					( *pbEnable ) = ( IsArrayRow( pSelected ) && ( pSelected->GetChildCount() > 0 ) ) || IsArrayElementRow( pSelected );
					( *pbCheck ) = false;
					return true;
				case ID_SELECTION_SELECT_ALL:
					( *pbEnable ) = true;
					( *pbCheck ) = false;
					return true;
				case ID_PC_ADD_NODE:
					if ( pSelected == nullptr )
					{
						return false;
					}
					( *pbEnable ) = bEnableEdit && IsArrayRow( pSelected );
					( *pbCheck ) = false;
					return true;
				case ID_PC_DELETE_ALL_NODES:
					if ( !IsArrayRow( pSelected ) )
					{
						return false;
					}
					( *pbEnable ) = bEnableEdit && ( pSelected->GetChildCount() > 0 );
					( *pbCheck ) = false;
					return true;
				case ID_PC_INSERT_NODE:
				case ID_PC_DELETE_NODE:
					if ( ( pSelected == nullptr ) || ( pSelected->GetParent() == nullptr ) || pSelected->GetParent()->IsRoot() )
					{
						return false;
					}
					( *pbEnable ) = bEnableEdit && IsArrayRow( pSelected->GetParent() );
					( *pbCheck ) = false;
					return true;
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
			wxPGProperty *const pRow = new CEllipsized<wxStringProperty>( name, name, wxString() );
			// A folder the tree made for a missing parent: nothing to edit.
			pRow->ChangeFlag( wxPGFlags::ReadOnly, true );
			wxPGProperty *const pAdded = ( pParent != nullptr ) ? pManager->AppendIn( pParent, pRow ) : pManager->Append( pRow );
			// After adding, which gives a row a copy of its parent's cells.
			pAdded->GetCell( 0 ).SetBitmap( TypeIcon( PCIE_FOLDER ) );
			return pAdded;
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
				pRow = new CEllipsized<wxBoolProperty>( name, name, false );
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
					pRow = new CEllipsized<wxEnumProperty>( name, name, labels );
				}
				else
				{
					pRow = new CEllipsized<wxEditEnumProperty>( name, name, labels, wxArrayInt(), wxString() );
				}
			}
			else
			{
				pRow = new CEllipsized<wxStringProperty>( name, name, wxString() );
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

		EPCIEType TypeOf( const std::string &rszName )
		{
			const SPropertyDesc *const pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( rszName ) );
			return ( pDesc != 0 ) ? typePCIEMnemonics.Get( pDesc, rszName ) : PCIE_UNKNOWN;
		}

		// The tree asked the item's type for PCIE_LIST; the name answers the same.
		bool IsArrayRow( const wxPGProperty *pProperty )
		{
			return ( pProperty != nullptr ) && ( TypeOf( FullName( pProperty ) ) == PCIE_LIST );
		}

		bool IsArrayElementRow( const wxPGProperty *pProperty )
		{
			const wxPGProperty *const pParent = ( pProperty != nullptr ) ? pProperty->GetParent() : nullptr;
			return ( pParent != nullptr ) && !pParent->IsRoot() && IsArrayRow( pParent );
		}

		// IDS_PC_DELETE_MESSAGE and IDS_PC_DELETE_ALL_MESSAGE, under the
		// application's title, No the default as MB_DEFBUTTON2 made it.
		bool Confirm( UINT nMessageID )
		{
			std::string strMessage = NResources::GetString( nMessageID );
			wxMessageDialog question( pManager, FromNarrow( strMessage ),
																FromNarrow( Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle ),
																wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
			return question.ShowModal() == wxID_YES;
		}

		void AddNode()
		{
			const wxPGProperty *const pSelected = pManager->GetSelection();
			if ( IsArrayRow( pSelected ) )
			{
				NPropertyPane::InsertNode( this, FullName( pSelected ), NODE_ADD_INDEX );
			}
		}

		void DeleteAllNodes()
		{
			const wxPGProperty *const pSelected = pManager->GetSelection();
			if ( !IsArrayRow( pSelected ) )
			{
				return;
			}
			// Named before asking: the question runs a message loop, and the row
			// may not outlive it.
			const std::string szArrayName = FullName( pSelected );
			if ( Confirm( IDS_PC_DELETE_ALL_MESSAGE ) )
			{
				NPropertyPane::RemoveNode( this, szArrayName, NODE_REMOVEALL_INDEX );
			}
		}

		// Before the selected element, which moves down one.
		void InsertNode()
		{
			const wxPGProperty *const pSelected = pManager->GetSelection();
			int nIndex = 0;
			if ( IsArrayElementRow( pSelected ) && NPropertyPane::GetNodeIndex( FullName( pSelected ), &nIndex ) )
			{
				NPropertyPane::InsertNode( this, NPropertyPane::ParentName( FullName( pSelected ) ), nIndex );
			}
		}

		void DeleteNode()
		{
			const wxPGProperty *const pSelected = pManager->GetSelection();
			int nIndex = 0;
			if ( !IsArrayElementRow( pSelected ) || !NPropertyPane::GetNodeIndex( FullName( pSelected ), &nIndex ) )
			{
				return;
			}
			// Named before asking: the question runs a message loop, and the row
			// may not outlive it.
			const std::string szArrayName = NPropertyPane::ParentName( FullName( pSelected ) );
			if ( Confirm( IDS_PC_DELETE_MESSAGE ) )
			{
				NPropertyPane::RemoveNode( this, szArrayName, nIndex );
			}
		}

		// Every selected row and all the rows under it, once each, in grid order:
		// NCA::CreateSelection's ST_COMPLETE_SELECT.
		void CollectSelected( wxPGProperty *pParent, bool bParentSelected, std::vector<wxPGProperty*> *pRows )
		{
			const wxArrayPGProperty &rSelected = pManager->GetGrid()->GetSelectedProperties();
			for ( unsigned nChild = 0; nChild < pParent->GetChildCount(); ++nChild )
			{
				wxPGProperty *const pChild = pParent->Item( nChild );
				const bool bSelected = bParentSelected || ( std::find( rSelected.begin(), rSelected.end(), pChild ) != rSelected.end() );
				if ( bSelected )
				{
					pRows->push_back( pChild );
				}
				CollectSelected( pChild, bSelected, pRows );
			}
		}

		// CPCMainTreeControl::CopySelection: the rows as NCA::FillWindowsClipboard
		// wrote them -- name, value and description, tab separated, a line each
		// -- and the fields' values kept for Paste.
		void CopySelection()
		{
			std::vector<wxPGProperty*> rows;
			CollectSelected( pManager->GetGrid()->GetRoot(), false, &rows );
			std::string szText;
			std::vector<std::string> names;
			for ( std::vector<wxPGProperty*>::const_iterator itRow = rows.begin(); itRow != rows.end(); ++itRow )
			{
				const std::string szName = FullName( *itRow );
				names.push_back( szName );
				std::string szValue;
				NPropertyPane::GetValueText( GetViewManipulator(), szName, &szValue );
				std::string szDescription;
				const SPropertyDesc *const pDesc = dynamic_cast<const SPropertyDesc*>( GetViewManipulator()->GetDesc( szName ) );
				if ( ( pDesc != 0 ) && ( szName[szName.size() - 1] != ARRAY_NODE_END_CHAR ) )
				{
					szDescription = pDesc->szDesc;
				}
				if ( !szText.empty() )
				{
					szText += "\r\n";
				}
				szText += szName + "\t" + szValue + "\t" + szDescription;
			}
			if ( wxTheClipboard->Open() )
			{
				if ( szText.empty() )
				{
					wxTheClipboard->Clear();
				}
				else
				{
					wxTheClipboard->SetData( new wxTextDataObject( FromNarrow( szText ) ) );
				}
				wxTheClipboard->Close();
			}
			NPropertyPane::CopyValues( GetViewManipulator(), names );
		}

		// NCA::SelectAll: every row whose parents are all open.
		void SelectVisibleBelow( wxPropertyGrid *pGrid, wxPGProperty *pParent )
		{
			for ( unsigned nChild = 0; nChild < pParent->GetChildCount(); ++nChild )
			{
				wxPGProperty *const pChild = pParent->Item( nChild );
				pGrid->AddToSelection( pChild );
				if ( ( pChild->GetChildCount() > 0 ) && pChild->IsExpanded() )
				{
					SelectVisibleBelow( pGrid, pChild );
				}
			}
		}

		void SelectAll()
		{
			wxPropertyGrid *const pGrid = pManager->GetGrid();
			bCreateControls = true;
			SelectVisibleBelow( pGrid, pGrid->GetRoot() );
			bCreateControls = false;
			pGrid->Refresh();
		}

		// The text box the grid has open over a row, while it has the focus.
		wxTextCtrl* FocusedEditorText() const
		{
			if ( pManager == nullptr )
			{
				return nullptr;
			}
			wxTextCtrl *const pText = wxDynamicCast( pManager->GetGrid()->GetEditorControl(), wxTextCtrl );
			return ( ( pText != nullptr ) && ( wxWindow::FindFocus() == pText ) ) ? pText : nullptr;
		}

		// CPCStringInputEditor's HandleCommand, and its siblings'.
		bool HandleEditorTextCommand( unsigned nCommandID )
		{
			wxTextCtrl *const pText = FocusedEditorText();
			if ( pText == nullptr )
			{
				return false;
			}
			switch ( nCommandID )
			{
				case ID_SELECTION_CUT:
					pText->Cut();
					return true;
				case ID_SELECTION_COPY:
					pText->Copy();
					return true;
				case ID_SELECTION_PASTE:
					pText->Paste();
					return true;
				case ID_SELECTION_CLEAR:
					pText->RemoveSelection();
					return true;
				case ID_SELECTION_SELECT_ALL:
					pText->SelectAll();
					return true;
				default:
					return false;
			}
		}

		bool UpdateEditorTextCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			const wxTextCtrl *const pText = FocusedEditorText();
			if ( pText == nullptr )
			{
				return false;
			}
			long nFrom = 0;
			long nTo = 0;
			pText->GetSelection( &nFrom, &nTo );
			switch ( nCommandID )
			{
				case ID_SELECTION_CUT:
					( *pbEnable ) = pText->CanCut();
					break;
				case ID_SELECTION_COPY:
					( *pbEnable ) = pText->CanCopy();
					break;
				case ID_SELECTION_PASTE:
					( *pbEnable ) = pText->CanPaste();
					break;
				case ID_SELECTION_CLEAR:
					( *pbEnable ) = pText->IsEditable() && ( nFrom != nTo );
					break;
				case ID_SELECTION_SELECT_ALL:
					( *pbEnable ) = ( nFrom != 0 ) || ( nTo != pText->GetLastPosition() );
					break;
				default:
					return false;
			}
			( *pbCheck ) = false;
			return true;
		}

		// Where the frame sends a command: the ranges MapEditorApp registers.
		static unsigned GetCommandHandlerFor( unsigned nCommandID )
		{
			if ( ( nCommandID >= ID_SELECTION_FIRST_COMMAND_ID ) && ( nCommandID <= ID_SELECTION_LAST_COMMAND_ID ) )
			{
				return CHID_SELECTION;
			}
			if ( ( nCommandID >= ID_PC_FIRST_COMMAND_ID ) && ( nCommandID <= ID_PC_LAST_COMMAND_ID ) )
			{
				return CHID_PROPERTY_CONTROL;
			}
			return CHID_CONTROLLER_CONTAINER;
		}

		// An entry whose state and action are the command handler container's,
		// as the tree's resource menu had them from the frame.
		void AppendCommand( wxMenu *pMenu, unsigned nCommandID, const char *pszLabel, bool bCheckable = false )
		{
			const unsigned nHandler = GetCommandHandlerFor( nCommandID );
			bool bEnable = false;
			bool bCheck = false;
			Singleton<ICommandHandlerContainer>()->UpdateCommand( nHandler, nCommandID, &bEnable, &bCheck );
			const wxString label = wxString::FromUTF8( pszLabel );
			wxMenuItem *const pItem = bCheckable ? pMenu->AppendCheckItem( nCommandID, label ) : pMenu->Append( nCommandID, label );
			pItem->Enable( bEnable );
			if ( bCheckable )
			{
				pItem->Check( bCheck );
			}
			pMenu->Bind( wxEVT_MENU,
									 [nHandler, nCommandID]( wxCommandEvent & )
									 {
										 Singleton<ICommandHandlerContainer>()->HandleCommand( nHandler, nCommandID, 0 );
									 },
									 nCommandID );
		}

		bool IsCommandEnabled( unsigned nCommandID )
		{
			bool bEnable = false;
			bool bCheck = false;
			return UpdateCommand( nCommandID, &bEnable, &bCheck ) && bEnable;
		}

		// CPCMainTreeControl::OnContextMenu. The grid has already selected the row
		// clicked. The resource has six variants; which entries a variant has is
		// decided here the way the tree chose among them.
		void OnRightClick( wxPropertyGridEvent &rEvent )
		{
			rEvent.Skip();
			if ( ( pManager == nullptr ) || ( GetViewManipulator() == 0 ) )
			{
				return;
			}
			// The tree took the focus on a right click, and the focus is what made it
			// the handler the frame sent the menu's commands to. The grid does not
			// take it on its own from a right click, so it is taken here, and the
			// handlers set as the focus would set them.
			pManager->GetGrid()->SetFocus();
			RegisterAsHandler();
			const bool bArray = IsCommandEnabled( ID_PC_ADD_NODE );
			const bool bArrayNode = IsCommandEnabled( ID_PC_INSERT_NODE );
			const bool bMultiNode = IsCommandEnabled( ID_PC_EXPAND ) || IsCommandEnabled( ID_PC_COLLAPSE );
			wxMenu menu;
			AppendCommand( &menu, ID_PC_REFRESH, "Re&fresh" );
			AppendCommand( &menu, ID_SELECTION_COPY, "&Copy" );
			AppendCommand( &menu, ID_SELECTION_PASTE, "&Paste" );
			menu.AppendSeparator();
			if ( bArray || bMultiNode )
			{
				AppendCommand( &menu, ID_PC_EXPAND, "&Expand" );
				AppendCommand( &menu, ID_PC_COLLAPSE, "&Collapse" );
				menu.AppendSeparator();
			}
			if ( bArray )
			{
				AppendCommand( &menu, ID_PC_ADD_NODE, "&Add" );
				AppendCommand( &menu, ID_PC_DELETE_ALL_NODES, "De&lete All" );
				menu.AppendSeparator();
			}
			if ( bArrayNode )
			{
				AppendCommand( &menu, ID_PC_INSERT_NODE, "&Insert" );
				AppendCommand( &menu, ID_PC_DELETE_NODE, "&Delete" );
				menu.AppendSeparator();
			}
			AppendCommand( &menu, ID_CC_UNDO, "&Undo" );
			AppendCommand( &menu, ID_CC_REDO, "&Redo" );
			menu.AppendSeparator();
			AppendCommand( &menu, ID_PC_SHOW_HIDDEN, "&Show Hidden", true );
			pManager->GetGrid()->PopupMenu( &menu );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
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
			// The type's icon before the name, as the tree's item image.
			pProperty->GetCell( 0 ).SetBitmap( TypeIcon( ( pDesc != 0 ) ? typePCIEMnemonics.Get( pDesc, rszName ) : PCIE_UNKNOWN ) );
			RefreshText( pProperty, rszName );
			if ( NPropertyPane::IsReadOnly( GetViewManipulator(), rszName ) )
			{
				pManager->SetPropertyTextColour( pProperty, wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ),
																				 wxPGPropertyValuesFlags::DontRecurse );
				// PickTextColors took a colour before the grey.
				ShowColour( pProperty, rszName );
			}
		}

		// CPCMainTreeControl::PickTextColors: a colour's value cell painted in the
		// colour, text and all, while the row is not selected -- the grid paints a
		// selected row in the selection's colours whatever its cells say. A value
		// the objects do not agree on shows as text.
		void ShowColour( wxPGProperty *pProperty, const std::string &rszName )
		{
			if ( !IsColourType( TypeOf( rszName ) ) )
			{
				return;
			}
			wxPGCell &rCell = pProperty->GetCell( 1 );
			CVariant value;
			if ( NPropertyPane::GetValue( GetViewManipulator(), rszName, &value ) && ( value.GetType() != CVariant::VT_MULTIVARIANT ) )
			{
				// GetBGRColorFromARGBColor gives COLORREF's 0x00BBGGRR, which is what
				// wxColour's unsigned long constructor reads.
				const wxColour colour( static_cast<unsigned long>( GetBGRColorFromARGBColor( (int)value ) ) );
				rCell.SetFgCol( colour );
				rCell.SetBgCol( colour );
			}
			else
			{
				const wxPGCell &rDefaultCell = pManager->GetGrid()->GetPropertyDefaultCell();
				rCell.SetFgCol( rDefaultCell.GetFgCol() );
				rCell.SetBgCol( rDefaultCell.GetBgCol() );
			}
			pManager->GetGrid()->RefreshProperty( pProperty );
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
			ShowColour( pProperty, rszName );
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
			// CPCMainTreeControl sent WM_PC_MANIPULATOR_CHANGE to its dialog here,
			// after an undo and after a redo.
			if ( changeCallback )
			{
				changeCallback();
			}
		}

		// False while the grid is too narrow for the first two columns: the grid
		// clamps a splitter to its width, and then gives later growth to the last
		// column, so widths set into a pane still being laid out stay squashed.
		//
		// And applied again after every resize, not once. The tree's columns kept
		// their widths when the tree was resized; wx scales all of them with the
		// grid. A dialog is laid out more than once before it settles -- fitted,
		// then sized to its template, then to where it was left -- and the build
		// data dialog opened with its first column squashed to 45 pixels from a
		// saved 100, having been restored at a size it was then scaled down from.
		bool ApplyColumnWidths()
		{
			if ( pManager == nullptr )
			{
				return false;
			}
			if ( !bColumnWidthsLoaded )
			{
				SDialogState state;
				NDialogState::Load( szOptionsLabel, &state );
				for ( unsigned nColumn = 0; nColumn < 2; ++nColumn )
				{
					const int nSaved = state.GetIntParameter( nColumn );
					nKeptColumnWidth[nColumn] = ( nSaved > 0 ) ? nSaved : N_DEFAULT_COLUMN_WIDTH[nColumn];
				}
				bColumnWidthsLoaded = true;
			}
			wxPropertyGrid *const pGrid = pManager->GetGrid();
			const int nClientWidth = pGrid->GetClientSize().x;
			const int nNarrowest = 16;
			if ( nClientWidth < 3 * nNarrowest )
			{
				return false;
			}
			int nFirst = nKeptColumnWidth[0];
			int nSecond = nKeptColumnWidth[0] + nKeptColumnWidth[1];
			// Narrower than the kept widths -- the docked pane usually is. The tree
			// kept its columns and the pane cut them off at its edge, the value
			// column going on out of sight. A grid column cannot run past the grid,
			// so the first keeps its width as far as there is room, the value takes
			// the rest, and the description is a sliver at the edge.
			bColumnsClipped = ( nClientWidth < nSecond + nNarrowest );
			if ( bColumnsClipped )
			{
				nSecond = nClientWidth - nNarrowest;
				nFirst = ( std::min )( nFirst, nSecond - nNarrowest );
			}
			// Both, in this order. The grid's own SetSplitterPosition moves the row
			// editor that is open with the column; the manager's does not, and left
			// a selected row's buttons where the old splitter was. The manager's
			// moves the header's columns, which the grid's does not -- outside the
			// manager's resize handler, which updates the header after it, the
			// header stayed at the squashed widths over rows at the right ones.
			pGrid->SetSplitterPosition( nFirst, 0 );
			pGrid->SetSplitterPosition( nSecond, 1 );
			pManager->SetSplitterPosition( nFirst, 0 );
			pManager->SetSplitterPosition( nSecond, 1 );
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
			// The tree could not be clicked without taking the focus, so a selected
			// row always had its commands. A wx window can be: focus is withheld
			// from a frame that is not active, as measured on a desktop with no
			// input, and a row picked there answered no command until the grid was
			// right-clicked. Selecting is as good a sign of the user's attention.
			RegisterAsHandler();
			UpdateStatus();
			NotifySelection();
		}

		void NotifySelection()
		{
			if ( !selectionCallback )
			{
				return;
			}
			const wxPGProperty *const pProperty = ( ( pManager != nullptr ) && ( GetViewManipulator() != 0 ) ) ? pManager->GetSelection() : nullptr;
			selectionCallback( ( pProperty != nullptr ) ? FullName( pProperty ) : std::string() );
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

		// What the user drags the columns to is what they keep from then on.
		void OnColumnDragged( wxPropertyGridEvent &rEvent )
		{
			rEvent.Skip();
			if ( pManager != nullptr )
			{
				const wxPropertyGrid *const pGrid = pManager->GetGrid();
				nKeptColumnWidth[0] = pGrid->GetSplitterPosition( 0 );
				nKeptColumnWidth[1] = pGrid->GetSplitterPosition( 1 ) - nKeptColumnWidth[0];
				bColumnWidthsLoaded = true;
			}
			SaveColumnWidths();
		}

		// After the grid's own resize handler, which is what rescales the
		// columns: this one runs first, so the widths go back in once the event
		// has been handled.
		void OnGridSize( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			if ( pManager != nullptr )
			{
				pManager->CallAfter( [this]()
				{
					if ( ApplyColumnWidths() )
					{
						bColumnsRestored = true;
					}
				} );
			}
		}

		void RegisterAsHandler()
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_PROPERTY_CONTROL, this );
			Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
		}

		void OnFocus( wxFocusEvent &rEvent )
		{
			rEvent.Skip();
			RegisterAsHandler();
		}

		// A click straight into a value gives the focus to the box the grid opens,
		// not to the grid, and a focus event does not travel up; this one does.
		void OnChildFocus( wxChildFocusEvent &rEvent )
		{
			rEvent.Skip();
			RegisterAsHandler();
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


	// The grid with its three titled columns, as CPCDialog and the dialogs set
	// up their trees.
	wxPropertyGridManager* CreateGridManager( wxWindow *pParent )
	{
		wxPropertyGridManager *const pManager = NWx::Child<CClassicGridManager>( pParent, wxPGMAN_DEFAULT_STYLE | wxBORDER_SUNKEN );
		pManager->AddPage();
		pManager->SetColumnCount( N_COLUMN_COUNT );
		for ( unsigned nColumn = 0; nColumn < N_COLUMN_COUNT; ++nColumn )
		{
			pManager->SetColumnTitle( nColumn, PSZ_COLUMN_TITLES[nColumn] );
		}
		pManager->ShowHeader();
		wxPropertyGrid *const pGrid = pManager->GetGrid();
		ApplyClassicLook( pGrid, pGrid->GetCellBackgroundColour() );
		// A row the font's height and a pixel either side: the tree's rows were
		// 18 pixels where wx's default spacing makes 20.
		pGrid->SetVerticalSpacing( 1 );
		// pc_header.bmp's icon beside each title, SetColumnImage( index, index ).
		// The manager does not hand out its header, which is its only
		// wxHeaderCtrl child, and whose columns are wxHeaderColumnSimple. The
		// bitmaps go into the columns alone: updating a column pushes its width
		// to the native header, and the header's widths are stale until the
		// manager works them out from the grid, which then squeezed the
		// splitters to them. SetColumnCount below has it work them out first.
		const std::vector<wxBitmap> &rHeaderIcons = Icons( IDB_PC_HEADER_IMAGE_LIST );
		const wxWindowList &rChildren = pManager->GetChildren();
		for ( wxWindowList::const_iterator itChild = rChildren.begin(); itChild != rChildren.end(); ++itChild )
		{
			wxHeaderCtrl *const pHeader = dynamic_cast<wxHeaderCtrl*>( *itChild );
			if ( pHeader == nullptr )
			{
				continue;
			}
			for ( unsigned nColumn = 0; ( nColumn < pHeader->GetColumnCount() ) && ( nColumn < rHeaderIcons.size() ); ++nColumn )
			{
				if ( const wxHeaderColumnSimple *pColumn = dynamic_cast<const wxHeaderColumnSimple*>( &CHeaderColumnAccess::Column( pHeader, nColumn ) ) )
				{
					const_cast<wxHeaderColumnSimple*>( pColumn )->SetBitmap( rHeaderIcons[nColumn] );
				}
			}
		}
		// The same count again: the widths from the grid, then every column
		// updated, bitmaps and all.
		pManager->SetColumnCount( N_COLUMN_COUNT );
		return pManager;
	}


	// The pane's contents: the grid over its status line, in a host window the
	// MFC pane can hold, registered as CHID_PC_DIALOG as CPCDialog registers
	// itself.
	class CWxPropertyPane : public IPropertyPane, public CPCBaseDialog
	{
		// A panel made in a wx window, destroyed when this goes -- after the view,
		// which is declared below it and so destroyed first, while its grid is
		// still there.
		struct CPanel
		{
			wxWeakRef<wxWindow> pWindow;
			~CPanel()
			{
				if ( pWindow )
				{
					pWindow->Destroy();
				}
			}
		};

		CPanel panel;
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

		// NPropertyPane::CreateWxIn: the contents in a panel of pParent's.
		bool CreateIn( wxWindow *pParent, IWidget *pOwner, const std::string &rszOptionsLabel )
		{
			if ( pParent == nullptr )
			{
				return false;
			}
			panel.pWindow = NWx::Child<wxPanel>( pParent, wxID_ANY );
			CreateContents( panel.pWindow, pOwner, rszOptionsLabel );
			return true;
		}

		wxWindow* GetWindow() const
		{
			return panel.pWindow;
		}

		void CreateContents( wxWindow *pRoot, IWidget *pOwner, const std::string &rszOptionsLabel )
		{
			// IDD_PC: the tree filling the pane, with a client edge, over a sunken
			// status line.
			wxPropertyGridManager *const pManager = CreateGridManager( pRoot );

			wxStaticText *const pStatus = NWx::Child<wxStaticText>( pRoot, wxID_ANY, wxString(), wxDefaultPosition,
																															wxSize( -1, pRoot->FromDIP( 18 ) ),
																															wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END | wxBORDER_SUNKEN );
			wxBoxSizer *const pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pManager, wxSizerFlags( 1 ).Expand() );
			pSizer->Add( pStatus, wxSizerFlags().Expand().Border( wxTOP, pRoot->FromDIP( 2 ) ) );
			pRoot->SetSizer( pSizer );

			view.Attach( pManager, pStatus, pOwner, rszOptionsLabel );

			ICommandHandlerContainer *const pContainer = Singleton<ICommandHandlerContainer>();
			pPreviousCommandHandler = pContainer->Get( CHID_PC_DIALOG );
			pContainer->Set( CHID_PC_DIALOG, this );
			bRegistered = true;
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


	// The grid alone, laid out by a dialog of its own.
	class CWxPropertyGrid : public NPropertyPane::IGrid
	{
		CPropertyGridView view;

	public:
		CWxPropertyGrid( wxWindow *pParent, wxStaticText *pStatus, IWidget *pOwner, const std::string &rszOptionsLabel )
		{
			view.Attach( CreateGridManager( pParent ), pStatus, pOwner, rszOptionsLabel );
		}

		virtual wxWindow* GetWindow()
		{
			return view.GetManager();
		}

		virtual CDefaultView* GetView()
		{
			return &view;
		}

		virtual ICommandHandler* GetCommandHandler()
		{
			return &view;
		}

		virtual void BuildTree()
		{
			view.BuildTree();
		}

		virtual void UpdateValues()
		{
			view.UpdateValues();
		}

		virtual void EnableEdit( bool bEnable )
		{
			view.EnableEdit( bEnable );
		}

		virtual void SetChangeCallback( const std::function<void()> &rCallback )
		{
			view.SetChangeCallback( rCallback );
		}

		virtual void SetSelectionCallback( const std::function<void( const std::string& )> &rCallback )
		{
			view.SetSelectionCallback( rCallback );
		}

		virtual bool GetColumnWidths( int *pnWidths ) const
		{
			return view.GetColumnWidths( pnWidths );
		}
	};
}


namespace NPropertyPane
{
	IGrid* CreateGridWx( wxWindow *pParent, wxStaticText *pStatus, IWidget *pOwner, const std::string &rszOptionsLabel )
	{
		return new CWxPropertyGrid( pParent, pStatus, pOwner, rszOptionsLabel );
	}


	IPropertyPane* CreateWxIn( wxWindow *pParent, IWidget *pOwner, const std::string &rszOptionsLabel, wxWindow **ppWindow )
	{
		CWxPropertyPane *const pPane = new CWxPropertyPane();
		if ( !pPane->CreateIn( pParent, pOwner, rszOptionsLabel ) )
		{
			delete pPane;
			return nullptr;
		}
		if ( ppWindow != nullptr )
		{
			( *ppWindow ) = pPane->GetWindow();
		}
		return pPane;
	}
}

