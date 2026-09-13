#include "stdafx.h"

#include "PC_StringRefEditor.h"

// CPCItemEditor

void CPCStringRefEditor::SetValue( const CVariant &rValue )
{
	if ( rValue.GetType() == CVariant::VT_NULL )
	{
		CVariant nulRefValue = std::string();
		CPCStringBrowseEditor::SetValue( nulRefValue );
	}
	else
	{
		CPCStringBrowseEditor::SetValue( rValue );
	}
}


void CPCStringRefEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CPCStringBrowseEditor::GetValue( pValue );
		if ( pValue->GetStringRecode().empty() )
		{
			( *pValue ) = CVariant();
		}
	}
}


// CPCStringBrowseEditor

// The database link picker, in NPropertyButton.
void CPCStringRefEditor::OnBrowse()
{
	PressButton( NPropertyButton::BUTTON_BROWSE );
}

// basement storage


