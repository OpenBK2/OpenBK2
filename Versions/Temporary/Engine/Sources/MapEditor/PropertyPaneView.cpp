#include "stdafx.h"

#include "PropertyPaneView.h"
#include "PC_Constants.h"
#include "PropertyValues.h"

#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/ObjectController.h"
#include "MapEditorLib/PCIEMnemonics.h"
#include "System/FileUtils.h"

#include <cstdio>

// The rules for a property's value, its text and its read-only state, and the
// undo-list writes, which the wx grid (PropertyPaneViewWx.cpp) and the dialogs
// around it use.

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
			if ( NPropertyValues::GetVec3Color( &nColor, pManipulator, rszName ) )
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
	bool GetValueText( IManipulator *pManipulator, const std::string &rszName, std::string *pszText, bool bMultiline )
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
		GetPCItemStringValue( pszText, value, "", nType, pDesc, bMultiline );
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


	bool GetChoices( const SPropertyDesc *pDesc, EPCIEType nType, std::vector<std::string> *pChoices )
	{
		if ( pDesc == 0 || pChoices == 0 )
		{
			return false;
		}
		pChoices->clear();
		switch ( nType )
		{
			case PCIE_INT_COMBO:
				return NPropertyValues::BuildIntChoices( pDesc, pChoices );
			case PCIE_FLOAT_COMBO:
			{
				int nPrecision = PCSV_DEFAULT_RECISION;
				return NPropertyValues::BuildFloatChoices( pDesc, pChoices, &nPrecision );
			}
			case PCIE_STRING_COMBO:
				pChoices->assign( pDesc->values.begin(), pDesc->values.end() );
				return true;
			case PCIE_STRING_COMBO_REF:
			case PCIE_STRING_COMBO_MULTI_REF:
				pChoices->push_back( PCSV_NULL );
				NPropertyValues::BuildRefChoices( pDesc, nType, pChoices );
				return true;
			case PCIE_BOOL_COMBO:
			case PCIE_BOOL_SWITCHER:
				pChoices->push_back( PCSV_TRUE );
				pChoices->push_back( PCSV_FALSE );
				return true;
			default:
				return false;
		}
	}


	bool ParseValueText( IManipulator *pManipulator, const std::string &rszName, const std::string &rszText, CVariant *pValue )
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
		const EPCIEType nType = typePCIEMnemonics.Get( pDesc, rszName );
		// GetValue of CPCStringFileRefEditor, CPCStringDirRefEditor and both text
		// file editors: an empty path clears the value, an invalid one is refused.
		switch ( nType )
		{
			case PCIE_STRING_FILE_REF:
			case PCIE_STRING_DIR_REF:
			case PCIE_TEXT_FILE:
			case PCIE_NEW_TEXT_FILE:
				if ( !rszText.empty() && !::IsValidFileName( rszText, false ) )
				{
					return false;
				}
				break;
			default:
				break;
		}
		return GetPCItemValue( pValue, rszText, CVariant(), nType, pDesc );
	}


	// CPCMainTreeControl::UpdateValueFromPCItemEditor, and its AddChangeOperation
	// with the vec3_color case.
	bool CommitValue( CDefaultView *pView, const std::string &rszName, const CVariant &rNewValue )
	{
		IManipulator *const pManipulator = ( pView != 0 ) ? pView->GetViewManipulator() : 0;
		if ( pManipulator == 0 )
		{
			return false;
		}
		CVariant oldValue;
		if ( !GetValue( pManipulator, rszName, &oldValue ) || ( oldValue == rNewValue ) )
		{
			return false;
		}
		bool bResult = true;
		pManipulator->CheckValue( rszName, rNewValue, &bResult );
		if ( !bResult )
		{
			return false;
		}
		const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pManipulator->GetDesc( rszName ) );
		if ( pDesc == 0 )
		{
			return false;
		}
		CPtr<CObjectBaseController> pController = pView->CreateController<CObjectController>( static_cast<CObjectController*>( 0 ) );
		const bool bAdded = ( typePCIEMnemonics.Get( pDesc, rszName ) == PCIE_VEC3_COLOR )
												? NPropertyValues::AddVec3ColorChange( rszName, (int)rNewValue, pController, pManipulator )
												: pController->AddChangeOperation( rszName, rNewValue, pManipulator );
		if ( !bAdded )
		{
			return false;
		}
		pController->Redo( false, true, 0 );
		Singleton<IControllerContainer>()->Add( pController );
		return true;
	}


	bool AddChangeOperation( CObjectBaseController *pController, IManipulator *pManipulator,
													 const std::string &rszName, const CVariant &rValue )
	{
		if ( ( pController == 0 ) || ( pManipulator == 0 ) )
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
			return NPropertyValues::AddVec3ColorChange( rszName, (int)rValue, pController, pManipulator );
		}
		return pController->AddChangeOperation( rszName, rValue, pManipulator );
	}


	// The tree read the index back from the element's label, "[3]", which is the
	// name's last segment.
	bool GetNodeIndex( const std::string &rszName, int *pnIndex )
	{
		if ( ( pnIndex == 0 ) || rszName.empty() || ( rszName[rszName.size() - 1] != ARRAY_NODE_END_CHAR ) )
		{
			return false;
		}
		const size_t nSeparator = rszName.rfind( LEVEL_SEPARATOR_CHAR );
		const std::string szLast = ( nSeparator == std::string::npos ) ? rszName : rszName.substr( nSeparator + 1 );
		return sscanf( szLast.c_str(), "[%d]", pnIndex ) == 1;
	}


	namespace
	{
		// The write every array command ends in.
		bool AddNodeOperation( CDefaultView *pView, const std::string &rszArrayName, int nIndex, bool bInsert )
		{
			IManipulator *const pManipulator = ( pView != 0 ) ? pView->GetViewManipulator() : 0;
			if ( pManipulator == 0 )
			{
				return false;
			}
			CPtr<CObjectBaseController> pController = pView->CreateController<CObjectController>( static_cast<CObjectController*>( 0 ) );
			const bool bAdded = bInsert ? pController->AddInsertOperation( rszArrayName, nIndex, pManipulator )
																	: pController->AddRemoveOperation( rszArrayName, nIndex, pManipulator );
			if ( !bAdded )
			{
				return false;
			}
			pController->Redo( false, true, 0 );
			Singleton<IControllerContainer>()->Add( pController );
			return true;
		}
	}


	bool InsertNode( CDefaultView *pView, const std::string &rszArrayName, int nIndex )
	{
		return AddNodeOperation( pView, rszArrayName, nIndex, true );
	}


	bool RemoveNode( CDefaultView *pView, const std::string &rszArrayName, int nIndex )
	{
		return AddNodeOperation( pView, rszArrayName, nIndex, false );
	}


	void CopyValues( IManipulator *pManipulator, const std::vector<std::string> &rNames )
	{
		SUserData *const pUserData = Singleton<IUserDataContainer>()->Get();
		if ( ( pUserData == 0 ) || ( pManipulator == 0 ) )
		{
			return;
		}
		pUserData->pcSelection.Clear();
		for ( std::vector<std::string>::const_iterator itName = rNames.begin(); itName != rNames.end(); ++itName )
		{
			const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pManipulator->GetDesc( *itName ) );
			if ( ( pDesc == 0 ) || !typePCIEMnemonics.IsLeaf( typePCIEMnemonics.Get( pDesc, *itName ) ) )
			{
				continue;
			}
			SUserData::SPCSelectionData selectionData;
			if ( GetValue( pManipulator, *itName, &( selectionData.value ) ) )
			{
				pUserData->pcSelection.Insert( *itName, selectionData );
			}
		}
	}


	// As the tree wrote it: one controller, added to the undo list even when
	// none of the copied fields is here.
	bool PasteValues( CDefaultView *pView, const std::function<bool( const std::string& )> &rShown )
	{
		SUserData *const pUserData = Singleton<IUserDataContainer>()->Get();
		IManipulator *const pManipulator = ( pView != 0 ) ? pView->GetViewManipulator() : 0;
		if ( ( pUserData == 0 ) || ( pManipulator == 0 ) || pUserData->pcSelection.IsEmpty() )
		{
			return false;
		}
		CPtr<CObjectBaseController> pController = pView->CreateController<CObjectController>( static_cast<CObjectController*>( 0 ) );
		const SUserData::CPCSelection::CControlSelectionDataMap &rSelectionDataMap = pUserData->pcSelection.Get();
		for ( SUserData::CPCSelection::CControlSelectionDataMap::const_iterator itSelectionData = rSelectionDataMap.begin();
					itSelectionData != rSelectionDataMap.end(); ++itSelectionData )
		{
			if ( rShown( itSelectionData->first ) )
			{
				AddChangeOperation( pController, pManipulator, itSelectionData->first, itSelectionData->second.data.value );
			}
		}
		pController->Redo( false, true, 0 );
		Singleton<IControllerContainer>()->Add( pController );
		return true;
	}
}
