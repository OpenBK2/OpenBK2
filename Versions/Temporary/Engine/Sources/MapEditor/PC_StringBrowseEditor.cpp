#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "WMDefines.h"
#include "PC_Constants.h"

#include "PC_StringBrowseEditor.h"

void CPCStringBrowseEditor::GetButtonTitle( CString *pstrTitle, int nButtonIndex )
{
	if ( nButtonIndex == 0 )
	{
		GetButtonTitle( pstrTitle, BT_BROWSE );
	}
}


void CPCStringBrowseEditor::OnButtonPressed( int nButtonIndex )
{
	if ( nButtonIndex == 0 )
	{
		OnBrowse();
	}
}


void CPCStringBrowseEditor::GetButtonTitle( CString *pstrTitle, EButtonType eButtonType )
{
	if ( pstrTitle )
	{
		switch( eButtonType )
		{
			case BT_BROWSE:
			{	
				pstrTitle->LoadString( IDS_BROWSE_BUTTON_TITLE );
				return;
			}
			default:
			{
				return;
			}
		}
	}
}

// basement storage  


