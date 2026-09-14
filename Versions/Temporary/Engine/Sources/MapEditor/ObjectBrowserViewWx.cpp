#include "stdafx.h"

#include "ObjectBrowserView.h"

#ifdef OBK2_WITH_WX

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
#include "MapEditorLib/MfcWidget.h"
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
#include "MapEditorLib/StringManager.h"

#include <fmt/format.h>
#include <fmt/printf.h>

#include <wx/choice.h>
#include <wx/imaglist.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/utils.h>
#include <wx/sizer.h>
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
// Renaming, new folders, cut, copy, paste, delete and colour come next, and
// drag and drop after; until then the menu shows them greyed.
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


	// tree_types.bmp from the editor's resources, split into its 16 pixel
	// icons with magenta as the mask, as CComboBoxGDBBrowser::InitImageLists made
	// its image list.
	//
	// The pixels are read out as 32-bit colour and made into a wxImage. The
	// bitmap is a palette one; handed to wx as it came from LoadImage, the image
	// list drew every icon as a black silhouette.
	wxImageList* CreateTypeImages()
	{
		wxImageList *const pImages = new wxImageList( 16, 16, true, GDBO_COUNT * 2 );
		const LPCTSTR pszResource = MAKEINTRESOURCE( IDB_TABGDBB_TREE_TYPES_IMAGE_LIST );
		const HBITMAP hBitmap = static_cast<HBITMAP>( ::LoadImage( AfxFindResourceHandle( pszResource, RT_BITMAP ), pszResource,
																															 IMAGE_BITMAP, 0, 0, 0 ) );
		if ( hBitmap == 0 )
		{
			return pImages;
		}
		BITMAP header = { 0 };
		if ( ( ::GetObject( hBitmap, sizeof( header ), &header ) == 0 ) || ( header.bmWidth <= 0 ) || ( header.bmHeight <= 0 ) )
		{
			::DeleteObject( hBitmap );
			return pImages;
		}
		const int nWidth = header.bmWidth;
		const int nHeight = header.bmHeight;
		BITMAPINFO info = {};
		info.bmiHeader.biSize = sizeof( info.bmiHeader );
		info.bmiHeader.biWidth = nWidth;
		// Negative: rows top-down, as wxImage holds them.
		info.bmiHeader.biHeight = -nHeight;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		std::vector<unsigned char> pixels( static_cast<size_t>( nWidth ) * nHeight * 4 );
		const HDC hScreen = ::GetDC( 0 );
		const int nRows = ::GetDIBits( hScreen, hBitmap, 0, nHeight, &pixels[0], &info, DIB_RGB_COLORS );
		::ReleaseDC( 0, hScreen );
		::DeleteObject( hBitmap );
		if ( nRows != nHeight )
		{
			return pImages;
		}
		wxImage strip( nWidth, nHeight, false );
		unsigned char *const pRGB = strip.GetData();
		for ( size_t nPixel = 0; nPixel < static_cast<size_t>( nWidth ) * nHeight; ++nPixel )
		{
			// BGRX in, RGB out.
			pRGB[nPixel * 3 + 0] = pixels[nPixel * 4 + 2];
			pRGB[nPixel * 3 + 1] = pixels[nPixel * 4 + 1];
			pRGB[nPixel * 3 + 2] = pixels[nPixel * 4 + 0];
		}
		strip.SetMaskColour( 255, 0, 255 );
		pImages->Add( wxBitmap( strip ) );
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
			ICommandHandlerContainer *const pContainer = Singleton<ICommandHandlerContainer>();
			pContainer->Remove( CHID_OBJECT, this );
			pContainer->Remove( CHID_SELECTION, this );
			++nBuildGeneration;
			pBuildIterator = 0;
			RemoveViewManipulator();
			// The tree calls into this view from its events, so it goes first.
			if ( pTree != nullptr )
			{
				pTree->Destroy();
			}
		}

		void CreateWindow( wxWindow *pParent, wxImageList *pImages )
		{
			// The MFC tree's styles: buttons, lines at the root, several rows
			// selected at once. The root is wx's, and hidden.
			pTree = NWx::Child<wxTreeCtrl>( pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																			wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT |
																			wxTR_MULTIPLE | wxBORDER_SUNKEN );
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
		// tree has the focus: CTreeGDBBrowserBase::HandleCommand, for the commands
		// this slice has. The ones that edit rows answer false and so show greyed.
		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			if ( pTree == nullptr )
			{
				return false;
			}
			switch ( nCommandID )
			{
				case ID_OBJECT_LOAD:
					Load();
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
			switch ( nCommandID )
			{
				case ID_OBJECT_LOAD:
					( *pbEnable ) = CanLoad();
					return true;
				case ID_OBJECT_NEW:
				case ID_OBJECT_NEW_AT_ROOT:
				case ID_SELECTION_NEW:
					( *pbEnable ) = bEnableEdit && CanNew();
					return true;
				case ID_OBJECT_CHECK:
				case ID_OBJECT_EXPORT_FORCE:
				case ID_OBJECT_EXPORT_NO_REF_FORCE:
					( *pbEnable ) = CanExport();
					return true;
				case ID_OBJECT_EXPORT:
				case ID_OBJECT_EXPORT_NO_REF:
					( *pbEnable ) = CanExport();
					return true;
				case ID_SELECTION_FIND:
					( *pbEnable ) = true;
					return true;
				case ID_OBJECT_REF_LOOKUP:
				{
					wxArrayTreeItemIds selected;
					( *pbEnable ) = ( pTree->GetSelections( selected ) == 1 ) && ( TypeOf( pTree->GetFocusedItem() ) == GDBO_OBJECT );
					return true;
				}
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
						if ( item.IsOk() && !pTree->ItemHasChildren( item ) )
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
			pTree->Delete( rItem );
		}

		// TYPE_RENAME: a move adds the new row and takes the old one away once
		// it is empty; a rename in place changes the row's text.
		void RenameRow( const std::string &rszDestination, const std::string &rszSource, bool bNewRow )
		{
			if ( bNewRow )
			{
				AddRow( pTree->GetRootItem(), rszDestination, TypeOfName( rszDestination ) );
				const wxTreeItemId source = FindRow( rszSource );
				if ( source.IsOk() && !pTree->ItemHasChildren( source ) )
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
			CString strText;
			strText.LoadString( nID );
			return std::string( strText.GetString() );
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
				++nBuildGeneration;
				pBuildIterator = 0;
				pTree = nullptr;
			}
		}
	};


	class CWxObjectBrowser : public IObjectBrowser, public ICommandHandler
	{
		CWxHostWindow host;
		wxChoice *pChoice = nullptr;
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

	public:
		virtual ~CWxObjectBrowser()
		{
			tables.clear();
		}

		virtual bool Create( IWidget *pParent, IListener *_pListener, EKind _eKind, int _nGDBBrowserID, unsigned nControlID )
		{
			pListener = _pListener;
			eKind = _eKind;
			nGDBBrowserID = _nGDBBrowserID;
			if ( !host.CreateHost( ToCWnd( pParent ) ) )
			{
				return false;
			}
			const wxWindow *const pRoot = host.Root();
			// CBS_DROPDOWNLIST | CBS_SORT over the chosen table's tree.
			pChoice = NWx::Child<wxChoice>( host.Root(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_SORT );
			pChoice->Bind( wxEVT_CHOICE, [this]( wxCommandEvent & )
			{
				if ( pListener != 0 )
				{
					pListener->OnTableSelected();
				}
			} );
			pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pChoice, wxSizerFlags().Expand() );
			host.Root()->SetSizer( pSizer );
			pImages.reset( CreateTypeImages() );
			return pRoot != nullptr;
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
			if ( ( pChoice == nullptr ) || ( host.Root() == nullptr ) )
			{
				return 0;
			}
			std::unique_ptr<CWxObjectTree> pTable( new CWxObjectTree( eKind, pListener, &host, nGDBBrowserID ) );
			pTable->CreateWindow( host.Root(), pImages.get() );
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
			if ( host.Root() != nullptr )
			{
				host.Root()->Layout();
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
	IObjectBrowser* CreateWx()
	{
		return new CWxObjectBrowser();
	}
}

#endif // OBK2_WITH_WX
