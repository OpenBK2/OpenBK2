#include "stdafx.h"

#include "pc_constants.h"
#include "PC_BoolComboEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CPCItemEditor

bool CPCBoolComboEditor::CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	if ( CPCStringComboEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		SetCreateControls( true );
		ResetContent();
		AddString( PCSV_TRUE );
		AddString( PCSV_FALSE );
		SetCreateControls( false );
		return true;
	}
	return false;
}


void CPCBoolComboEditor::SetValue( const CVariant &rValue )
{
	CVariant value = string( (bool)rValue ? PCSV_TRUE : PCSV_FALSE );
	CPCStringComboEditor::SetValue( value );
}


void CPCBoolComboEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CVariant value;
		CPCStringComboEditor::GetValue( &value );
		*pValue = (bool)( (string)(value.GetStr()) == PCSV_TRUE );
	}
}

// basement storage  

