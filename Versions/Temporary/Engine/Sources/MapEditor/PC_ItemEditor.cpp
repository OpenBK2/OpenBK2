#include "stdafx.h"

#include "pc_constants.h"
#include "pc_itemeditor.h"
#include "PC_ItemEditor.h"
#include "../Misc/StrProc.h"
#include "../MapEditorLib/StringManager.h"

//Editors
#include "PC_IntInputEditor.h"
#include "PC_BinaryBitFieldEditor.h"
#include "PC_GUIDEditor.h"
#include "PC_TextFileEditor.h"
#include "PC_ExTextFileEditor.h"
#include "PC_Vec3Coloreditor.h"

bool CPCItemEditor::CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	szName = rszName;
	nEditorType = _nEditorType;
	pPropertyDesc = _pPropertyDesc;
	nControlID = _nControlID;
	objectSet = rObjectSet;
	pwndTargetWindow = _pwndTargetWindow;
	bDefaultValue = true;
	return true;
}


bool GetPCItemStringValue( string *pszValue,
													 const CVariant &rValue,
													 const string &rszDefaultValue,
													 EPCIEType nType,
													 const SPropertyDesc *pDesc,
													 bool bMultiline )
{
	NI_ASSERT( pszValue != 0, "GetPCItemStringValue(): pszValue == 0" );
	NI_ASSERT( pDesc != 0,  "GetPCItemStringValue(): pDesc == 0" );
	//
	if ( rValue.GetType() == CVariant::VT_MULTIVARIANT )
	{
		( *pszValue ) = "...";
		return true;
	}
	switch ( nType )
	{
		case PCIE_INT_INPUT:
				return CPCIntInputEditor::GetPCItemStringValue( pszValue, rValue, pDesc ); 
		case PCIE_INT_SLIDER:
		case PCIE_INT_COMBO:
			( *pszValue ) = StrFmt( "%d", (int)rValue );
			return true;
		case PCIE_INT_COLOR:
		case PCIE_INT_COLOR_WITH_ALPHA:
		case PCIE_VEC3_COLOR:
		{
			const int nValue = (int)rValue;
			const int a = ( nValue >> 24 ) & 0xFF;
			const int r = ( nValue >> 16 ) & 0xFF;
			const int g = ( nValue >> 8 ) & 0xFF;
			const int b = nValue & 0xFF;
			if ( nType == PCIE_INT_COLOR_WITH_ALPHA )
			{
				( *pszValue ) = StrFmt( "%d, %d, %d, %d", a, r, g, b );
			}
			else
			{
				( *pszValue ) = StrFmt( "%d, %d, %d", r, g, b );
			}
			return true;
		}
		case PCIE_FLOAT_INPUT:
		case PCIE_FLOAT_SLIDER:
		case PCIE_FLOAT_COMBO:
		{
			string szValues = pDesc->szStringParam;
			NStr::ToLowerASCII( &szValues );
			int nPrecision = CStringManager::GetIntValueFromString( szValues, PCSPL_PRECISION, 0, PCSP_DIVIDERS, PCSV_DEFAULT_RECISION );
			if ( nPrecision > PCSV_MAX_RECISION )
			{
				nPrecision = PCSV_DEFAULT_RECISION;
			}
			else if ( nPrecision < 0 )
			{
				nPrecision = PCSV_DEFAULT_RECISION;
			}
			( *pszValue ) = CStringManager::GetFloatStringWithPrecision( (float)rValue, nPrecision );
//			const string szFormat = StrFmt( "%%.%df", nPrecision );
//			( *pszValue ) = StrFmt( szFormat.c_str(), (float)rValue );
			return true;
		}
		case PCIE_BOOL_COMBO:
		case PCIE_BOOL_SWITCHER:
			( *pszValue ) = (bool)rValue ? PCSV_TRUE : PCSV_FALSE;
			return true;
		case PCIE_BOOL_CHECKBOX:
			( *pszValue ) = (bool)rValue ? PCSV_CHECK : PCSV_UNCHECK;
			return true;
		case PCIE_STRING_REF:
		case PCIE_STRING_COMBO_REF:
		case PCIE_STRING_MULTI_REF:
		case PCIE_STRING_COMBO_MULTI_REF:
		case PCIE_STRING_NEW_REF:
		case PCIE_STRING_NEW_MULTI_REF:
		if ( rValue.GetType() == CVariant::VT_NULL )
			{
				( *pszValue ) = PCSV_NULL;
			}
			else
			{
				( *pszValue ) = rValue.GetStringRecode();
			}
			return true;
		case PCIE_STRING_INPUT:
		case PCIE_STRING_COMBO:
		case PCIE_STRING_FILE_REF:
		case PCIE_STRING_DIR_REF:
			( *pszValue ) = rValue.GetStringRecode();
			return true;
		case PCIE_STRING_BIG_INPUT:
		{
			string szValue = rValue.GetStringRecode();
			if ( !bMultiline )
			{
				szValue = szValue.substr( 0, szValue.find_first_of( "\r\n" ) );
			}
			( *pszValue ) = szValue;
			return true;
		}
		case PCIE_BINARY_BIT_FIELD:
		{
			return CPCBinaryBitFieldEditor::GetPCItemStringValue( pszValue, rValue, pDesc ); 
		}
		case PCIE_GUID:
		{
			return CPCGUIDEditor::GetPCItemStringValue( pszValue, rValue, pDesc ); 
		}
		case PCIE_TEXT_FILE:
		{
			return CPCTextFileEditor::GetPCItemStringValue( pszValue, rValue, pDesc ); 
		}
		case PCIE_NEW_TEXT_FILE:
		{
			return CPCExTextFileEditor::GetPCItemStringValue( pszValue, rValue, pDesc ); 
		}
		default:
			break;
	}
	( *pszValue ) = rszDefaultValue;
	return false;
}


