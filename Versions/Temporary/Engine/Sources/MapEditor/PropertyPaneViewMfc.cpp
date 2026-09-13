#include "stdafx.h"

#include "PropertyPaneView.h"
#include "PC_Dialog.h"
#include "PC_ItemEditor.h"
#include "PC_Vec3ColorEditor.h"

#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/PCIEMnemonics.h"

#include <cstdlib>

// Selection Properties as it has always been, CPCDialog, behind the boundary;
// the dispatcher; and the rules for a property's value and read-only state,
// which the wx grid uses as well.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		const char *pszUseWx = std::getenv( "OBK2_WX_DIALOGS" );
		return ( pszUseWx != 0 ) && ( pszUseWx[0] != '0' ) && ( pszUseWx[0] != '\0' );
	}


	// What CDWPropertyBrowser did with its CPCDialog member, moved here. The
	// dialog registers itself as CHID_PC_DIALOG in its own OnInitDialog.
	class CMfcPropertyPane : public IPropertyPane
	{
		CPCDialog dialog;

	public:
		virtual bool Create( IWidget *pParentPane, const std::string &rszOptionsLabel )
		{
			dialog.SetXMLOptionsLabel( rszOptionsLabel );
			return dialog.Create( CPCDialog::IDD, ToCWnd( pParentPane ) ) != FALSE;
		}

		virtual bool IsCreated() const
		{
			return dialog.GetSafeHwnd() != 0;
		}

		virtual void SetBounds( const CTRect<int> &rBounds )
		{
			dialog.SetWindowPos( 0, rBounds.left, rBounds.top, rBounds.Width(), rBounds.Height(),
													 SWP_NOZORDER | SWP_NOACTIVATE );
		}

		virtual void Show( bool bShow )
		{
			dialog.ShowWindow( bShow ? SW_SHOW : SW_HIDE );
		}

		virtual void EnableEdit( bool bEnable )
		{
			dialog.EnableEdit( bEnable );
		}
	};
}


namespace NPropertyPane
{
	std::string ParentName( const std::string &rszName )
	{
		const size_t nSeparator = rszName.rfind( LEVEL_SEPARATOR_CHAR );
		return ( nSeparator == std::string::npos ) ? std::string() : rszName.substr( 0, nSeparator );
	}


	// CPCMainTreeControl::GetValue.
	bool GetValue( IManipulator *pManipulator, const std::string &rszName, CVariant *pValue )
	{
		if ( pManipulator == 0 || pValue == 0 )
		{
			return false;
		}
		const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pManipulator->GetDesc( rszName ) );
		if ( pDesc == 0 )
		{
			return false;
		}
		if ( typePCIEMnemonics.Get( pDesc, rszName ) == PCIE_VEC3_COLOR )
		{
			int nColor = 0xFFffFFff;
			if ( CPCVec3ColorEditor::GetColorValue( &nColor, pManipulator, rszName ) )
			{
				( *pValue ) = nColor;
				return true;
			}
			return false;
		}
		return pManipulator->GetValue( rszName, pValue );
	}


	// The value half of CPCMainTreeControl::SetPCItemView. The text is set even
	// when GetPCItemStringValue has no format for the type, as the tree did:
	// it answers the empty default then.
	bool GetValueText( IManipulator *pManipulator, const std::string &rszName, std::string *pszText )
	{
		if ( pManipulator == 0 || pszText == 0 )
		{
			return false;
		}
		const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pManipulator->GetDesc( rszName ) );
		if ( pDesc == 0 )
		{
			return false;
		}
		const EPCIEType nType = typePCIEMnemonics.Get( pDesc, rszName );
		if ( !typePCIEMnemonics.IsLeaf( nType ) )
		{
			return false;
		}
		CVariant value;
		if ( !GetValue( pManipulator, rszName, &value ) )
		{
			return false;
		}
		GetPCItemStringValue( pszText, value, "", nType, pDesc, false );
		return true;
	}


	// CPCMainTreeControl::ForceRelativeParam_ReadOnly, walking names instead of
	// tree items.
	bool IsReadOnly( IManipulator *pManipulator, const std::string &rszName )
	{
		if ( pManipulator == 0 )
		{
			return false;
		}
		for ( std::string szName = rszName; !szName.empty(); szName = ParentName( szName ) )
		{
			const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pManipulator->GetDesc( szName ) );
			if ( ( pDesc != 0 ) && pDesc->bReadOnly )
			{
				return true;
			}
		}
		return false;
	}


	IPropertyPane* CreateMfc()
	{
		return new CMfcPropertyPane();
	}


	IPropertyPane* Create()
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
