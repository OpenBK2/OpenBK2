#include "stdafx.h"

#include "WMDefines.h"
#include "PC_Constants.h"
#include "PC_BoolSwitcherEditor.h"

CPCBoolSwitcherEditor::CPCBoolSwitcherEditor() : bDefaultValue( false )
{	
}


CPCBoolSwitcherEditor::~CPCBoolSwitcherEditor()
{
}


// CPCItemEditor

bool CPCBoolSwitcherEditor::ActivateEditor( CDialog *pwndActiveDialog )
{
	bDefaultValue = !bDefaultValue;
	if ( GetTargetWindow() )
	{
		SetValueChanged();
		GetTargetWindow()->SendMessage( WM_PC_ITEM_CHANGE, MAKEWPARAM( IC_KILL_FOCUS, PC_TEMPORARY_EDITOR ), 0 );
		return true;
	}
	return false;
}


void CPCBoolSwitcherEditor::SetValue( const CVariant &rValue )
{
	bDefaultValue = (bool)rValue;
}


void CPCBoolSwitcherEditor::GetValue( CVariant *pValue )
{
	if ( !pValue )
	{
		return;
	}
	//
	*pValue = bDefaultValue;
}

// basement storage  


