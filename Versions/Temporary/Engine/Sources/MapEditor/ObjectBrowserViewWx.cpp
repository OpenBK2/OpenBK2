#include "stdafx.h"
#include "MapEditorLib/Resources.h"

#include "ObjectBrowserView.h"


#include "ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/FolderController.h"
#include "MapEditorLib/Interface_Editor.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"
#include "Misc/StrProc.h"
#include "libdb/Db.h"
#include "libdb/Manipulator.h"
#include "libdb/ResourceManager.h"
#include "RefListView.h"
#include "SearchObjectView.h"
#include "MapEditorLib/Interface_Builder.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/Interface_FolderCallback.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/WxColourDialog.h"
#include "MapEditorLib/WxResourceImages.h"
#include "System/GlobalVars.h"
#include "port/vkcodes.h"
#include "port/wordpack.h"
#include "port/mousekeys.h"

#include <fmt/format.h>
#include <fmt/printf.h>

#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/headerctrl.h>
#include <wx/imaglist.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/utils.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/time.h>
#include <wx/treectrl.h>

#include <cstring>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

// The database browser's contents in wx, first slice: a table list over a
// wxTreeCtrl per table, browsed and not yet edited.
//
// What this slice does, each CTreeGDBBrowserBase's behaviour:
//
//   * fills a table's tree from its folder manipulator, folders before objects
//     and each kind by name ignoring case, in batches of about 16 milliseconds
//     so a large table does not stop the editor -- the object last selected in
//     the table first, and selected, as the MFC tree's timer did;
//   * shows each row's type as the MFC tree's icon, and its colour;
//   * opens the folders that were open, and remembers opening and closing, per
//     table in the user data, through an expand controller;
//   * remembers the selected object per table and hands the selected objects
//     to the Selection Properties pane (CHID_PC_DIALOG) or whatever properties
//     view the tree was given;
//   * loads an object on a double click: in a browser pane that opens its
//     editor, in the link picker it picks it;
//   * answers CHID_OBJECT_STORAGE with the selected objects, for the placing
//     tools, and follows folder changes made anywhere -- new objects, removals,
//     renames, colours, opened folders -- through the view container.
//
// And the commands that do not edit a row's name, second slice: the context
// menu, New Object at the selected folder or the root, Find, List References,
// Check and the Export variants.
//
// And the commands that edit rows, third slice: renaming in place, from the
// menu or with Space or Enter, the row then sorted among its siblings; New
// Folder at the selected folder or the root, which starts a rename; cut, copy
// and paste, into the tree's own clipboard of rows and as text onto the
// system clipboard; delete, asked first; and colour. While a row's name is
// being edited, the clipboard commands act on its text.
//
// And drag and drop, last: the selected rows dragged onto a folder are moved
// into it, or copied with Ctrl held, when enable_drag_and_drop is 1.
//
// **Names.** A row is found by its path, as the MFC tree found an HTREEITEM:
// "Scenario\Campaigns\" for a folder, "Scenario\Campaigns\GER1.0\MapInfo.xdb"
// for an object, compared ignoring case. A cache maps the lowered path to the
// row, and a walk from the root answers what the cache does not know.

namespace
{
	// CTreeGDBBrowserBase::EGDBOType, whose values are also the icons' indices
	// in tree_types.bmp: unknown, object, folder, and the same three again for a
	// selected row.
	enum EGDBOType
	{
		GDBO_UNKNOWN	= 0,
		GDBO_OBJECT		= 1,
		GDBO_FOLDER		= 2,
		GDBO_COUNT		= 3,
	};


	std::string ToNarrow( const wxString &rText )
	{
		return std::string( rText.utf8_str() );
	}


	wxString FromNarrow( const std::string &rszText )
	{
		const wxString text = wxString::FromUTF8( rszText.c_str(), rszText.size() );
		if ( text.empty() && !rszText.empty() )
		{
			return wxString( rszText.c_str(), wxConvLocal, rszText.size() );
		}
		return text;
	}


	std::string Lowered( const std::string &rszText )
	{
		std::string szLowered = rszText;
		NStr::ToLower( &szLowered );
		return szLowered;
	}


	// CTreeGDBBrowserBase::SortItemText: unknown rows first, then folders, then
	// objects, each kind by name ignoring case.
	int CompareRows( const std::string &rszText0, EGDBOType eType0, const std::string &rszText1, EGDBOType eType1 )
	{
		if ( eType0 != eType1 )
		{
			if ( eType0 == GDBO_UNKNOWN )
			{
				return -1;
			}
			if ( eType1 == GDBO_UNKNOWN )
			{
				return 1;
			}
			return ( eType0 != GDBO_FOLDER ) ? 1 : -1;
		}
		return _stricmp( rszText0.c_str(), rszText1.c_str() );
	}


	class CRowData : public wxTreeItemData
	{
	public:
		const EGDBOType eType;

		explicit CRowData( EGDBOType _eType ) : eType( _eType ) {}
	};


	// A tree whose SortChildren orders rows as CompareRows does, which is what
	// the MFC tree's TreeGDBBrowserBaseCompareFunc did after a rename. wxMSW
	// asks OnCompareItems only of a class with class info of its own; otherwise
	// it sorts by text alone.
	class CSortedTreeCtrl : public wxTreeCtrl
	{
		wxDECLARE_CLASS( CSortedTreeCtrl );

	public:
		// Sees the tree's window messages before wx does; answering true keeps
		// them from wx and the native tree. Drag and drop needs the mouse as the
		// MFC tree had it: wx's multiple-selection tree takes a button release
		// over a row for itself, changing the selection and raising no
		// wxEVT_LEFT_UP.
		std::function<bool( WXUINT, WXWPARAM, WXLPARAM )> messageHook;

		CSortedTreeCtrl( wxWindow *pParent, wxWindowID nID, const wxPoint &rPosition, const wxSize &rSize, long nStyle )
			: wxTreeCtrl( pParent, nID, rPosition, rSize, nStyle )
		{
		}

		virtual WXLRESULT MSWWindowProc( WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam ) override
		{
			if ( messageHook && messageHook( nMsg, wParam, lParam ) )
			{
				return 0;
			}
			return wxTreeCtrl::MSWWindowProc( nMsg, wParam, lParam );
		}

		virtual int OnCompareItems( const wxTreeItemId &rItem0, const wxTreeItemId &rItem1 ) override
		{
			const CRowData *const pData0 = dynamic_cast<const CRowData*>( GetItemData( rItem0 ) );
			const CRowData *const pData1 = dynamic_cast<const CRowData*>( GetItemData( rItem1 ) );
			return CompareRows( ToNarrow( GetItemText( rItem0 ) ), ( pData0 != nullptr ) ? pData0->eType : GDBO_UNKNOWN,
													ToNarrow( GetItemText( rItem1 ) ), ( pData1 != nullptr ) ? pData1->eType : GDBO_UNKNOWN );
		}
	};

	wxIMPLEMENT_CLASS( CSortedTreeCtrl, wxTreeCtrl );


	// tree_types.bmp from the editor's resources, split into its 16 pixel
	// icons with magenta as the mask, as CComboBoxGDBBrowser::InitImageLists made
	// its image list.
	wxImageList* CreateTypeImages()
	{
		wxImageList *const pImages = new wxImageList( 16, 16, true, GDBO_COUNT * 2 );
		// NWxResourceImages reads the palette bitmap out as colour; the image
		// list cuts the strip into icons.
		const wxImage strip = NWxResourceImages::LoadStrip( IDB_TABGDBB_TREE_TYPES_IMAGE_LIST );
		if ( strip.IsOk() )
		{
			pImages->Add( wxBitmap( strip ) );
		}
		return pImages;
	}


	class CWxObjectTree : public CDefaultView, public ICommandHandler, public IObjectTree
	{
		wxTreeCtrl *pTree = nullptr;
		IObjectBrowser::EKind eKind;
		IObjectBrowser::IListener *pListener;
		// What the tree's dialogs belong to: the browser's host window, which
		// outlives every tree in it.
		IWidget *pOwner;
		int nGDBBrowserID;
		unsigned nPCDialogCommandHandlerID = INVALID_COMMAND_HANDLER_ID;
		bool bEnableEdit = true;
		// Set while this view changes the tree itself, so the tree's own events
		// are not taken for the user's.
		bool bCreateControls = false;
		bool bStrongSelection = false;

		// Lowered path to row.
		std::unordered_map<std::string, wxTreeItemId> rows;

		// CSortTreeControl's clipboard: the rows cut or copied, by lowered path,
		// kept until the next cut or copy. A row that goes away leaves it.
		std::unordered_map<std::string, wxTreeItemId> clipboard;
		bool bClipboardCut = false;

		// The row whose name is being edited, and its text when the edit began.
		wxTreeItemId labelEditItem;
		std::string szLabelBeforeEdit;

		// CTreeGDBBrowserInputState: the row pressed on and where, whether a drag
		// is on, the folder it would drop into, whether the mouse has left the
		// tree, whether it copies, and the cursor it replaced.
		wxTreeItemId dragSource;
		wxPoint dragSourcePoint;
		bool bDragging = false;
		wxTreeItemId dragTarget;
		bool bDragLeft = false;
		bool bDragCopy = false;
		HCURSOR hDragDefaultCursor = 0;

		// The fill in progress: CreateTree's iterator, handed out in batches. A
		// new fill bumps the generation, which strands a batch still queued.
		CPtr<IManipulatorIterator> pBuildIterator;
		unsigned nBuildGeneration = 0;
		bool bBuildSelectionChanged = false;
		std::string szBuildFirstObject;

	public:
		CWxObjectTree( IObjectBrowser::EKind _eKind, IObjectBrowser::IListener *_pListener, IWidget *_pOwner, int _nGDBBrowserID )
			: eKind( _eKind ), pListener( _pListener ), pOwner( _pOwner ), nGDBBrowserID( _nGDBBrowserID )
		{
		}

		virtual ~CWxObjectTree()
		{
			wxTreeCtrl *const pWindow = pTree;
			StopTree();
			RemoveViewManipulator();
			// The tree calls into this view from its events, so it goes first.
			if ( pWindow != nullptr )
			{
				pWindow->Destroy();
			}
		}

		void CreateWindow( wxWindow *pParent, wxImageList *pImages )
		{
			// The MFC tree's styles: buttons, lines at the root, several rows
			// selected at once, names edited in place. The root is wx's, and hidden.
			CSortedTreeCtrl *const pSortedTree = NWx::Child<CSortedTreeCtrl>( pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																																				wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT |
																																				wxTR_MULTIPLE | wxTR_EDIT_LABELS | wxBORDER_SUNKEN );
			// The tree goes before this view does, so the hook never outlives it.
			pSortedTree->messageHook = [this]( WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam )
			{
				return OnTreeMessage( nMsg, wParam, lParam );
			};
			pTree = pSortedTree;
			pTree->Bind( wxEVT_TREE_BEGIN_LABEL_EDIT, &CWxObjectTree::OnBeginLabelEdit, this );
			pTree->Bind( wxEVT_TREE_END_LABEL_EDIT, &CWxObjectTree::OnEndLabelEdit, this );
			pTree->Bind( wxEVT_TREE_KEY_DOWN, &CWxObjectTree::OnTreeKey, this );
			pTree->SetImageList( pImages );
			pTree->AddRoot( wxString() );
			pTree->Bind( wxEVT_TREE_SEL_CHANGED, &CWxObjectTree::OnSelectionChanged, this );
			pTree->Bind( wxEVT_TREE_ITEM_EXPANDED, &CWxObjectTree::OnExpanded, this );
			pTree->Bind( wxEVT_TREE_ITEM_COLLAPSED, &CWxObjectTree::OnCollapsed, this );
			pTree->Bind( wxEVT_LEFT_DCLICK, &CWxObjectTree::OnDoubleClick, this );
			pTree->Bind( wxEVT_SET_FOCUS, &CWxObjectTree::OnFocus, this );
			pTree->Bind( wxEVT_DESTROY, &CWxObjectTree::OnDestroyed, this );
			pTree->Bind( wxEVT_TREE_ITEM_MENU, &CWxObjectTree::OnItemMenu, this );
			pTree->Bind( wxEVT_CONTEXT_MENU, &CWxObjectTree::OnContextMenu, this );
		}

		wxWindow* GetWindow() const
		{
			return pTree;
		}

		void EnableEdit( bool bEnable )
		{
			bEnableEdit = bEnable;
		}

		// IObjectTree
		virtual bool IsTreeCreated()
		{
			return GetViewManipulator() != 0;
		}

		virtual IView* GetView()
		{
			return this;
		}

