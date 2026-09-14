#include "stdafx.h"
// IDC_GDB_TREE_0, which CComboBoxGDBBrowser::AddNewTab's inline body names.
#include "MapEditorLib/ResourceDefines.h"
#include "WMDefines.h"

#include "ObjectBrowserView.h"
#include "ComboBox_GDBBrowser.h"
#include "Tree_GDBBrowser.h"
#include "Tree_GDBLinkBrowser.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>
#include <memory>

// The database browser's contents as they have always been, CComboBoxGDBBrowser
// and its trees, behind the boundary.

namespace
{
	// The same flag every migrated view follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		const char *pszUseWx = std::getenv( "OBK2_WX_DIALOGS" );
		return ( pszUseWx != 0 ) && ( pszUseWx[0] != '0' ) && ( pszUseWx[0] != '\0' );
	}


	class CMfcObjectBrowser : public IObjectBrowser
	{
		std::unique_ptr<CComboBoxGDBBrowser> pContents;
		EKind eKind = KIND_BROWSER;

		static CTreeGDBBrowserBase* ToTree( IObjectTree *pTree )
		{
			return dynamic_cast<CTreeGDBBrowserBase*>( pTree );
		}

	public:
		// The listener is not used: the combo box and the trees send
		// WM_GDB_BROWSER and WM_TREE_GDB_BROWSER to pParent themselves.
		virtual bool Create( IWidget *pParent, IListener *pListener, EKind _eKind, int nGDBBrowserID, unsigned nControlID )
		{
			eKind = _eKind;
			pContents.reset( new CComboBoxGDBBrowser( nGDBBrowserID ) );
			return pContents->Create( ToCWnd( pParent ), nControlID );
		}

		virtual void SetBounds( const CTRect<int> &rBounds )
		{
			pContents->MoveWindow( CRect( rBounds.left, rBounds.top, rBounds.right, rBounds.bottom ) );
		}

		virtual void Show( bool bShow )
		{
			pContents->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
		}

		virtual void EnableEdit( bool bEnable )
		{
			pContents->EnableEdit( bEnable );
		}

		virtual void RemoveAllTables()
		{
			pContents->RemoveAllTabs();
		}

		virtual IObjectTree* AddTable( const std::string &rszTableName )
		{
			if ( eKind == KIND_LINK )
			{
				return pContents->AddNewTab( static_cast<CTreeGDBLinkBrowser*>( 0 ), rszTableName );
			}
			return pContents->AddNewTab( static_cast<CTreeGDBBrowser*>( 0 ), rszTableName );
		}

		virtual int GetTableCount()
		{
			return pContents->GetTabCount();
		}

		virtual IObjectTree* GetTable( int nIndex )
		{
			return pContents->GetTab( nIndex );
		}

		virtual IObjectTree* GetTable( const std::string &rszTableName )
		{
			return pContents->GetTab( rszTableName );
		}

		virtual bool ActivateTable( IObjectTree *pTree )
		{
			return pContents->ActivateTab( ToTree( pTree ) );
		}

		virtual IObjectTree* GetActiveTable()
		{
			CTreeGDBBrowserBase *pTree = 0;
			return pContents->GetActiveTab( &pTree ) ? pTree : 0;
		}

		virtual bool GetActiveTableName( std::string *pszTableName )
		{
			return pContents->GetActiveTabName( pszTableName );
		}

		virtual void ShowActiveTable()
		{
			pContents->SwitchTabs();
		}

		virtual ICommandHandler* GetObjectStorage()
		{
			return pContents.get();
		}
	};
}


namespace NObjectBrowser
{
	IObjectBrowser* CreateMfc()
	{
		return new CMfcObjectBrowser();
	}


	IObjectBrowser* Create()
	{
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return CreateWx();
		}
#endif
		return CreateMfc();
	}
}