bool GetPCItemValue( CVariant *pValue,
										 const string &rszValue,
										 const CVariant &rDefaultValue,
										 EPCIEType nType,
										 const SPropertyDesc *pDesc )
{
	NI_ASSERT( pValue != 0, "GetPCItemValue(): pValue == 0" );
	NI_ASSERT( pDesc != 0,  "GetPCItemValue(): pDesc == 0" );
	//
	switch ( nType )
	{
		case PCIE_INT_INPUT:
				return CPCIntInputEditor::GetPCItemValue( pValue, rszValue, pDesc );
		case PCIE_INT_SLIDER:
		case PCIE_INT_COMBO:
		{
			int nValue = 0;
			if ( sscanf( rszValue.c_str(), "%d", &nValue ) == 1 )
			{
				( *pValue ) = nValue;
				return true;
			}
			break;
		}
		case PCIE_INT_COLOR:
		case PCIE_INT_COLOR_WITH_ALPHA:
		case PCIE_VEC3_COLOR:
		{
			int a = 255;
			int r = 0;
			int g = 0;
			int b = 0;
			bool bScanned = false;
			if ( nType == PCIE_INT_COLOR_WITH_ALPHA )
			{
				if ( sscanf( rszValue.c_str(), "%d,%d,%d,%d", &a, &r, &g, &b ) == 4 )
				{
					if ( ( ( a >= 0 ) && ( a < 256 ) ) &&
							 ( ( r >= 0 ) && ( r < 256 ) ) &&
							 ( ( g >= 0 ) && ( g < 256 ) ) &&
							 ( ( b >= 0 ) && ( b < 256 ) ) )
					{
						bScanned = true;
					}
				}
			}
			else
			{
				if ( sscanf( rszValue.c_str(), "%d,%d,%d", &r, &g, &b ) == 3 )
				{
					if ( ( ( r >= 0 ) && ( r < 256 ) ) &&
							 ( ( g >= 0 ) && ( g < 256 ) ) &&
							 ( ( b >= 0 ) && ( b < 256 ) ) )
					{
						bScanned = true;
					}
				}
			}
			if ( bScanned )
			{
				( *pValue ) = (int)( ( a << 24 ) + ( r << 16 ) + ( g << 8 ) + b );
				return true;
			}
			break;
		}
		case PCIE_FLOAT_INPUT:
		case PCIE_FLOAT_SLIDER:
		case PCIE_FLOAT_COMBO:
		{
			float fValue = 0.0f;
			if ( sscanf( rszValue.c_str(), "%g", &fValue ) == 1 )
			{
				( *pValue ) = fValue;
				return true;
			}
			break;
		}
		case PCIE_BOOL_COMBO:
		case PCIE_BOOL_SWITCHER:
			( *pValue ) = (bool)( rszValue == PCSV_TRUE );
			return true;
		case PCIE_BOOL_CHECKBOX:
			( *pValue ) = (bool)( rszValue == PCSV_CHECK );
			return true;
		case PCIE_STRING_REF:
		case PCIE_STRING_COMBO_REF:
		case PCIE_STRING_MULTI_REF:
		case PCIE_STRING_COMBO_MULTI_REF:
		case PCIE_STRING_NEW_REF:
		case PCIE_STRING_NEW_MULTI_REF:
			if ( rszValue.empty() || ( rszValue == PCSV_NULL ) )
			{
				( *pValue ) = CVariant();
			}
			else
			{
				( *pValue ) = rszValue;
			}
			return true;
		case PCIE_STRING_INPUT:
		case PCIE_STRING_COMBO:
		case PCIE_STRING_FILE_REF:
		case PCIE_STRING_DIR_REF:
		case PCIE_STRING_BIG_INPUT:
			( *pValue ) = rszValue;
			return true;
		case PCIE_BINARY_BIT_FIELD:
		{
			return CPCBinaryBitFieldEditor::GetPCItemValue( pValue, rszValue, pDesc ); 
		}
		case PCIE_GUID:
		{
			return CPCGUIDEditor::GetPCItemValue( pValue, rszValue, pDesc ); 
		}
		case PCIE_TEXT_FILE:
		{
			return CPCTextFileEditor::GetPCItemValue( pValue, rszValue, pDesc ); 
		}
		case PCIE_NEW_TEXT_FILE:
		{
			return CPCExTextFileEditor::GetPCItemValue( pValue, rszValue, pDesc ); 
		}
		default:
			break;
	}
	( *pValue ) = rDefaultValue;
	return false;
}