		// CTreeGDBBrowserBase::CreateTree.
		virtual void CreateTree()
		{
			if ( pTree == nullptr )
			{
				return;
			}
			StopBuild();
			bCreateControls = true;
			pTree->DeleteAllItems();
			rows.clear();
			clipboard.clear();
			pTree->AddRoot( wxString() );
			bBuildSelectionChanged = false;
			szBuildFirstObject.clear();
			bCreateControls = false;
			if ( ( GetViewManipulator() == 0 ) || GetObjectSet().szObjectTypeName.empty() )
			{
				return;
			}
			// The object last selected in the table goes in first, and is
			// selected, so a large table shows it at once; the fill then skips it.
			bool bDeferred = false;
			const std::string szCurrentObject = CurrentObject();
			if ( !szCurrentObject.empty() && ( szCurrentObject[szCurrentObject.size() - 1] != PATH_SEPARATOR_CHAR ) )
			{
				szBuildFirstObject = szCurrentObject;
				const CDBID objectDBID( szCurrentObject );
				if ( NDb::DoesObjectExist( objectDBID ) && ( NDb::GetClassTypeName( objectDBID ) == GetObjectSet().szObjectTypeName ) )
				{
					const wxTreeItemId item = AddRow( pTree->GetRootItem(), szCurrentObject, GDBO_OBJECT );
					if ( item.IsOk() )
					{
						SelectOnly( item );
						SelectionChanged();
					}
				}
				bDeferred = true;
			}
			pBuildIterator = GetViewManipulator()->Iterate( true, ECT_CACHE_LOCAL );
			if ( !pBuildIterator )
			{
				UpdateSelectionManipulator( true );
				return;
			}
			if ( bDeferred )
			{
				ScheduleBuild();
			}
			else
			{
				BuildBatch( nBuildGeneration );
			}
		}

		// CTreeGDBBrowserBase::UpdateSelectionManipulator.
		virtual void UpdateSelectionManipulator( bool bUpdate )
		{
			std::string szCurrentObject;
			GetCurrentTreeItemName( &szCurrentObject );
			const bool bStrong = bStrongSelection;
			bStrongSelection = false;
			if ( bStrong && ( szCurrentObject != CurrentObject() ) )
			{
				SetCurrentTreeItemName( CurrentObject(), true );
				return;
			}
			SObjectSet objectSet;
			GetCurrentObjectSet( &objectSet );
			CPtr<IManipulator> pObjectManipulator = CManipulatorManager::CreateObectSetManipulator( objectSet );
			if ( nPCDialogCommandHandlerID != INVALID_COMMAND_HANDLER_ID )
			{
				IView *pView = 0;
				Singleton<ICommandHandlerContainer>()->HandleCommand( nPCDialogCommandHandlerID, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<uintptr_t>( &pView ) );
				if ( pView != 0 )
				{
					pView->SetViewManipulator( pObjectManipulator, objectSet, std::string() );
					if ( bUpdate )
					{
						Singleton<ICommandHandlerContainer>()->HandleCommand( nPCDialogCommandHandlerID, ID_PC_DIALOG_CREATE_TREE, 0 );
					}
				}
			}
			if ( pListener != 0 )
			{
				pListener->OnTreeSelectionChanged( this );
			}
		}

		virtual void SetPCDialogCommandHandlerID( unsigned _nPCDialogCommandHandlerID, bool bUpdate )
		{
			if ( nPCDialogCommandHandlerID != INVALID_COMMAND_HANDLER_ID )
			{
				IView *pView = 0;
				Singleton<ICommandHandlerContainer>()->HandleCommand( nPCDialogCommandHandlerID, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<uintptr_t>( &pView ) );
				if ( pView != 0 )
				{
					pView->RemoveViewManipulator();
				}
			}
			nPCDialogCommandHandlerID = _nPCDialogCommandHandlerID;
			if ( nPCDialogCommandHandlerID != INVALID_COMMAND_HANDLER_ID )
			{
				UpdateSelectionManipulator( bUpdate );
			}
		}

		virtual bool GetCurrentTreeItemName( std::string *pszName )
		{
			if ( pTree == nullptr )
			{
				return false;
			}
			wxArrayTreeItemIds selected;
			if ( pTree->GetSelections( selected ) != 1 )
			{
				return false;
			}
			if ( pszName != 0 )
			{
				( *pszName ) = NameOf( selected[0] );
			}
			return true;
		}

		virtual bool SetCurrentTreeItemName( const std::string &rszName, bool bUpdateSelection )
		{
			if ( !bUpdateSelection )
			{
				if ( !GetObjectSet().szObjectTypeName.empty() )
				{
					CurrentObject() = rszName;
				}
				return true;
			}
			const wxTreeItemId item = FindRow( rszName );
			if ( !item.IsOk() )
			{
				return false;
			}
			SelectOnly( item );
			// The MFC tree's Select raised TVN_SELCHANGED, and that is where the
			// selection was remembered and handed on.
			SelectionChanged();
			return true;
		}

		virtual void SetStrongSelection()
		{
			bStrongSelection = true;
		}

		// The selected objects, folders left out.
		bool GetCurrentObjectSet( SObjectSet *pObjectSet )
		{
			if ( ( GetViewManipulator() == 0 ) || ( pObjectSet == 0 ) || ( pTree == nullptr ) )
			{
				return false;
			}
			pObjectSet->szObjectTypeName = GetObjectSet().szObjectTypeName;
			pObjectSet->objectNameSet.clear();
			wxArrayTreeItemIds selected;
			pTree->GetSelections( selected );
			for ( size_t nRow = 0; nRow < selected.size(); ++nRow )
			{
				const std::string szName = NameOf( selected[nRow] );
				if ( !szName.empty() && ( szName[szName.size() - 1] != PATH_SEPARATOR_CHAR ) )
				{
					InsertHashSetElement( &( pObjectSet->objectNameSet ), CDBID( szName ) );
				}
			}
			return true;
		}

		// The selected row's name, folder or object, when exactly one is.
		bool GetCurrentSelectionSet( SSelectionSet *pSelectionSet )
		{
			if ( ( GetViewManipulator() == 0 ) || ( pSelectionSet == 0 ) || ( pTree == nullptr ) )
			{
				return false;
			}
			pSelectionSet->szObjectTypeName = GetObjectSet().szObjectTypeName;
			pSelectionSet->objectNameList.clear();
			wxArrayTreeItemIds selected;
			if ( pTree->GetSelections( selected ) == 1 )
			{
				const std::string szName = NameOf( selected[0] );
				if ( !szName.empty() )
				{
					pSelectionSet->objectNameList.push_back( szName );
				}
			}
			return true;
		}

		// ICommandHandler, registered as CHID_OBJECT and CHID_SELECTION while the
		// tree has the focus: CTreeGDBBrowserBase::HandleCommand.
		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			if ( pTree == nullptr )
			{
				return false;
			}
			wxTextCtrl *const pLabelText = LabelEditor();
			switch ( nCommandID )
			{
				case ID_OBJECT_LOAD:
					Load();
					return true;
				case ID_OBJECT_NEW_FOLDER:
					if ( bEnableEdit )
					{
						NewFolder( SelectedFolder() );
					}
					return true;
				case ID_OBJECT_NEW_FOLDER_AT_ROOT:
					if ( bEnableEdit )
					{
						NewFolder( pTree->GetRootItem() );
					}
					return true;
				case ID_OBJECT_NEW:
				case ID_SELECTION_NEW:
					if ( bEnableEdit )
					{
						NewObject( SelectedFolder() );
					}
					return true;
				case ID_OBJECT_NEW_AT_ROOT:
					if ( bEnableEdit )
					{
						NewObject( pTree->GetRootItem() );
					}
					return true;
				case ID_SELECTION_CUT:
					if ( bEnableEdit && !pBuildIterator )
					{
						if ( pLabelText != nullptr )
						{
							pLabelText->Cut();
						}
						else
						{
							FillClipboard( true );
						}
					}
					return true;
				case ID_SELECTION_COPY:
					if ( pLabelText != nullptr )
					{
						pLabelText->Copy();
					}
					else
					{
						FillClipboard( false );
					}
					return true;
				case ID_SELECTION_PASTE:
					if ( bEnableEdit && !pBuildIterator )
					{
						if ( pLabelText != nullptr )
						{
							pLabelText->Paste();
						}
						else
						{
							Paste();
						}
					}
					return true;
				case ID_SELECTION_CLEAR:
					if ( bEnableEdit && !pBuildIterator )
					{
						if ( pLabelText != nullptr )
						{
							// The selected text, or the character after the caret, as the
							// Delete key the MFC tree sent its edit box.
							long nFrom = 0;
							long nTo = 0;
							pLabelText->GetSelection( &nFrom, &nTo );
							if ( nFrom != nTo )
							{
								pLabelText->RemoveSelection();
							}
							else if ( nFrom < pLabelText->GetLastPosition() )
							{
								pLabelText->Remove( nFrom, nFrom + 1 );
							}
						}
						else
						{
							Delete();
						}
					}
					return true;
				case ID_SELECTION_RENAME:
					if ( bEnableEdit && !pBuildIterator )
					{
						Rename();
					}
					return true;
				case ID_SELECTION_SELECT_ALL:
					if ( pLabelText != nullptr )
					{
						pLabelText->SelectAll();
					}
					return true;
				case ID_OBJECT_COLOR:
					if ( bEnableEdit )
					{
						Color();
					}
					return true;
				case ID_OBJECT_CHECK:
				case ID_OBJECT_EXPORT:
				case ID_OBJECT_EXPORT_NO_REF:
				case ID_OBJECT_EXPORT_FORCE:
				case ID_OBJECT_EXPORT_NO_REF_FORCE:
					CheckOrExport( nCommandID );
					return true;
				case ID_SELECTION_FIND:
					Find();
					return true;
				case ID_OBJECT_REF_LOOKUP:
					LookupReferences();
					return true;
				default:
					return false;
			}
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			if ( ( pbEnable == 0 ) || ( pbCheck == 0 ) || ( pTree == nullptr ) )
			{
				return false;
			}
			( *pbCheck ) = false;
			const wxTextCtrl *const pLabelText = LabelEditor();
			const bool bNotEditing = !labelEditItem.IsOk();
			wxArrayTreeItemIds selected;
			const size_t nSelected = pTree->GetSelections( selected );
			switch ( nCommandID )
			{
				case ID_OBJECT_LOAD:
					( *pbEnable ) = CanLoad() && bNotEditing;
					return true;
				case ID_OBJECT_NEW_FOLDER:
				case ID_OBJECT_NEW_FOLDER_AT_ROOT:
				case ID_OBJECT_NEW:
				case ID_OBJECT_NEW_AT_ROOT:
				case ID_SELECTION_NEW:
					( *pbEnable ) = bEnableEdit && CanNew() && bNotEditing;
					return true;
				case ID_SELECTION_CUT:
				case ID_SELECTION_COPY:
				{
					bool bAllowed = ( nCommandID == ID_SELECTION_COPY ) || ( bEnableEdit && !pBuildIterator );
					if ( pLabelText != nullptr )
					{
						long nFrom = 0;
						long nTo = 0;
						pLabelText->GetSelection( &nFrom, &nTo );
						bAllowed = bAllowed && ( nFrom != nTo );
					}
					else
					{
						bAllowed = bAllowed && bNotEditing && ( nSelected > 0 );
					}
					( *pbEnable ) = bAllowed;
					return true;
				}
				case ID_SELECTION_PASTE:
					if ( pLabelText != nullptr )
					{
						( *pbEnable ) = bEnableEdit && !pBuildIterator && pLabelText->CanPaste();
					}
					else
					{
						( *pbEnable ) = bEnableEdit && bNotEditing && CanNew() && ( nSelected == 1 ) && !clipboard.empty() &&
														!IsClipboardRow( PasteTarget() );
					}
					return true;
				case ID_SELECTION_CLEAR:
					( *pbEnable ) = bEnableEdit && !pBuildIterator && ( ( pLabelText != nullptr ) || ( bNotEditing && ( nSelected > 0 ) ) );
					return true;
				case ID_SELECTION_RENAME:
					( *pbEnable ) = bEnableEdit && CanNew() && ( nSelected == 1 ) && bNotEditing;
					return true;
				case ID_SELECTION_SELECT_ALL:
					// The MFC tree answered this one as not its own, so the frame kept
					// it greyed; its edit box took Ctrl+A itself.
					( *pbEnable ) = ( pLabelText != nullptr );
					return false;
				case ID_OBJECT_COLOR:
					( *pbEnable ) = bEnableEdit && ( nSelected > 0 ) && bNotEditing;
					return true;
				case ID_OBJECT_CHECK:
				case ID_OBJECT_EXPORT:
				case ID_OBJECT_EXPORT_NO_REF:
				case ID_OBJECT_EXPORT_FORCE:
				case ID_OBJECT_EXPORT_NO_REF_FORCE:
					( *pbEnable ) = CanExport() && bNotEditing;
					return true;
				case ID_SELECTION_FIND:
					( *pbEnable ) = bNotEditing;
					return true;
				case ID_OBJECT_REF_LOOKUP:
					( *pbEnable ) = ( nSelected == 1 ) && bNotEditing && ( TypeOf( pTree->GetFocusedItem() ) == GDBO_OBJECT );
					return true;
				default:
					return false;
			}
		}

