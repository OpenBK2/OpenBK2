#include "stdafx.h"

#include "PCIEMnemonics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CPCIEMnemonics::CPCIEMnemonics() : CMnemonicsCollector<int>( PCIE_UNKNOWN, "" )
{
	Insert( PCIE_INT_INPUT,								"int_input" );
	Insert( PCIE_INT_SLIDER,							"int_slider" );
	Insert( PCIE_INT_COMBO,								"int_combo" );
	Insert( PCIE_INT_COLOR,								"int_color" );
	Insert( PCIE_INT_COLOR_WITH_ALPHA,		"int_color_with_alpha" );
	Insert( PCIE_FLOAT_INPUT,							"float_input" );
	Insert( PCIE_FLOAT_SLIDER,						"float_slider" );
	Insert( PCIE_FLOAT_COMBO,							"float_combo" );
	Insert( PCIE_BOOL_COMBO,							"bool_combo" );
	Insert( PCIE_BOOL_CHECKBOX,						"bool_checkbox" );
	Insert( PCIE_BOOL_SWITCHER,						"bool_switcher" );
	Insert( PCIE_STRING_REF,							"string_ref" );
	Insert( PCIE_STRING_MULTI_REF,				"string_multi_ref" );
	Insert( PCIE_STRING_INPUT,						"string_input" );
	Insert( PCIE_STRING_BIG_INPUT,				"string_big_input" );
	Insert( PCIE_STRING_COMBO,						"string_combo" );
	Insert( PCIE_STRING_COMBO_REF,				"string_combo_ref" );
	Insert( PCIE_STRING_COMBO_MULTI_REF,	"string_combo_multi_ref" ); 
	Insert( PCIE_STRING_FILE_REF,					"string_file_ref" );
	Insert( PCIE_STRING_DIR_REF,					"string_dir_ref" );
	Insert( PCIE_BINARY_BIT_FIELD,				"bit_field" );
	Insert( PCIE_STRING_NEW_REF,					"string_new_ref" );
	Insert( PCIE_STRING_NEW_MULTI_REF,		"string_new_multi_ref" );
	Insert( PCIE_GUID,										"guid" );
	Insert( PCIE_TEXT_FILE,								"new_text_file" );
	Insert( PCIE_NEW_TEXT_FILE,						"text_file" );
	Insert( PCIE_VEC3_COLOR,							"vec3_color" );
}


EPCIEType CPCIEMnemonics::Get( const SPropertyDesc *pDesc, bool bArrayNode )
{
	EPCIEType nType = PCIE_UNKNOWN;
	if ( pDesc )
	{
		if ( pDesc->bArray && ( !bArrayNode ) )
			nType = PCIE_LIST;
		else if ( pDesc->bStruct )
		{
			nType = static_cast<EPCIEType>( GetValue( pDesc->szPropControlType ) );
			if ( nType == PCIE_UNKNOWN )
			{
				nType = PCIE_STRUCT;
			}
		}
		else
			nType = static_cast<EPCIEType>( GetValue( pDesc->szPropControlType ) );
	}
	return nType;
}


EPCIEType CPCIEMnemonics::Get( const SPropertyDesc *pDesc, const string &rszName )
{
	if ( rszName.empty() )
	{
		return Get( pDesc, false );
	}	
	else
	{
		return Get( pDesc, rszName[rszName.size() - 1] == ARRAY_NODE_END_CHAR );
	}
}


bool CPCIEMnemonics::IsPointer( EPCIEType nType )
{
	return ( ( nType == PCIE_GUID ) ||
					 ( nType == PCIE_BINARY_BIT_FIELD ) );
}


bool CPCIEMnemonics::IsLeaf( EPCIEType nType )
{
	return ( ( nType != PCIE_UNKNOWN ) &&
					 ( nType != PCIE_FOLDER ) &&	 
					 ( nType != PCIE_STRUCT ) &&
					 ( nType != PCIE_LIST ) );
}


bool CPCIEMnemonics::IsRef( EPCIEType nType )
{
	return ( ( nType == PCIE_STRING_REF ) ||
					 ( nType == PCIE_STRING_MULTI_REF ) ||	 
					 ( nType == PCIE_STRING_COMBO_REF ) ||
					 ( nType == PCIE_STRING_COMBO_MULTI_REF ) ||
					 ( nType == PCIE_STRING_NEW_REF ) ||
					 ( nType == PCIE_STRING_NEW_MULTI_REF ) );
}


bool CPCIEMnemonics::IsSingleRef( EPCIEType nType )
{
	return ( ( nType == PCIE_STRING_REF ) ||
					 ( nType == PCIE_STRING_COMBO_REF ) ||
					 ( nType == PCIE_STRING_NEW_REF ) );
}


bool CPCIEMnemonics::IsMultiRef( EPCIEType nType )
{
	return ( ( nType == PCIE_STRING_MULTI_REF ) ||	 
					 ( nType == PCIE_STRING_COMBO_MULTI_REF ) ||
					 ( nType == PCIE_STRING_NEW_MULTI_REF ) );
}


CPCIEMnemonics typePCIEMnemonics;

// basement storage  

