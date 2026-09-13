#include "stdafx.h"

#include "PC_StringNewRefEditor.h"

// CPCItemEditor

void CPCStringNewRefEditor::SetValue( const CVariant &rValue )
{
	if ( rValue.GetType() == CVariant::VT_NULL )
	{
		CVariant nulRefValue = std::string();
		CPCStringNewBrowseEditor::SetValue( nulRefValue );
	}
	else
	{
		CPCStringNewBrowseEditor::SetValue( rValue );
	}
}


void CPCStringNewRefEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CPCStringNewBrowseEditor::GetValue( pValue );
		if ( std::string( pValue->GetStr() ).empty() )
		{
			( *pValue ) = CVariant();
		}
	}
}


// CPCStringNewBrowseEditor

// A new object through the builder, in NPropertyButton.
void CPCStringNewRefEditor::OnNew()
{
	if ( PressButton( NPropertyButton::BUTTON_NEW ) )
	{
		RedrawWindow();
	}
}


// The database link picker, in NPropertyButton.
void CPCStringNewRefEditor::OnBrowse()
{
	PressButton( NPropertyButton::BUTTON_BROWSE );
}

// basement storage