		// IView, through CDefaultView. CTreeGDBBrowserBase::InternalRedo: a folder
		// controller's changes, made by this tree or anything else on the table.
		// Undo changes nothing here, as the MFC tree's did not: folder
		// controllers are absolute.
		virtual void Undo( IController *pController )
		{
		}

		virtual void Redo( IController *pController )
		{
			CFolderController *const pFolderController = dynamic_cast<CFolderController*>( pController );
			if ( ( pFolderController == 0 ) || ( GetViewManipulator() == 0 ) || ( pTree == nullptr ) || pBuildIterator )
			{
				return;
			}
			for ( CFolderController::CUndoDataList::const_iterator itUndoData = pFolderController->undoDataList.begin();
						itUndoData != pFolderController->undoDataList.end(); ++itUndoData )
			{
				switch ( itUndoData->eType )
				{
					case CFolderController::SUndoData::TYPE_INSERT:
					case CFolderController::SUndoData::TYPE_COPY:
						if ( !itUndoData->szDestination.empty() )
						{
							GetViewManipulator()->ClearCache();
							AddRow( pTree->GetRootItem(), itUndoData->szDestination, TypeOfName( itUndoData->szDestination ) );
						}
						break;
					case CFolderController::SUndoData::TYPE_REMOVE:
					{
						const wxTreeItemId item = FindRow( itUndoData->szDestination );
						if ( item.IsOk() && ( pTree->GetChildrenCount( item, false ) == 0 ) )
						{
							GetViewManipulator()->ClearCache();
							DeleteRow( item );
						}
						break;
					}
					case CFolderController::SUndoData::TYPE_RENAME:
						if ( !itUndoData->szDestination.empty() && !itUndoData->szSource.empty() )
						{
							GetViewManipulator()->ClearCache();
							RenameRow( itUndoData->szDestination, itUndoData->szSource, (bool)( itUndoData->newValue ) );
						}
						break;
					case CFolderController::SUndoData::TYPE_COLOR:
					{
						const wxTreeItemId item = FindRow( itUndoData->szDestination );
						if ( item.IsOk() )
						{
							GetViewManipulator()->ClearCache();
							SetRowColour( item, (int)( itUndoData->newValue ) );
						}
						break;
					}
					case CFolderController::SUndoData::TYPE_EXPAND:
					{
						const wxTreeItemId item = FindRow( itUndoData->szDestination );
						if ( item.IsOk() )
						{
							bCreateControls = true;
							if ( (bool)( itUndoData->newValue ) )
							{
								pTree->Expand( item );
							}
							else
							{
								pTree->Collapse( item );
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

	private:
		std::string& CurrentObject()
		{
			return Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].szCurrentObject;
		}

		EGDBOType TypeOf( const wxTreeItemId &rItem ) const
		{
			const CRowData *const pData = rItem.IsOk() ? dynamic_cast<const CRowData*>( pTree->GetItemData( rItem ) ) : nullptr;
			return ( pData != nullptr ) ? pData->eType : GDBO_UNKNOWN;
		}

		static EGDBOType TypeOfName( const std::string &rszName )
		{
			return ( rszName[rszName.size() - 1] != PATH_SEPARATOR_CHAR ) ? GDBO_OBJECT : GDBO_FOLDER;
		}

		// CTreeGDBBrowserBase::GetTreeItemName: the row's text under its folders',
		// with a separator after each folder.
		std::string NameOf( wxTreeItemId item ) const
		{
			std::string szName;
			const wxTreeItemId root = pTree->GetRootItem();
			for ( ; item.IsOk() && ( item != root ); item = pTree->GetItemParent( item ) )
			{
				const std::string szText = ToNarrow( pTree->GetItemText( item ) );
				switch ( TypeOf( item ) )
				{
					case GDBO_OBJECT:
						szName = szText + szName;
						break;
					case GDBO_FOLDER:
						szName = szText + PATH_SEPARATOR_CHAR + szName;
						break;
					default:
						break;
				}
			}
			return szName;
		}

		wxTreeItemId FindChild( const wxTreeItemId &rParent, const std::string &rszText ) const
		{
			const std::string szLowered = Lowered( rszText );
			wxTreeItemIdValue cookie;
			for ( wxTreeItemId child = pTree->GetFirstChild( rParent, cookie ); child.IsOk(); child = pTree->GetNextChild( rParent, cookie ) )
			{
				if ( Lowered( ToNarrow( pTree->GetItemText( child ) ) ) == szLowered )
				{
					return child;
				}
			}
			return wxTreeItemId();
		}

		// CTreeGDBBrowserBase::GetTreeItem: the cache, then a walk.
		wxTreeItemId FindRow( const std::string &rszName ) const
		{
			if ( rszName.empty() || ( pTree == nullptr ) )
			{
				return wxTreeItemId();
			}
			const std::unordered_map<std::string, wxTreeItemId>::const_iterator posRow = rows.find( Lowered( rszName ) );
			if ( posRow != rows.end() )
			{
				return posRow->second;
			}
			std::string szRest = rszName;
			if ( szRest[szRest.size() - 1] == PATH_SEPARATOR_CHAR )
			{
				szRest.erase( szRest.size() - 1 );
			}
			wxTreeItemId item = pTree->GetRootItem();
			while ( item.IsOk() )
			{
				const size_t nDivider = szRest.find( PATH_SEPARATOR_CHAR );
				item = FindChild( item, szRest.substr( 0, nDivider ) );
				if ( nDivider == std::string::npos )
				{
					break;
				}
				szRest = szRest.substr( nDivider + 1 );
			}
			return item;
		}

		// CTreeGDBBrowserBase::FindPlaceToInsert and InsertTreeItem: before the
		// first child that does not sort before it.
		wxTreeItemId InsertRow( const wxTreeItemId &rParent, const std::string &rszText, EGDBOType eType )
		{
			wxTreeItemId previous;
			wxTreeItemIdValue cookie;
			for ( wxTreeItemId child = pTree->GetFirstChild( rParent, cookie ); child.IsOk(); child = pTree->GetNextChild( rParent, cookie ) )
			{
				if ( CompareRows( ToNarrow( pTree->GetItemText( child ) ), TypeOf( child ), rszText, eType ) >= 0 )
				{
					break;
				}
				previous = child;
			}
			const wxString text = FromNarrow( rszText );
			const wxTreeItemId item = previous.IsOk()
																? pTree->InsertItem( rParent, previous, text, eType, eType + GDBO_COUNT, new CRowData( eType ) )
																: pTree->PrependItem( rParent, text, eType, eType + GDBO_COUNT, new CRowData( eType ) );
			if ( item.IsOk() )
			{
				const std::string szName = NameOf( item );
				rows[Lowered( szName )] = item;
				int nColour = 0;
				if ( CManipulatorManager::GetValue( &nColour, GetViewManipulator(), szName ) )
				{
					SetRowColour( item, nColour );
				}
			}
			return item;
		}

		// CTreeGDBBrowserBase::AddTreeItem: the folders on the way made as they
		// are needed.
		wxTreeItemId AddRow( const wxTreeItemId &rParent, const std::string &rszName, EGDBOType eType )
		{
			if ( rszName.empty() )
			{
				return wxTreeItemId();
			}
			const size_t nDivider = rszName.find( PATH_SEPARATOR_CHAR );
			const std::string szShortName = rszName.substr( 0, nDivider );
			if ( nDivider == std::string::npos )
			{
				const wxTreeItemId existing = FindChild( rParent, szShortName );
				if ( existing.IsOk() && ( TypeOf( existing ) == eType ) )
				{
					return existing;
				}
				return InsertRow( rParent, szShortName, eType );
			}
			wxTreeItemId folder = FindChild( rParent, szShortName );
			if ( !folder.IsOk() )
			{
				folder = InsertRow( rParent, szShortName, GDBO_FOLDER );
			}
			const std::string szRest = rszName.substr( nDivider + 1 );
			return szRest.empty() ? folder : AddRow( folder, szRest, eType );
		}

		void ForgetRows( const std::string &rszName )
		{
			const std::string szPrefix = Lowered( rszName );
			for ( std::unordered_map<std::string, wxTreeItemId>::iterator itRow = rows.begin(); itRow != rows.end(); )
			{
				if ( itRow->first.compare( 0, szPrefix.size(), szPrefix ) == 0 )
				{
					itRow = rows.erase( itRow );
				}
				else
				{
					++itRow;
				}
			}
		}

		void DeleteRow( const wxTreeItemId &rItem )
		{
			ForgetRows( NameOf( rItem ) );
			// A cut or copied row inside the one going is dropped from the
			// clipboard while its id can still be walked.
			for ( std::unordered_map<std::string, wxTreeItemId>::iterator itEntry = clipboard.begin(); itEntry != clipboard.end(); )
			{
				if ( IsWithin( itEntry->second, rItem ) )
				{
					itEntry = clipboard.erase( itEntry );
				}
				else
				{
					++itEntry;
				}
			}
			if ( labelEditItem.IsOk() && IsWithin( labelEditItem, rItem ) )
			{
				labelEditItem = wxTreeItemId();
			}
			if ( dragSource.IsOk() && IsWithin( dragSource, rItem ) )
			{
				dragSource = wxTreeItemId();
			}
			if ( dragTarget.IsOk() && IsWithin( dragTarget, rItem ) )
			{
				dragTarget = wxTreeItemId();
			}
			pTree->Delete( rItem );
		}

		// Whether rItem is rAncestor or lies under it.
		bool IsWithin( wxTreeItemId item, const wxTreeItemId &rAncestor ) const
		{
			for ( ; item.IsOk(); item = pTree->GetItemParent( item ) )
			{
				if ( item == rAncestor )
				{
					return true;
				}
			}
			return false;
		}

		// TYPE_RENAME: a move adds the new row and takes the old one away once
		// it is empty; a rename in place changes the row's text.
		void RenameRow( const std::string &rszDestination, const std::string &rszSource, bool bNewRow )
		{
			if ( bNewRow )
			{
				AddRow( pTree->GetRootItem(), rszDestination, TypeOfName( rszDestination ) );
				const wxTreeItemId source = FindRow( rszSource );
				if ( source.IsOk() && ( pTree->GetChildrenCount( source, false ) == 0 ) )
				{
					DeleteRow( source );
				}
				return;
			}
			const wxTreeItemId source = FindRow( rszSource );
			if ( !source.IsOk() )
			{
				return;
			}
			const std::string szParentName = NameOf( pTree->GetItemParent( source ) );
			std::string szText = rszDestination.substr( std::min( szParentName.size(), rszDestination.size() ) );
			if ( !szText.empty() && ( szText[szText.size() - 1] == PATH_SEPARATOR_CHAR ) )
			{
				szText.erase( szText.size() - 1 );
			}
			ForgetRows( rszSource );
			pTree->SetItemText( source, FromNarrow( szText ) );
			RememberRows( source );
		}

		void RememberRows( const wxTreeItemId &rItem )
		{
			rows[Lowered( NameOf( rItem ) )] = rItem;
			wxTreeItemIdValue cookie;
			for ( wxTreeItemId child = pTree->GetFirstChild( rItem, cookie ); child.IsOk(); child = pTree->GetNextChild( rItem, cookie ) )
			{
				RememberRows( child );
			}
		}

		// CTreeGDBBrowserBase::PickTextColors: the colour the folder keeps for the
		// row, 0x00BBGGRR; black draws as the tree's own text colour.
		void SetRowColour( const wxTreeItemId &rItem, int nColour )
		{
			if ( nColour != 0 )
			{
				pTree->SetItemTextColour( rItem, wxColour( static_cast<unsigned long>( nColour ) ) );
			}
			else
			{
				pTree->SetItemTextColour( rItem, wxNullColour );
			}
		}

		void StopBuild()
		{
			++nBuildGeneration;
			pBuildIterator = 0;
		}

		void StopTree()
		{
			// wxTreeCtrl deletes its selected items after wxEVT_DESTROY and can
			// still send selection/focus events. Stop callbacks before releasing
			// the view: the frame may already be gone, and this tree must not
			// register itself as a command handler again while being destroyed.
			if ( pTree != nullptr )
			{
				pTree->SetEvtHandlerEnabled( false );
				pTree->DeletePendingEvents();
				// The native message hook also becomes inert when pTree is null.
				pTree = nullptr;
			}
			StopBuild();
			ICommandHandlerContainer *const pContainer = Singleton<ICommandHandlerContainer>();
			pContainer->Remove( CHID_OBJECT, this );
			pContainer->Remove( CHID_SELECTION, this );
		}

		void ScheduleBuild()
		{
			const unsigned nGeneration = nBuildGeneration;
			pTree->CallAfter( [this, nGeneration]()
			{
				BuildBatch( nGeneration );
			} );
		}

		// CTreeGDBBrowserBase::OnCreateTreeTimer.
		void BuildBatch( unsigned nGeneration )
		{
			if ( ( nGeneration != nBuildGeneration ) || ( pTree == nullptr ) || !pBuildIterator )
			{
				return;
			}
			const wxLongLong nStart = wxGetLocalTimeMillis();
			int nCount = 0;
			const std::string szSkipped = Lowered( szBuildFirstObject );
			pTree->Freeze();
			while ( !pBuildIterator->IsEnd() && ( ( nCount == 0 ) || ( wxGetLocalTimeMillis() - nStart < 16 ) ) )
			{
				std::string szName;
				std::string szType;
				pBuildIterator->GetName( &szName );
				pBuildIterator->GetType( &szType );
				if ( szSkipped.empty() || ( Lowered( szName ) != szSkipped ) )
				{
					const EGDBOType eType = ( szType == "object" ) ? GDBO_OBJECT : ( ( szType == "folder" ) ? GDBO_FOLDER : GDBO_UNKNOWN );
					if ( eType != GDBO_UNKNOWN )
					{
						AddRow( pTree->GetRootItem(), szName, eType );
					}
				}
				pBuildIterator->Next();
				++nCount;
			}
			pTree->Thaw();
			if ( !pBuildIterator->IsEnd() )
			{
				ScheduleBuild();
				return;
			}
			pBuildIterator = 0;
			bCreateControls = true;
			OpenRememberedFolders( pTree->GetRootItem() );
			wxArrayTreeItemIds selected;
			if ( pTree->GetSelections( selected ) > 0 )
			{
				pTree->EnsureVisible( selected[0] );
			}
			bCreateControls = false;
			if ( !bBuildSelectionChanged && szBuildFirstObject.empty() )
			{
				UpdateSelectionManipulator( true );
			}
		}

		// The folders the user left open, per table: AddTreeItem set their state
		// as it made them, which a wx tree cannot do for a row with no children
		// yet, so it is done once the rows are in.
		void OpenRememberedFolders( const wxTreeItemId &rParent )
		{
			const SUserData::SObjectTypeData::CExpandedObjectSet &rExpanded =
				Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].expandedObjectSet;
			wxTreeItemIdValue cookie;
			for ( wxTreeItemId child = pTree->GetFirstChild( rParent, cookie ); child.IsOk(); child = pTree->GetNextChild( rParent, cookie ) )
			{
				if ( ( TypeOf( child ) != GDBO_FOLDER ) || !pTree->ItemHasChildren( child ) )
				{
					continue;
				}
				if ( rExpanded.find( NameOf( child ) ) != rExpanded.end() )
				{
					pTree->Expand( child );
				}
				OpenRememberedFolders( child );
			}
		}

		void SelectOnly( const wxTreeItemId &rItem )
		{
			bCreateControls = true;
			pTree->UnselectAll();
			pTree->SelectItem( rItem );
			pTree->SetFocusedItem( rItem );
			pTree->EnsureVisible( rItem );
			bCreateControls = false;
		}

		// CTreeGDBBrowserBase::OnSelChanged.
		void SelectionChanged()
		{
			bBuildSelectionChanged = true;
			if ( !GetObjectSet().szObjectTypeName.empty() )
			{
				const wxTreeItemId focused = pTree->GetFocusedItem();
				wxArrayTreeItemIds selected;
				const bool bAnySelected = ( pTree->GetSelections( selected ) > 0 );
				CurrentObject() = ( bAnySelected && focused.IsOk() ) ? NameOf( focused ) : std::string();
			}
			UpdateSelectionManipulator( true );
		}

		void RegisterAsHandler()
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_OBJECT, this );
			Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
			if ( nGDBBrowserID != -1 )
			{
				Singleton<IMainFrameContainer>()->Get()->SaveObjectStorage( nGDBBrowserID );
			}
		}

		static std::string LoadResourceString( UINT nID )
		{
			return NResources::GetString( nID );
		}

		// Where the frame sends a command: the ranges MapEditorApp registers.
		// The menu has object and selection commands only.
		static unsigned GetCommandHandlerFor( unsigned nCommandID )
		{
			if ( ( nCommandID >= ID_SELECTION_FIRST_COMMAND_ID ) && ( nCommandID <= ID_SELECTION_LAST_COMMAND_ID ) )
			{
				return CHID_SELECTION;
			}
			return CHID_OBJECT;
		}

		// The selected rows none of whose folders are selected too, in tree
		// order: CSortTreeControl::IsTopSelection over the selection.
		void GetTopSelection( std::vector<wxTreeItemId> *pRows ) const
		{
			const wxTreeItemId root = pTree->GetRootItem();
			wxArrayTreeItemIds selected;
			pTree->GetSelections( selected );
			for ( size_t nRow = 0; nRow < selected.size(); ++nRow )
			{
				bool bTop = true;
				for ( wxTreeItemId parent = pTree->GetItemParent( selected[nRow] ); parent.IsOk() && ( parent != root ); parent = pTree->GetItemParent( parent ) )
				{
					if ( pTree->IsSelected( parent ) )
					{
						bTop = false;
						break;
					}
				}
				if ( bTop )
				{
					pRows->push_back( selected[nRow] );
				}
			}
		}

		// The folder New puts an object in: the focused row, or the folder it is
		// in; the root when there is none.
		wxTreeItemId SelectedFolder() const
		{
			const wxTreeItemId root = pTree->GetRootItem();
			wxTreeItemId item = pTree->GetFocusedItem();
			while ( item.IsOk() && ( item != root ) && ( TypeOf( item ) != GDBO_FOLDER ) )
			{
				item = pTree->GetItemParent( item );
			}
			return ( item.IsOk() && ( item != root ) ) ? item : root;
		}

		// CTreeGDBBrowserBase::FindName with the type checked.
		bool HasChild( const wxTreeItemId &rParent, const std::string &rszText, EGDBOType eType ) const
		{
			const wxTreeItemId child = FindChild( rParent, rszText );
			return child.IsOk() && ( TypeOf( child ) == eType );
		}

		// CTreeGDBBrowserBase::GetUniqueName: "Name (2)", "Name (3)" and on, the
		// .xdb kept at the end.
		std::string UniqueName( const wxTreeItemId &rParent, const std::string &rszName, EGDBOType eType ) const
		{
			std::string szName = rszName;
			std::string szBaseName = rszName;
			const bool bExtension = CStringManager::CutFileExtention( &szBaseName, ".xdb" );
			for ( uint32_t nNumber = 2; HasChild( rParent, szName, eType ); ++nNumber )
			{
				szName = szBaseName + fmt::format( " ({})", nNumber );
				if ( bExtension )
				{
					CStringManager::ExtendFileExtention( &szName, ".xdb" );
				}
			}
			return szName;
		}

		bool CanNew() const
		{
			return !pBuildIterator && Singleton<IBuilderContainer>()->CanDefaultBuildObject( GetObjectSet().szObjectTypeName );
		}

		// CTreeGDBBrowserBase::CanExport, which asked the same of both kinds.
		bool CanExport() const
		{
			return Singleton<IExporterContainer>()->CanExportObject( DEFAULT_EXPORTER_LABEL_TXT ) && pTree->GetFocusedItem().IsOk();
		}

		// CTreeGDBBrowser's CanAutoLoadAfterBuildingObject, and the link tree's.
		bool CanAutoLoad() const
		{
			return eKind == IObjectBrowser::KIND_BROWSER;
		}

		// CTreeGDBBrowserBase::New( HTREEITEM ): an object named "New Resource"
		// under rParent, made by the builder, exported if the builder says so,
		// and opened if it says that.
		void NewObject( const wxTreeItemId &rParent )
		{
			if ( GetViewManipulator() == 0 )
			{
				return;
			}
			std::string szObjectTypeName = GetObjectSet().szObjectTypeName;
			std::string szObjectName = UniqueName( rParent, LoadResourceString( IDS_TREE_GDB_BROWSE_NEW_RESOURCE ), GDBO_OBJECT );
			CStringManager::ExtendFileExtention( &szObjectName, ".xdb" );
			szObjectName = NameOf( rParent ) + szObjectName;
			bool bCanChangeObjectName = true;
			bool bNeedEdit = true;
			bool bNeedExport = false;
			Singleton<IFolderCallback>()->ClearUndoData();
			if ( !Singleton<IBuilderContainer>()->InsertObject( &szObjectTypeName, &szObjectName, false,
																													&bCanChangeObjectName, &bNeedExport, &bNeedEdit ) )
			{
				Singleton<IFolderCallback>()->UndoChanges();
				return;
			}
			// The insert redid a folder controller into this view, which added the
			// row; the tree may have gone with a dialog in between.
			const wxTreeItemId item = ( pTree != nullptr ) ? FindRow( szObjectName ) : wxTreeItemId();
			if ( item.IsOk() )
			{
				pTree->EnsureVisible( item );
				// The table's first object: nothing else will hand the selection on.
				if ( pTree->GetCount() == 1 )
				{
					SelectionChanged();
				}
				if ( bNeedExport )
				{
					if ( CPtr<IManipulator> pObjectManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szObjectTypeName, szObjectName ) )
					{
						IExporterContainer *const pExporters = Singleton<IExporterContainer>();
						pExporters->StartExport( szObjectTypeName, FORCE_EXPORT, START_EXPORT_TOOLS, EXPORT_REFERENCES );
						pExporters->ExportObject( pObjectManipulator, szObjectTypeName, szObjectName, FORCE_EXPORT, EXPORT_REFERENCES );
						pExporters->FinishExport( szObjectTypeName, FORCE_EXPORT, FINISH_EXPORT_TOOLS, EXPORT_REFERENCES );
					}
				}
				if ( CanAutoLoad() && bNeedEdit )
				{
					SelectOnly( item );
					SelectionChanged();
					if ( CanLoad() )
					{
						Load();
					}
				}
			}
			Singleton<IFolderCallback>()->ClearUndoData();
		}

		// CTreeGDBBrowserBase::FindFirstItem: depth first, starting after rStart
		// or at the top, the first row whose path holds the text ignoring case,
		// with '/' read as '\'.
		wxTreeItemId FindFirstRow( const std::string &rszSearch, const wxTreeItemId &rStart ) const
		{
			if ( rszSearch.empty() )
			{
				return wxTreeItemId();
			}
			std::string szSearch = Lowered( rszSearch );
			NStr::ReplaceAllChars( &szSearch, '/', '\\' );
			const wxTreeItemId root = pTree->GetRootItem();
			wxTreeItemIdValue cookie;
			wxTreeItemId item = ( rStart.IsOk() && ( rStart != root ) ) ? rStart : pTree->GetFirstChild( root, cookie );
			while ( item.IsOk() )
			{
				if ( ( item != rStart ) && ( Lowered( NameOf( item ) ).find( szSearch ) != std::string::npos ) )
				{
					return item;
				}
				wxTreeItemIdValue childCookie;
				wxTreeItemId next = pTree->GetFirstChild( item, childCookie );
				for ( wxTreeItemId climb = item; !next.IsOk() && climb.IsOk() && ( climb != root ); climb = pTree->GetItemParent( climb ) )
				{
					next = pTree->GetNextSibling( climb );
				}
				item = next;
			}
			return wxTreeItemId();
		}

		// CTreeGDBBrowserBase::Find.
		void Find()
		{
			std::string szSearch = Singleton<IUserDataContainer>()->Get()->szLastSearchedText;
			if ( !NSearchObject::Run( pOwner, &szSearch ) || ( pTree == nullptr ) )
			{
				return;
			}
			Singleton<IUserDataContainer>()->Get()->szLastSearchedText = szSearch;
			const wxTreeItemId focused = pTree->GetFocusedItem();
			wxTreeItemId found = FindFirstRow( szSearch, focused );
			if ( !found.IsOk() )
			{
				found = FindFirstRow( szSearch, wxTreeItemId() );
			}
			if ( found.IsOk() )
			{
				// As the tree did, only a single selection moves to what was found.
				wxArrayTreeItemIds selected;
				if ( ( pTree->GetSelections( selected ) == 1 ) && ( found != focused ) )
				{
					SelectOnly( found );
					SelectionChanged();
				}
			}
			else
			{
				const std::string szMessage = fmt::sprintf( LoadResourceString( IDS_TREE_GDB_BROWSE_NO_OBJECT_FOUND_MESSAGE ), GetObjectSet().szObjectTypeName );
				wxMessageDialog message( pTree, FromNarrow( szMessage ), FromNarrow( LoadResourceString( AFX_IDS_APP_TITLE ) ),
																 wxOK | wxICON_INFORMATION );
				message.ShowModal();
			}
			pTree->SetFocus();
		}

		// CTreeGDBBrowserBase::LookupReferences: the focused row's references,
		// scanned and then listed.
		void LookupReferences()
		{
			wxBusyCursor busy;
			const std::string szObjectTypeName = GetObjectSet().szObjectTypeName;
			const std::string szObjectName = NameOf( pTree->GetFocusedItem() );
			std::list<std::string> referenceObjects;
			if ( NRefList::RunScan( pOwner, szObjectTypeName, szObjectName, &referenceObjects ) )
			{
				NRefList::Run( pOwner, szObjectTypeName, szObjectName, &referenceObjects );
			}
			if ( pTree != nullptr )
			{
				pTree->SetFocus();
			}
		}

		// ExecuteTreeOperation's TYPE_CHECK and TYPE_EXPORT: a row's children
		// first, then the row itself when it is an object.
		void ForEachObject( const wxTreeItemId &rItem, const std::function<void( const std::string& )> &rAction )
		{
			std::vector<wxTreeItemId> children;
			wxTreeItemIdValue cookie;
			for ( wxTreeItemId child = pTree->GetFirstChild( rItem, cookie ); child.IsOk(); child = pTree->GetNextChild( rItem, cookie ) )
			{
				children.push_back( child );
			}
			for ( size_t nChild = 0; nChild < children.size(); ++nChild )
			{
				ForEachObject( children[nChild], rAction );
			}
			if ( TypeOf( rItem ) == GDBO_OBJECT )
			{
				rAction( NameOf( rItem ) );
			}
		}

		// CTreeGDBBrowserBase::Check.
		void Check( bool bCheckReferences )
		{
			if ( GetViewManipulator() == 0 )
			{
				return;
			}
			wxBusyCursor busy;
			const std::string szObjectTypeName = GetObjectSet().szObjectTypeName;
			IExporterContainer *const pExporters = Singleton<IExporterContainer>();
			Singleton<IFolderCallback>()->ClearUndoData();
			pExporters->StartCheck( szObjectTypeName, START_EXPORT_TOOLS, bCheckReferences );
			std::vector<wxTreeItemId> rows;
			GetTopSelection( &rows );
			for ( size_t nRow = 0; nRow < rows.size(); ++nRow )
			{
				ForEachObject( rows[nRow], [&]( const std::string &rszObjectName )
				{
					if ( CPtr<IManipulator> pObjectManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szObjectTypeName, rszObjectName ) )
					{
						pExporters->CheckObject( pObjectManipulator, szObjectTypeName, rszObjectName, bCheckReferences );
					}
				} );
			}
			pExporters->FinishCheck( szObjectTypeName, FINISH_EXPORT_TOOLS, bCheckReferences );
			Singleton<IFolderCallback>()->ClearUndoData();
			UpdateSelectionManipulator( true );
			pTree->SetFocus();
		}

