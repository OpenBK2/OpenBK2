#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "PC_Constants.h"


#include "PC_StringBigInputEditor.h"

// CPCItemEditor

bool CPCStringBigInputEditor::CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	bool bResult = CPCStringBrowseEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow );
	if ( bResult )
	{
		SetMultiLine( true );
	}
	return bResult;
}


// CPCStringBrowseEditor

// The text in the Lua or text editor, in NPropertyButton.
void CPCStringBigInputEditor::OnBrowse()
{
	PressButton( NPropertyButton::BUTTON_BROWSE );
}

// basement storage