/**
		if ( CWnd *pwndEditor = PCGetEditorWindow( pActiveItemEditor ) )
		{
			pwndEditor->PostMessage( WM_CLOSE );
		}
		if ( CWnd *pwndPCEditorButton0 = PCGetEditorWindow( pEditorButton0 ) )
		{
			pwndPCEditorButton0->PostMessage( WM_CLOSE );
		}
			pEditorButton0->SetAnchorWnd( PCGetEditorWindow( pwndEditor, false ) );
			pEditorButton0->SetPreviousWnd( PCGetEditorWindow( pwndEditor, false ) );
			pEditorButton0->SetCaption( "..." );
			pEditorButton0->CreateEditor( IDC_PC_ACTIVE_ITEM_EDITOR_BUTTON_0, szName, nType, pDesc, this );
	if ( pActiveItemEditor )
	{
		CRect pcItemRect( 0, 0, 0, 0 );
		HTREEITEM hItem = GetTreeItem( pActiveItemEditor->GetItemName() );
		if ( GetTreeItemEditorPlace( hItem, &pcItemRect ) )
		{
			if ( pEditorButton0 )
			{
				CRect itemRect( pcItemRect );
				CRect buttonRect( pcItemRect );
				//
				itemRect.right -= 20;
				pcItemRect.left = itemRect.right;
				//
				pActiveItemEditor->PlaceEditor( itemRect );
				pEditorButton0->PlaceEditor( pcItemRect );
			}
			else
			{
				pActiveItemEditor->PlaceEditor( pcItemRect );
			}
		}
		else
		{
			pActiveItemEditor->PlaceEditor( CRect( 0, 0, 0, 0 ) );
			if ( pEditorButton0 )
			{
				pEditorButton0->PlaceEditor( CRect( 0, 0, 0, 0 ) );
			}
		}
	}
				//
				CDialog *pwndParentDialog = dynamic_cast<CDialog*>( GetParent() );
				if ( pwndParentDialog )
				{
					CWnd *pwnd = PCGetEditorWindow( pActiveItemEditor );
					pwndParentDialog->GotoDlgCtrl( pwnd );
				}
/**/