		// CTreeGDBBrowserBase::Export.
		void Export( bool bForce, bool bExportReferences )
		{
			if ( GetViewManipulator() == 0 )
			{
				return;
			}
			wxBusyCursor busy;
			const std::string szObjectTypeName = GetObjectSet().szObjectTypeName;
			IExporterContainer *const pExporters = Singleton<IExporterContainer>();
			Singleton<IFolderCallback>()->ClearUndoData();
			pExporters->StartExport( szObjectTypeName, bForce, START_EXPORT_TOOLS, bExportReferences );
			std::vector<wxTreeItemId> rows;
			GetTopSelection( &rows );
			for ( size_t nRow = 0; nRow < rows.size(); ++nRow )
			{
				ForEachObject( rows[nRow], [&]( const std::string &rszObjectName )
				{
					if ( CPtr<IManipulator> pObjectManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szObjectTypeName, rszObjectName ) )
					{
						pExporters->ExportObject( pObjectManipulator, szObjectTypeName, rszObjectName, bForce, bExportReferences );
					}
				} );
			}
			pExporters->FinishExport( szObjectTypeName, bForce, FINISH_EXPORT_TOOLS, bExportReferences );
			Singleton<IEditorContainer>()->ReloadActiveEditor( true );
			Singleton<IFolderCallback>()->ClearUndoData();
			UpdateSelectionManipulator( true );
			pTree->SetFocus();
		}

		// The warning each of check and export asks under, No the default. An
		// empty message asks nothing, as in the tree.
		bool AskYesNo( UINT nMessageID )
		{
			const std::string szMessage = LoadResourceString( nMessageID );
			if ( szMessage.empty() )
			{
				return true;
			}
			wxMessageDialog question( pTree, FromNarrow( szMessage ),
																FromNarrow( Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle ),
																wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
			return question.ShowModal() == wxID_YES;
		}

		// The check and export commands. Holding Shift leaves references out of
		// the ones that would follow them.
		void CheckOrExport( unsigned nCommandID )
		{
			const bool bForce = ( nCommandID == ID_OBJECT_EXPORT_FORCE ) || ( nCommandID == ID_OBJECT_EXPORT_NO_REF_FORCE );
			const bool bReferences = ( ( nCommandID == ID_OBJECT_CHECK ) || ( nCommandID == ID_OBJECT_EXPORT ) || ( nCommandID == ID_OBJECT_EXPORT_FORCE ) ) &&
															 !wxGetKeyState( WXK_SHIFT );
			UINT nMessageID = IDS_TREE_GDB_BROWSE_EXPORT_WARNING;
			switch ( nCommandID )
			{
				case ID_OBJECT_CHECK:
					nMessageID = IDS_TREE_GDB_BROWSE_CHECK_WARNING;
					break;
				case ID_OBJECT_EXPORT_NO_REF:
					nMessageID = IDS_TREE_GDB_BROWSE_EXPORT_NO_REF_WARNING;
					break;
				case ID_OBJECT_EXPORT_FORCE:
					nMessageID = IDS_TREE_GDB_BROWSE_EXPORT_FORCE_WARNING;
					break;
				case ID_OBJECT_EXPORT_NO_REF_FORCE:
					nMessageID = IDS_TREE_GDB_BROWSE_EXPORT_NO_REF_FORCE_WARNING;
					break;
				default:
					break;
			}
			if ( !AskYesNo( nMessageID ) )
			{
				return;
			}
			if ( nCommandID == ID_OBJECT_CHECK )
			{
				Check( bReferences );
			}
			else
			{
				Export( bForce, bReferences );
			}
		}

		// An entry whose state and action are the command handler container's,
		// as the resource menu had them from the frame.
		void AppendCommand( wxMenu *pMenu, unsigned nCommandID, const std::string &rszLabel )
		{
			const unsigned nHandler = GetCommandHandlerFor( nCommandID );
			bool bEnable = false;
			bool bCheck = false;
			Singleton<ICommandHandlerContainer>()->UpdateCommand( nHandler, nCommandID, &bEnable, &bCheck );
			wxMenuItem *const pItem = pMenu->Append( nCommandID, FromNarrow( rszLabel ) );
			pItem->Enable( bEnable );
			pMenu->Bind( wxEVT_MENU,
									 [nHandler, nCommandID]( wxCommandEvent & )
									 {
										 Singleton<ICommandHandlerContainer>()->HandleCommand( nHandler, nCommandID, 0 );
									 },
									 nCommandID );
		}

		// IDM_MAIN_CONTEXT_MENU's TREE_GDB_BROWSER popup, its first entry named
		// for what loading does in this kind of tree.
		void ShowContextMenu()
		{
			RegisterAsHandler();
			wxMenu menu;
			AppendCommand( &menu, ID_OBJECT_LOAD,
										 LoadResourceString( ( eKind == IObjectBrowser::KIND_LINK ) ? IDS_TREE_GDB_LINK_BROWSE_SELECT : IDS_TREE_GDB_BROWSE_LOAD ) );
			AppendCommand( &menu, ID_OBJECT_REF_LOOKUP, "List &References..." );
			menu.AppendSeparator();
			AppendCommand( &menu, ID_OBJECT_NEW_FOLDER, "New &Folder" );
			AppendCommand( &menu, ID_OBJECT_NEW, "&New Object" );
			menu.AppendSeparator();
			AppendCommand( &menu, ID_OBJECT_NEW_FOLDER_AT_ROOT, "New Root F&older" );
			AppendCommand( &menu, ID_OBJECT_NEW_AT_ROOT, "Ne&w Root Object" );
			menu.AppendSeparator();
			AppendCommand( &menu, ID_SELECTION_CUT, "Cu&t" );
			AppendCommand( &menu, ID_SELECTION_COPY, "&Copy" );
			AppendCommand( &menu, ID_SELECTION_PASTE, "&Paste" );
			AppendCommand( &menu, ID_SELECTION_CLEAR, "&Delete" );
			AppendCommand( &menu, ID_SELECTION_RENAME, "Rena&me" );
			menu.AppendSeparator();
			AppendCommand( &menu, ID_SELECTION_SELECT_ALL, "Select &All" );
			menu.AppendSeparator();
			AppendCommand( &menu, ID_OBJECT_EXPORT_NO_REF, "&Export" );
			AppendCommand( &menu, ID_OBJECT_EXPORT_NO_REF_FORCE, "Force E&xport" );
			wxMenu *const pHierarchical = new wxMenu();
			AppendCommand( pHierarchical, ID_OBJECT_CHECK, "C&heck" );
			AppendCommand( pHierarchical, ID_OBJECT_EXPORT, "&Export" );
			AppendCommand( pHierarchical, ID_OBJECT_EXPORT_FORCE, "Force E&xport" );
			menu.AppendSubMenu( pHierarchical, "Hierarchical Export" );
			menu.AppendSeparator();
			AppendCommand( &menu, ID_SELECTION_FIND, "&Find..." );
			pTree->PopupMenu( &menu );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		}

		// A right click on a row: the row is selected first when it is not, as
		// the tree's PrepareContextMenu did.
		void OnItemMenu( wxTreeEvent &rEvent )
		{
			const wxTreeItemId item = rEvent.GetItem();
			if ( item.IsOk() && !pTree->IsSelected( item ) )
			{
				SelectOnly( item );
				SelectionChanged();
			}
			pTree->SetFocus();
			ShowContextMenu();
		}

		// A right click below the rows, where no row menu is raised.
		void OnContextMenu( wxContextMenuEvent &rEvent )
		{
			const wxPoint at = rEvent.GetPosition();
			if ( at != wxDefaultPosition )
			{
				int nFlags = 0;
				if ( pTree->HitTest( pTree->ScreenToClient( at ), nFlags ).IsOk() )
				{
					return;
				}
			}
			pTree->SetFocus();
			ShowContextMenu();
		}

		CFolderController* NewFolderController()
		{
			return CreateController<CFolderController>( static_cast<CFolderController*>( 0 ) );
		}

		// The text box of the row being renamed, while there is one.
		wxTextCtrl* LabelEditor() const
		{
			return ( ( pTree != nullptr ) && labelEditItem.IsOk() ) ? pTree->GetEditControl() : nullptr;
		}

		std::vector<std::string> ChildNames( const wxTreeItemId &rItem ) const
		{
			std::vector<std::string> names;
			wxTreeItemIdValue cookie;
			for ( wxTreeItemId child = pTree->GetFirstChild( rItem, cookie ); child.IsOk(); child = pTree->GetNextChild( rItem, cookie ) )
			{
				names.push_back( NameOf( child ) );
			}
			return names;
		}

		// A folder's row by its name, the root for "".
		wxTreeItemId FolderRow( const std::string &rszName ) const
		{
			return rszName.empty() ? pTree->GetRootItem() : FindRow( rszName );
		}

		// CTreeGDBBrowserBase::NewFolder( HTREEITEM ): "New Folder", numbered when
		// taken, made under rParent and its name opened for editing.
		void NewFolder( const wxTreeItemId &rParent )
		{
			if ( ( GetViewManipulator() == 0 ) || pBuildIterator )
			{
				return;
			}
			const std::string szName = NameOf( rParent ) + UniqueName( rParent, LoadResourceString( IDS_TREE_GDB_BROWSE_NEW_FOLDER ), GDBO_FOLDER ) +
																 PATH_SEPARATOR_CHAR;
			CPtr<CFolderController> pFolderController = NewFolderController();
			pFolderController->AddInsertOperation( szName );
			// This view is left out of the replay and adds the row itself.
			if ( !pFolderController->Redo( true, true, this ) || ( pTree == nullptr ) )
			{
				return;
			}
			GetViewManipulator()->ClearCache();
			const wxTreeItemId item = AddRow( pTree->GetRootItem(), szName, GDBO_FOLDER );
			if ( item.IsOk() )
			{
				pTree->EnsureVisible( item );
				pTree->EditLabel( item );
			}
		}

		// CTreeGDBBrowserBase::Rename: the focused row's name, edited in place.
		void Rename()
		{
			if ( ( GetViewManipulator() == 0 ) || pBuildIterator )
			{
				return;
			}
			const wxTreeItemId item = pTree->GetFocusedItem();
			if ( item.IsOk() )
			{
				pTree->EnsureVisible( item );
				pTree->EditLabel( item );
			}
		}

		// CSortTreeControl::IsClipboardItem: whether the row is a cut or copied
		// one, or lies inside one.
		bool IsClipboardRow( const wxTreeItemId &rItem ) const
		{
			if ( !rItem.IsOk() || ( rItem == pTree->GetRootItem() ) )
			{
				return false;
			}
			for ( std::unordered_map<std::string, wxTreeItemId>::const_iterator itEntry = clipboard.begin(); itEntry != clipboard.end(); ++itEntry )
			{
				if ( IsWithin( rItem, itEntry->second ) )
				{
					return true;
				}
			}
			return false;
		}

		// Where paste puts rows: the focused folder, or the folder the focused
		// object is in.
		wxTreeItemId PasteTarget() const
		{
			const wxTreeItemId root = pTree->GetRootItem();
			wxTreeItemId item = pTree->GetFocusedItem();
			while ( item.IsOk() && ( item != root ) && ( TypeOf( item ) == GDBO_OBJECT ) )
			{
				item = pTree->GetItemParent( item );
			}
			return item.IsOk() ? item : root;
		}

		// CSortTreeControl::FillWindowsClipboard: a line for each row with nothing
		// under it, "Type<tab>path".
		void AppendClipboardText( const wxTreeItemId &rItem, std::string *pszText ) const
		{
			if ( pTree->GetChildrenCount( rItem, false ) > 0 )
			{
				wxTreeItemIdValue cookie;
				for ( wxTreeItemId child = pTree->GetFirstChild( rItem, cookie ); child.IsOk(); child = pTree->GetNextChild( rItem, cookie ) )
				{
					AppendClipboardText( child, pszText );
				}
				return;
			}
			const std::string &rszPrefix = GetObjectSet().szObjectTypeName;
			( *pszText ) += ( rszPrefix.empty() ? std::string() : ( rszPrefix + "\t" ) ) + NameOf( rItem ) + "\r\n";
		}

		// CSortTreeControl::FillClipboard, for Cut and Copy: the selected rows no
		// selected folder holds, remembered for Paste and put on the system
		// clipboard as text.
		void FillClipboard( bool bCut )
		{
			if ( GetViewManipulator() == 0 )
			{
				return;
			}
			clipboard.clear();
			std::string szText;
			std::vector<wxTreeItemId> rows;
			GetTopSelection( &rows );
			for ( size_t nRow = 0; nRow < rows.size(); ++nRow )
			{
				clipboard[Lowered( NameOf( rows[nRow] ) )] = rows[nRow];
				AppendClipboardText( rows[nRow], &szText );
			}
			bClipboardCut = bCut;
			if ( wxTheClipboard->Open() )
			{
				wxTheClipboard->SetData( new wxTextDataObject( FromNarrow( szText ) ) );
				wxTheClipboard->Close();
			}
			pTree->SetFocus();
		}

		// ExecuteTreeOperation's TYPE_COPY: the row rszSource copied into the
		// folder rszFolder. An object whose name is taken there is numbered; a
		// folder is numbered only when copied beside itself, and otherwise merged
		// into the folder of that name. Rows are followed by name, since each step
		// adds and removes rows.
		void CopyRow( const std::string &rszFolder, const std::string &rszSource )
		{
			const wxTreeItemId source = FindRow( rszSource );
			const wxTreeItemId folder = FolderRow( rszFolder );
			if ( !source.IsOk() || !folder.IsOk() )
			{
				return;
			}
			const std::string szText = ToNarrow( pTree->GetItemText( source ) );
			const EGDBOType eType = TypeOf( source );
			const std::vector<std::string> children = ChildNames( source );
			const wxTreeItemId existing = FindChild( folder, szText );
			std::string szDestination = rszFolder;
			bool bNeedCopy = true;
			if ( !existing.IsOk() || ( TypeOf( existing ) != eType ) )
			{
				szDestination += szText;
				if ( eType == GDBO_FOLDER )
				{
					szDestination += PATH_SEPARATOR_CHAR;
				}
			}
			else if ( eType == GDBO_OBJECT )
			{
				szDestination += UniqueName( folder, szText, eType );
			}
			else if ( NameOf( existing ) == rszSource )
			{
				szDestination += UniqueName( folder, szText, eType ) + PATH_SEPARATOR_CHAR;
			}
			else
			{
				szDestination = NameOf( existing );
				bNeedCopy = false;
			}
			if ( bNeedCopy )
			{
				if ( eType == GDBO_FOLDER )
				{
					CPtr<CFolderController> pFolderController = NewFolderController();
					pFolderController->AddCopyOperation( szDestination, rszSource );
					pFolderController->Redo( true, true, 0 );
				}
				else
				{
					Singleton<IFolderCallback>()->ClearUndoData();
					if ( !Singleton<IBuilderContainer>()->CopyObject( GetObjectSet().szObjectTypeName, szDestination, rszSource ) )
					{
						Singleton<IFolderCallback>()->UndoChanges();
					}
					Singleton<IFolderCallback>()->ClearUndoData();
				}
			}
			if ( ( eType == GDBO_FOLDER ) && ( pTree != nullptr ) && FindRow( szDestination ).IsOk() )
			{
				for ( size_t nChild = 0; nChild < children.size(); ++nChild )
				{
					CopyRow( szDestination, children[nChild] );
				}
			}
		}

		// ExecuteTreeOperation's TYPE_RENAME, for a cut row pasted: an object is
		// renamed into the folder, numbered when its name is taken there; a folder
		// is made there, or found there, its rows moved into it one by one, and
		// the emptied folder removed.
		void MoveRow( const std::string &rszFolder, const std::string &rszSource )
		{
			const wxTreeItemId source = FindRow( rszSource );
			const wxTreeItemId folder = FolderRow( rszFolder );
			if ( !source.IsOk() || !folder.IsOk() )
			{
				return;
			}
			const std::string szText = ToNarrow( pTree->GetItemText( source ) );
			const EGDBOType eType = TypeOf( source );
			const std::vector<std::string> children = ChildNames( source );
			const wxTreeItemId existing = FindChild( folder, szText );
			std::string szDestination = rszFolder;
			if ( !existing.IsOk() || ( TypeOf( existing ) != eType ) )
			{
				szDestination += szText;
				if ( eType == GDBO_FOLDER )
				{
					szDestination += PATH_SEPARATOR_CHAR;
					CPtr<CFolderController> pFolderController = NewFolderController();
					pFolderController->AddInsertOperation( szDestination );
					pFolderController->Redo( true, true, 0 );
				}
			}
			else if ( NameOf( existing ) == rszSource )
			{
				// Pasted where it already is.
				return;
			}
			else if ( eType == GDBO_OBJECT )
			{
				szDestination += UniqueName( folder, szText, eType );
			}
			else
			{
				szDestination = NameOf( existing );
			}
			if ( eType == GDBO_OBJECT )
			{
				Singleton<IFolderCallback>()->ClearUndoData();
				if ( !Singleton<IBuilderContainer>()->RenameObject( GetObjectSet().szObjectTypeName, szDestination, rszSource ) )
				{
					Singleton<IFolderCallback>()->UndoChanges();
				}
				Singleton<IFolderCallback>()->ClearUndoData();
				return;
			}
			if ( ( pTree == nullptr ) || !FindRow( szDestination ).IsOk() )
			{
				return;
			}
			for ( size_t nChild = 0; nChild < children.size(); ++nChild )
			{
				MoveRow( szDestination, children[nChild] );
			}
			CPtr<CFolderController> pFolderController = NewFolderController();
			pFolderController->AddRemoveOperation( rszSource );
			pFolderController->Redo( true, true, 0 );
		}

		// CTreeGDBBrowserBase::Paste: the clipboard's rows copied, or moved when
		// they were cut, into the paste target, unless it is one of them.
		void Paste()
		{
			if ( ( GetViewManipulator() == 0 ) || pBuildIterator )
			{
				return;
			}
			const wxTreeItemId target = PasteTarget();
			if ( !IsClipboardRow( target ) )
			{
				const std::string szFolder = NameOf( target );
				std::vector<std::string> sources;
				for ( std::unordered_map<std::string, wxTreeItemId>::const_iterator itEntry = clipboard.begin(); itEntry != clipboard.end(); ++itEntry )
				{
					sources.push_back( NameOf( itEntry->second ) );
				}
				const bool bMove = bClipboardCut;
				for ( size_t nSource = 0; ( nSource < sources.size() ) && ( pTree != nullptr ); ++nSource )
				{
					if ( bMove )
					{
						MoveRow( szFolder, sources[nSource] );
					}
					else
					{
						CopyRow( szFolder, sources[nSource] );
					}
				}
				UpdateSelectionManipulator( true );
			}
			if ( pTree != nullptr )
			{
				pTree->SetFocus();
			}
		}

		// ExecuteTreeOperation's TYPE_REMOVE: a row's rows first, then the row once
		// nothing is left under it -- a folder through the folder controller, an
		// object through its builder unless it is locked.
		void RemoveRow( const std::string &rszName )
		{
			wxTreeItemId item = FindRow( rszName );
			if ( !item.IsOk() )
			{
				return;
			}
			const std::vector<std::string> children = ChildNames( item );
			for ( size_t nChild = 0; nChild < children.size(); ++nChild )
			{
				RemoveRow( children[nChild] );
			}
			item = ( pTree != nullptr ) ? FindRow( rszName ) : wxTreeItemId();
			if ( !item.IsOk() || ( pTree->GetChildrenCount( item, false ) > 0 ) )
			{
				return;
			}
			const std::string &rszObjectTypeName = GetObjectSet().szObjectTypeName;
			if ( TypeOf( item ) == GDBO_FOLDER )
			{
				CPtr<CFolderController> pFolderController = NewFolderController();
				pFolderController->AddRemoveOperation( rszName );
				pFolderController->Redo( true, true, 0 );
				return;
			}
			Singleton<IFolderCallback>()->ClearUndoData();
			if ( !Singleton<IFolderCallback>()->IsObjectLocked( rszObjectTypeName, rszName ) )
			{
				if ( !Singleton<IBuilderContainer>()->RemoveObject( rszObjectTypeName, rszName ) )
				{
					Singleton<IFolderCallback>()->UndoChanges();
				}
			}
			else
			{
				NLog::Log( LT_IMPORTANT, "Can't remove object. Object is locked. %s:%s\n", rszObjectTypeName.c_str(), rszName.c_str() );
			}
			Singleton<IFolderCallback>()->ClearUndoData();
		}

		// CTreeGDBBrowserBase::Delete: asked first, No by default.
		void Delete()
		{
			if ( ( GetViewManipulator() == 0 ) || pBuildIterator )
			{
				return;
			}
			if ( AskYesNo( IDS_TREE_GDB_BROWSE_DELETE_OBJECTS_MESSAGE ) && ( pTree != nullptr ) )
			{
				std::vector<wxTreeItemId> rows;
				GetTopSelection( &rows );
				std::vector<std::string> names;
				for ( size_t nRow = 0; nRow < rows.size(); ++nRow )
				{
					names.push_back( NameOf( rows[nRow] ) );
				}
				for ( size_t nName = 0; ( nName < names.size() ) && ( pTree != nullptr ); ++nName )
				{
					RemoveRow( names[nName] );
				}
				UpdateSelectionManipulator( true );
			}
			if ( pTree != nullptr )
			{
				pTree->SetFocus();
			}
		}

		// CTreeGDBBrowserBase::Color: one colour for every selected row, started
		// on the row's own when one is selected.
		void Color()
		{
			if ( ( GetViewManipulator() == 0 ) || pBuildIterator )
			{
				return;
			}
			wxArrayTreeItemIds selected;
			pTree->GetSelections( selected );
			std::vector<std::string> names;
			for ( size_t nRow = 0; nRow < selected.size(); ++nRow )
			{
				names.push_back( NameOf( selected[nRow] ) );
			}
			int nStartColour = 0;
			if ( names.size() == 1 )
			{
				CManipulatorManager::GetValue( &nStartColour, GetViewManipulator(), names[0] );
			}
			wxColour chosen;
			if ( NWxColourDialog::Pick( pOwner, wxColour( static_cast<unsigned long>( nStartColour ) ), &chosen ) )
			{
				const int nColour = static_cast<int>( chosen.GetRGB() );
				for ( size_t nName = 0; nName < names.size(); ++nName )
				{
					CPtr<CFolderController> pFolderController = NewFolderController();
					pFolderController->AddColorOperation( names[nName], nColour );
					pFolderController->Redo( true, true, 0 );
				}
			}
			if ( pTree != nullptr )
			{
				pTree->Refresh();
			}
		}

		// CTreeGDBBrowserBase::OnBeginLabelEdit: only where editing is allowed,
		// and one row at a time.
		void OnBeginLabelEdit( wxTreeEvent &rEvent )
		{
			if ( !bEnableEdit || labelEditItem.IsOk() || !rEvent.GetItem().IsOk() )
			{
				rEvent.Veto();
				return;
			}
			labelEditItem = rEvent.GetItem();
			szLabelBeforeEdit = ToNarrow( pTree->GetItemText( labelEditItem ) );
		}

		// CTreeGDBBrowserBase::OnEndLabelEdit: a new name no sibling has renames
		// the row through a folder controller, which every view on the table
		// replays, this one included; the row is then sorted among its siblings.
		// Anything else puts the old name back.
		void OnEndLabelEdit( wxTreeEvent &rEvent )
		{
			const wxTreeItemId item = rEvent.GetItem();
			const std::string szBefore = szLabelBeforeEdit;
			labelEditItem = wxTreeItemId();
			szLabelBeforeEdit.clear();
			bool bRenamed = false;
			if ( !rEvent.IsEditCancelled() && item.IsOk() && ( GetViewManipulator() != 0 ) )
			{
				const std::string szAfter = ToNarrow( rEvent.GetLabel() );
				const wxTreeItemId parent = pTree->GetItemParent( item );
				bool bTaken = false;
				wxTreeItemIdValue cookie;
				for ( wxTreeItemId sibling = pTree->GetFirstChild( parent, cookie ); sibling.IsOk(); sibling = pTree->GetNextChild( parent, cookie ) )
				{
					if ( ( sibling != item ) && ( Lowered( ToNarrow( pTree->GetItemText( sibling ) ) ) == Lowered( szAfter ) ) )
					{
						bTaken = true;
						break;
					}
				}
				if ( !szAfter.empty() && ( szAfter != szBefore ) && !bTaken )
				{
					const std::string szParentName = NameOf( parent );
					std::string szDestination = szParentName + szAfter;
					std::string szSource = szParentName + szBefore;
					if ( TypeOf( item ) == GDBO_FOLDER )
					{
						szDestination += PATH_SEPARATOR_CHAR;
						szSource += PATH_SEPARATOR_CHAR;
					}
					CPtr<CFolderController> pFolderController = NewFolderController();
					pFolderController->AddRenameOperation( szDestination, szSource, false );
					bRenamed = pFolderController->Redo( true, true, 0 );
					if ( bRenamed && ( pTree != nullptr ) )
					{
						GetViewManipulator()->ClearCache();
						pTree->EnsureVisible( item );
						UpdateSelectionManipulator( true );
						SortLater( szParentName );
					}
				}
			}
			if ( !bRenamed )
			{
				rEvent.Veto();
			}
			if ( pTree != nullptr )
			{
				pTree->SetFocus();
			}
		}

		// OnLabelEditSortTimer: the folder's rows sorted once the edit has ended.
		void SortLater( const std::string &rszFolder )
		{
			pTree->CallAfter( [this, rszFolder]()
			{
				const wxTreeItemId folder = ( pTree != nullptr ) ? FolderRow( rszFolder ) : wxTreeItemId();
				if ( folder.IsOk() )
				{
					bCreateControls = true;
					pTree->SortChildren( folder );
					bCreateControls = false;
				}
			} );
		}

		// CTreeGDBBrowser::CanLoad and CTreeGDBLinkBrowser::CanLoad.
		bool CanLoad()
		{
			wxArrayTreeItemIds selected;
			if ( ( pTree == nullptr ) || ( pTree->GetSelections( selected ) != 1 ) || ( TypeOf( selected[0] ) != GDBO_OBJECT ) )
			{
				return false;
			}
			if ( eKind == IObjectBrowser::KIND_LINK )
			{
				return true;
			}
			return Singleton<IEditorContainer>()->CanCreate( GetObjectSet().szObjectTypeName ) ||
						 ( Singleton<IEditorContainer>()->GetActiveEditor() != 0 );
		}

		// CTreeGDBBrowser::Load and CTreeGDBLinkBrowser::Load.
		void Load()
		{
			if ( ( eKind == IObjectBrowser::KIND_BROWSER ) && ( GetViewManipulator() != 0 ) )
			{
				SObjectSet objectSet;
				if ( GetCurrentObjectSet( &objectSet ) && !objectSet.objectNameSet.empty() )
				{
					if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
					{
						return;
					}
					if ( Singleton<IEditorContainer>()->CanCreate( objectSet.szObjectTypeName ) )
					{
						if ( CPtr<IManipulator> pObjectManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first ) )
						{
							Singleton<IEditorContainer>()->Create( pObjectManipulator, objectSet );
						}
					}
					else if ( Singleton<IEditorContainer>()->GetActiveEditor() != 0 )
					{
						Singleton<IEditorContainer>()->DestroyActiveEditor( false );
						Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
					}
				}
			}
			if ( pListener != 0 )
			{
				pListener->OnTreeLoad( this );
			}
		}

		// Remembered, and told to the other views on the table, which an expand
		// controller replays without adding to the undo list.
		void NoteExpanded( const wxTreeItemId &rItem, bool bExpanded )
		{
			if ( bCreateControls || GetObjectSet().szObjectTypeName.empty() || !rItem.IsOk() )
			{
				return;
			}
			bCreateControls = true;
			const std::string szName = NameOf( rItem );
			SUserData::SObjectTypeData::CExpandedObjectSet &rExpanded =
				Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[GetObjectSet().szObjectTypeName].expandedObjectSet;
			if ( bExpanded )
			{
				InsertHashSetElement( &rExpanded, szName );
			}
			else
			{
				SUserData::SObjectTypeData::CExpandedObjectSet::iterator posExpanded = rExpanded.find( szName );
				if ( posExpanded != rExpanded.end() )
				{
					rExpanded.erase( posExpanded );
				}
			}
			CPtr<CFolderController> pFolderController = CreateController<CFolderController>( static_cast<CFolderController*>( 0 ) );
			pFolderController->AddExpandOperation( szName, bExpanded );
			pFolderController->Redo( true, true, this );
			bCreateControls = false;
		}

		void OnSelectionChanged( wxTreeEvent &rEvent )
		{
			rEvent.Skip();
			if ( bCreateControls )
			{
				return;
			}
			// A click gives the tree the focus, which registers it; a frame that
			// is not active withholds the focus, and the row picked there should
			// still answer its commands.
			RegisterAsHandler();
			SelectionChanged();
		}

		void OnExpanded( wxTreeEvent &rEvent )
		{
			rEvent.Skip();
			NoteExpanded( rEvent.GetItem(), true );
		}

		void OnCollapsed( wxTreeEvent &rEvent )
		{
			rEvent.Skip();
			NoteExpanded( rEvent.GetItem(), false );
		}

		// CTreeGDBBrowserInputState::OnLButtonDblClk: a double click on a row's
		// icon or label loads it. The tree still opens or closes a folder on it.
		void OnDoubleClick( wxMouseEvent &rEvent )
		{
			rEvent.Skip();
			int nFlags = 0;
			const wxTreeItemId item = pTree->HitTest( rEvent.GetPosition(), nFlags );
			if ( item.IsOk() && ( ( nFlags & ( wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL ) ) != 0 ) )
			{
				Load();
			}
		}

		// CTreeGDBBrowserInputState's drag and drop, which followed the mouse
		// itself rather than the tree's own drag. Pressed on a row's icon or name
		// and moved more than four pixels -- or with the right button pressed too
		// -- the selected rows are dragged, the mouse captured. A folder under the
		// mouse that is not selected and not inside a selected row becomes the
		// target, highlighted, and stays it until another does. Releasing moves
		// the rows into it, or copies them with Ctrl held; Escape ends the drag
		// without either. The cursor shows move, copy, or no drop outside the
		// tree.
		bool OnTreeMessage( WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam )
		{
			if ( pTree == nullptr )
			{
				return false;
			}
			const wxPoint at( static_cast<short>( LOWORD( lParam ) ), static_cast<short>( HIWORD( lParam ) ) );
			switch ( nMsg )
			{
				case WM_LBUTTONDOWN:
				{
					int nFlags = 0;
					dragSource = pTree->HitTest( at, nFlags );
					if ( dragSource.IsOk() && ( ( nFlags & ( wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL ) ) == 0 ) )
					{
						dragSource = wxTreeItemId();
					}
					dragSourcePoint = at;
					return false;
				}
				case WM_MOUSEMOVE:
					if ( ( wParam & MK_LBUTTON ) == 0 )
					{
						return false;
					}
					if ( bDragging )
					{
						int nFlags = 0;
						const wxTreeItemId item = pTree->HitTest( at, nFlags );
						if ( item.IsOk() && ( ( nFlags & ( wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL ) ) != 0 ) &&
								 ( item != dragTarget ) && !pTree->IsSelected( item ) && IsTopSelection( item, dragTarget ) &&
								 ( TypeOf( item ) == GDBO_FOLDER ) )
						{
							if ( dragTarget.IsOk() )
							{
								pTree->SetItemDropHighlight( dragTarget, false );
							}
							dragTarget = item;
							pTree->SetItemDropHighlight( dragTarget, true );
						}
						ContinueDrag( at, ( wParam & MK_CONTROL ) != 0 );
						return true;
					}
					if ( dragSource.IsOk() )
					{
						const int nDX = dragSourcePoint.x - at.x;
						const int nDY = dragSourcePoint.y - at.y;
						if ( nDX * nDX + nDY * nDY > 16 )
						{
							BeginDrag( ( wParam & MK_CONTROL ) != 0 );
						}
					}
					return bDragging;
				case WM_RBUTTONDOWN:
					if ( !bDragging && ( ( wParam & MK_LBUTTON ) != 0 ) && dragSource.IsOk() )
					{
						BeginDrag( ( wParam & MK_CONTROL ) != 0 );
					}
					return bDragging;
				case WM_LBUTTONUP:
					if ( bDragging )
					{
						EndDrag( true );
						return true;
					}
					return false;
				case WM_KEYDOWN:
					if ( bDragging && ( wParam == VK_ESCAPE ) )
					{
						EndDrag( false );
						return true;
					}
					if ( bDragging && ( wParam == VK_CONTROL ) )
					{
						ContinueDrag( pTree->ScreenToClient( wxGetMousePosition() ), true );
					}
					return false;
				case WM_KEYUP:
					if ( bDragging && ( wParam == VK_CONTROL ) )
					{
						ContinueDrag( pTree->ScreenToClient( wxGetMousePosition() ), false );
					}
					return false;
				case WM_CAPTURECHANGED:
					// Another window took the mouse: nothing is dropped.
					if ( bDragging && ( reinterpret_cast<HWND>( lParam ) != pTree->GetHWND() ) )
					{
						EndDrag( false );
					}
					return false;
				case WM_CONTEXTMENU:
					return bDragging;
				default:
					return false;
			}
		}

		// CSortTreeControl::IsTopSelection: no selected folder above the row but
		// rSkip.
		bool IsTopSelection( const wxTreeItemId &rItem, const wxTreeItemId &rSkip ) const
		{
			const wxTreeItemId root = pTree->GetRootItem();
			for ( wxTreeItemId parent = pTree->GetItemParent( rItem ); parent.IsOk() && ( parent != root ); parent = pTree->GetItemParent( parent ) )
			{
				if ( ( parent != rSkip ) && pTree->IsSelected( parent ) )
				{
					return false;
				}
			}
			return true;
		}

		// IDC_DRAG_AND_DROP_MOVE or _COPY, built once from the generated tables.
		//
		// The handle rather than the wxCursor, because the drag around it is
		// still Win32 -- SetCapture, SetCursor and the tree's HWND -- and taking
		// that apart is a separate job from where the cursor comes from.
		static HCURSOR DragCursor( bool bCopy )
		{
			static const wxCursor moveCursor = NWxResourceImages::LoadCursorResource( IDC_DRAG_AND_DROP_MOVE );
			static const wxCursor copyCursor = NWxResourceImages::LoadCursorResource( IDC_DRAG_AND_DROP_COPY );
			const wxCursor &rCursor = bCopy ? copyCursor : moveCursor;
			return rCursor.IsOk() ? reinterpret_cast<HCURSOR>( rCursor.GetHCURSOR() ) : 0;
		}

		void BeginDrag( bool bCopy )
		{
			if ( ( NGlobal::GetVar( "enable_drag_and_drop", 0 ) != 1 ) || !bEnableEdit )
			{
				return;
			}
			bDragging = true;
			bDragLeft = false;
			bDragCopy = false;
			::SetCapture( pTree->GetHWND() );
			hDragDefaultCursor = ::SetCursor( DragCursor( bCopy ) );
		}

		// CTreeGDBBrowserInputState::ContinueDrag: the cursor for where the mouse
		// is and whether Ctrl is held.
		void ContinueDrag( const wxPoint &rAt, bool bCopy )
		{
			const wxSize size = pTree->GetClientSize();
			if ( ( rAt.x < 0 ) || ( rAt.x > size.x ) || ( rAt.y < 0 ) || ( rAt.y > size.y ) )
			{
				if ( !bDragLeft )
				{
					::SetCursor( ::LoadCursor( 0, IDC_NO ) );
					bDragLeft = true;
				}
			}
			else if ( bDragLeft || ( bCopy != bDragCopy ) )
			{
				::SetCursor( DragCursor( bCopy ) );
				bDragCopy = bCopy;
				bDragLeft = false;
			}
		}

		// CTreeGDBBrowserInputState::EndDrag. The mouse is let go before the rows
		// move, since a builder may ask something on the way.
		void EndDrag( bool bDrop )
		{
			if ( !bDragging )
			{
				return;
			}
			const wxTreeItemId target = dragTarget;
			const bool bCopy = bDragCopy;
			bDragging = false;
			::SetCursor( hDragDefaultCursor );
			if ( target.IsOk() )
			{
				pTree->SetItemDropHighlight( target, false );
			}
			if ( ::GetCapture() == pTree->GetHWND() )
			{
				::ReleaseCapture();
			}
			dragSource = wxTreeItemId();
			dragTarget = wxTreeItemId();
			bDragLeft = false;
			bDragCopy = false;
			hDragDefaultCursor = 0;
			if ( !bDrop || !target.IsOk() || ( GetViewManipulator() == 0 ) )
			{
				return;
			}
			const std::string szFolder = NameOf( target );
			std::vector<std::string> sources;
			wxArrayTreeItemIds selected;
			pTree->GetSelections( selected );
			for ( size_t nRow = 0; nRow < selected.size(); ++nRow )
			{
				if ( ( selected[nRow] != target ) && IsTopSelection( selected[nRow], target ) )
				{
					sources.push_back( NameOf( selected[nRow] ) );
				}
			}
			for ( size_t nSource = 0; ( nSource < sources.size() ) && ( pTree != nullptr ); ++nSource )
			{
				if ( bCopy )
				{
					CopyRow( szFolder, sources[nSource] );
				}
				else
				{
					MoveRow( szFolder, sources[nSource] );
				}
			}
			if ( pTree != nullptr )
			{
				SortLater( std::string() );
				UpdateSelectionManipulator( true );
			}
		}

		// CTreeGDBBrowserInputState::OnKeyDown: Space or Enter renames the one
		// selected row. Started after the key is handled, as the tree is still
		// inside its key notification here.
		void OnTreeKey( wxTreeEvent &rEvent )
		{
			rEvent.Skip();
			const int nKey = rEvent.GetKeyCode();
			if ( ( nKey != WXK_SPACE ) && ( nKey != WXK_RETURN ) && ( nKey != WXK_NUMPAD_ENTER ) )
			{
				return;
			}
			wxArrayTreeItemIds selected;
			if ( bEnableEdit && !labelEditItem.IsOk() && ( pTree->GetSelections( selected ) == 1 ) )
			{
				pTree->CallAfter( [this]()
				{
					if ( ( pTree != nullptr ) && !labelEditItem.IsOk() )
					{
						Rename();
					}
				} );
			}
		}

		void OnFocus( wxFocusEvent &rEvent )
		{
			rEvent.Skip();
			RegisterAsHandler();
		}

		void OnDestroyed( wxWindowDestroyEvent &rEvent )
		{
			rEvent.Skip();
			if ( rEvent.GetEventObject() == pTree )
			{
				// Also covers the parent destroying the window before this view.
				StopTree();
			}
		}
	};


	class CWxObjectBrowser : public IObjectBrowser, public ICommandHandler
	{
		// What the list and the trees are children of: a panel of the pane's or
		// the dialog's.
		wxWindow *pRoot = nullptr;
		// What the trees' dialogs open over: the pane's or the dialog's owner.
		IWidget *pTreeOwner = nullptr;
		wxChoice *pChoice = nullptr;
		// The trees' "Name" column header, shared by every table's tree.
		wxHeaderCtrlSimple *pHeader = nullptr;
		wxBoxSizer *pSizer = nullptr;
		IListener *pListener = nullptr;
		EKind eKind = KIND_BROWSER;
		int nGDBBrowserID = -1;
		bool bEnableEdit = true;
		// Shared by every table's tree, and so declared before them: it has to
		// outlive them.
		std::unique_ptr<wxImageList> pImages;
		std::vector<std::unique_ptr<CWxObjectTree>> tables;

		CWxObjectTree* Table( int nIndex ) const
		{
			return ( ( nIndex >= 0 ) && ( nIndex < static_cast<int>( tables.size() ) ) ) ? tables[nIndex].get() : nullptr;
		}

		int IndexOfChoice( int nChoice ) const
		{
			return static_cast<int>( reinterpret_cast<intptr_t>( pChoice->GetClientData( nChoice ) ) );
		}

		void CreateContents()
		{
			// CBS_DROPDOWNLIST | CBS_SORT over the chosen table's tree.
			pChoice = NWx::Child<wxChoice>( pRoot, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_SORT );
			pChoice->Bind( wxEVT_CHOICE, [this]( wxCommandEvent & )
			{
				if ( pListener != 0 )
				{
					pListener->OnTableSelected();
				}
			} );
			pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pChoice, wxSizerFlags().Expand() );
			CreateHeader();
			pRoot->SetSizer( pSizer );
			pImages.reset( CreateTypeImages() );
		}

		// The user data's list the header width is kept in: the browser panes'
		// or the link picker's, as CTreeGDBBrowser and CTreeGDBLinkBrowser kept
		// them apart.
		std::vector<int>& HeaderWidths() const
		{
			SUserData *const pUserData = Singleton<IUserDataContainer>()->Get();
			std::vector<int> &rWidths = ( eKind == KIND_LINK ) ? pUserData->tableLinkHeaderWidthList : pUserData->tableHeaderWidthList;
			if ( rWidths.empty() )
			{
				rWidths.resize( 1, 0 );
			}
			return rWidths;
		}

		// The MFC trees' header: one column, "Name" with tree_header.bmp's icon,
		// as wide as the user left it -- TABGDBB_TREE_COLUMN_WIDTH's 150 until
		// then -- and the width kept when the user drags it, as LoadHeaderWidth
		// and SaveHeaderWidth did.
		void CreateHeader()
		{
			std::string strName = NResources::GetString( IDS_TABGDBB_PROPERTY_THN_0 );
			const int nKeptWidth = HeaderWidths()[0];
			wxHeaderColumnSimple column( FromNarrow( strName ), ( nKeptWidth > 0 ) ? nKeptWidth : 150 );
			column.SetResizeable( true );
			const std::vector<wxBitmap> icons = NWxResourceImages::LoadIcons( IDB_TABGDBB_TREE_HEADER_IMAGE_LIST );
			if ( !icons.empty() )
			{
				column.SetBitmap( icons[0] );
			}
			pHeader = NWx::Child<wxHeaderCtrlSimple>( pRoot, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
			pHeader->AppendColumn( column );
			pHeader->Bind( wxEVT_HEADER_END_RESIZE, [this]( wxHeaderCtrlEvent &rEvent )
			{
				HeaderWidths()[0] = rEvent.GetWidth();
			} );
			pSizer->Add( pHeader, wxSizerFlags().Expand() );
		}

	public:
		virtual ~CWxObjectBrowser()
		{
			tables.clear();
		}

		// NObjectBrowser::CreateWxIn: the contents in a panel of pParent's.
		bool CreateIn( wxWindow *pParent, IWidget *pOwner, IListener *_pListener, EKind _eKind, int _nGDBBrowserID )
		{
			if ( pParent == nullptr )
			{
				return false;
			}
			pListener = _pListener;
			eKind = _eKind;
			nGDBBrowserID = _nGDBBrowserID;
			pRoot = NWx::Child<wxPanel>( pParent, wxID_ANY );
			pTreeOwner = pOwner;
			CreateContents();
			return true;
		}

		wxWindow* GetRoot() const
		{
			return pRoot;
		}

		virtual void Show( bool bShow )
		{
			if ( ( pRoot != nullptr ) && ( pRoot->IsShown() != bShow ) )
			{
				// A panel in a wx layout, which has to place what is left.
				pRoot->Show( bShow );
				if ( pRoot->GetParent() != nullptr )
				{
					pRoot->GetParent()->Layout();
				}
			}
		}

		virtual void EnableEdit( bool bEnable )
		{
			bEnableEdit = bEnable;
			for ( size_t nTable = 0; nTable < tables.size(); ++nTable )
			{
				tables[nTable]->EnableEdit( bEnable );
			}
		}

		virtual void RemoveAllTables()
		{
			if ( pChoice != nullptr )
			{
				pChoice->Clear();
			}
			tables.clear();
		}

		virtual IObjectTree* AddTable( const std::string &rszTableName )
		{
			if ( ( pChoice == nullptr ) || ( pRoot == nullptr ) )
			{
				return 0;
			}
			std::unique_ptr<CWxObjectTree> pTable( new CWxObjectTree( eKind, pListener, pTreeOwner, nGDBBrowserID ) );
			pTable->CreateWindow( pRoot, pImages.get() );
			pTable->GetWindow()->Hide();
			pTable->EnableEdit( bEnableEdit );
			pSizer->Add( pTable->GetWindow(), wxSizerFlags( 1 ).Expand() );
			tables.push_back( std::move( pTable ) );
			pChoice->Append( FromNarrow( rszTableName ), reinterpret_cast<void*>( static_cast<intptr_t>( tables.size() - 1 ) ) );
			return tables.back().get();
		}

		virtual int GetTableCount()
		{
			return static_cast<int>( tables.size() );
		}

		virtual IObjectTree* GetTable( int nIndex )
		{
			return Table( nIndex );
		}

		virtual IObjectTree* GetTable( const std::string &rszTableName )
		{
			const int nChoice = pChoice->FindString( FromNarrow( rszTableName ), true );
			return ( nChoice != wxNOT_FOUND ) ? Table( IndexOfChoice( nChoice ) ) : nullptr;
		}

		// CComboBoxGDBBrowser::ActivateTab, which sent the pane WM_GDB_BROWSER
		// straight away.
		virtual bool ActivateTable( IObjectTree *pTree )
		{
			for ( unsigned nChoice = 0; nChoice < pChoice->GetCount(); ++nChoice )
			{
				if ( Table( IndexOfChoice( nChoice ) ) == pTree )
				{
					pChoice->SetSelection( nChoice );
					if ( pListener != 0 )
					{
						pListener->OnTableSelected();
					}
					return true;
				}
			}
			return false;
		}

		virtual IObjectTree* GetActiveTable()
		{
			const int nChoice = ( pChoice != nullptr ) ? pChoice->GetSelection() : wxNOT_FOUND;
			return ( nChoice != wxNOT_FOUND ) ? Table( IndexOfChoice( nChoice ) ) : nullptr;
		}

		virtual bool GetActiveTableName( std::string *pszTableName )
		{
			const int nChoice = ( pChoice != nullptr ) ? pChoice->GetSelection() : wxNOT_FOUND;
			if ( nChoice == wxNOT_FOUND )
			{
				return false;
			}
			if ( pszTableName != 0 )
			{
				( *pszTableName ) = ToNarrow( pChoice->GetString( nChoice ) );
			}
			return true;
		}

		virtual void ShowActiveTable()
		{
			const IObjectTree *const pActive = GetActiveTable();
			for ( size_t nTable = 0; nTable < tables.size(); ++nTable )
			{
				tables[nTable]->GetWindow()->Show( tables[nTable].get() == pActive );
			}
			if ( pRoot != nullptr )
			{
				pRoot->Layout();
			}
		}

		virtual ICommandHandler* GetObjectStorage()
		{
			return this;
		}

		// CComboBoxGDBBrowser's ICommandHandler: the chosen table's selection.
		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			CWxObjectTree *const pTable = dynamic_cast<CWxObjectTree*>( GetActiveTable() );
			if ( ( pTable == nullptr ) || !pTable->IsTreeCreated() )
			{
				return false;
			}
			switch ( nCommandID )
			{
				case ID_OS_GET_OBJECTSET:
					return pTable->GetCurrentObjectSet( reinterpret_cast<SObjectSet*>( dwData ) );
				case ID_OS_GET_SELECTION:
					return pTable->GetCurrentSelectionSet( reinterpret_cast<SSelectionSet*>( dwData ) );
				default:
					return false;
			}
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			switch ( nCommandID )
			{
				case ID_OS_GET_OBJECTSET:
				case ID_OS_GET_SELECTION:
					( *pbEnable ) = ( GetActiveTable() != nullptr );
					( *pbCheck ) = false;
					return true;
				default:
					return false;
			}
		}
	};
}


namespace NObjectBrowser
{
	IObjectBrowser* CreateWxIn( wxWindow *pParent, IWidget *pOwner, IObjectBrowser::IListener *pListener,
															IObjectBrowser::EKind eKind, wxWindow **ppWindow, int nGDBBrowserID )
	{
		std::unique_ptr<CWxObjectBrowser> pBrowser( new CWxObjectBrowser() );
		if ( !pBrowser->CreateIn( pParent, pOwner, pListener, eKind, nGDBBrowserID ) )
		{
			return nullptr;
		}
		if ( ppWindow != nullptr )
		{
			( *ppWindow ) = pBrowser->GetRoot();
		}
		return pBrowser.release();
	}
}