/**
bool CheckPCValue( IManipulator *pManipulator, const string &rszName, const CVariant &rValue )
{
	bool bResult = true;
	const SPropertyDesc *pDesc = pManipulator->GetPropertyDesc( rszName.c_str() );
	if ( !pDesc )
	{
		return false;
	}

	if ( ( pDesc->dwChecks & SPropertyDesc::PCT_LEFT_BORDER ) > 0 )
	{
		switch ( pDesc->ePropType )
		{
			case SPropertyDesc::VAL_INT:
				if ( ( (int)rValue ) < ( (int)pDesc->fLeftBorder ) )
				{
					bResult = false;
				}
				break;
			case SPropertyDesc::VAL_FLOAT:
				if ( ( (float)rValue ) < pDesc->fLeftBorder )
				{
					bResult = false;
				}
				break;
			default:
				break;
		}
	}
	else if ( ( pDesc->dwChecks & SPropertyDesc::PCT_LEFT_INTERVAL ) > 0 )
	{
		switch ( pDesc->ePropType )
		{
			case SPropertyDesc::VAL_INT:
				if ( ( (int)rValue ) <= ( (int)pDesc->fLeftBorder ) )
				{
					bResult = false;
				}
				break;
			case SPropertyDesc::VAL_FLOAT:
				if ( ( (float)rValue ) <= pDesc->fLeftBorder )
				{
					bResult = false;
				}
				break;
			default:
				break;
		}
	}
	//
	if ( ( pDesc->dwChecks & SPropertyDesc::PCT_RIGHT_BORDER ) > 0 )
	{
		switch ( pDesc->ePropType )
		{
			case SPropertyDesc::VAL_INT:
				if ( ( (int)rValue ) > ( (int)pDesc->fRightBorder ) )
				{
					bResult = false;
				}
				break;
			case SPropertyDesc::VAL_FLOAT:
				if ( ( (float)rValue ) > pDesc->fRightBorder )
				{
					bResult = false;
				}
				break;
			default:
				break;
		}
	}
	else if ( ( pDesc->dwChecks & SPropertyDesc::PCT_RIGHT_INTERVAL ) > 0 )
	{
		switch ( pDesc->ePropType )
		{
			case SPropertyDesc::VAL_INT:
				if ( ( (int)rValue ) >= ( (int)pDesc->fRightBorder ) )
				{
					bResult = false;
				}
				break;
			case SPropertyDesc::VAL_FLOAT:
				if ( ( (float)rValue ) >= pDesc->fRightBorder )
				{
					bResult = false;
				}
				break;
			default:
				break;
		}
	}
	if ( ( pDesc->dwChecks & SPropertyDesc::PCT_FS_NAME ) > 0 )
	{
		if ( ( ( string)rValue ).find_first_of( FS_EXLUDE_SYMBOLS ) != string::npos )
		{
			bResult = false;
		}
	}
	if ( ( pDesc->dwChecks & SPropertyDesc::PCT_EXLUDE_SYMBOLS ) > 0 )
	{
		if ( ( ( string)rValue ).find_first_of( pDesc->szSymbols ) != string::npos )
		{
			bResult = false;
		}
	}
	else if ( ( pDesc->dwChecks & SPropertyDesc::PCT_INCLUDE_SYMBOLS ) > 0 )
	{
		if ( ( ( string)rValue ).find_first_not_of( pDesc->szSymbols ) != string::npos )
		{
			bResult = false;
		}
	}
	//
	if ( bResult )
	{
		pManipulator->CheckValue( rszName.c_str(), rValue, &bResult );
	}
	return bResult;
}
/**/		

// basement storage  


