#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "WMDefines.h"
#include "PC_Constants.h"

#include "PC_String3ButtonEditor.h"

void CPCString3ButtonEditor::GetButtonTitle( CString *pstrTitle, int nButtonIndex )
{
	if ( nButtonIndex == 0 )
	{
		GetButtonTitle( pstrTitle, BT_BROWSE );
	}
	else if ( nButtonIndex == 1 )
	{
		GetButtonTitle( pstrTitle, BT_NEW );
	}
	else if ( nButtonIndex == 2 )
	{
		GetButtonTitle( pstrTitle, BT_EDIT );
	}
}


void CPCString3ButtonEditor::OnButtonPressed( int nButtonIndex )
{
	if ( nButtonIndex == 0 )
	{
		OnBrowse();
	}
	else if ( nButtonIndex == 1 )
	{
		OnNew();
	}
	else if ( nButtonIndex == 2 )
	{
		OnEdit();
	}
}


void CPCString3ButtonEditor::GetButtonTitle( CString *pstrTitle, EButtonType eButtonType )
{
	{
		if ( pstrTitle )
		{
			switch( eButtonType )
			{
				case BT_BROWSE:
					pstrTitle->LoadString( IDS_BROWSE_BUTTON_TITLE );
					return;
				case BT_NEW:
					pstrTitle->LoadString( IDS_NEW_BUTTON_TITLE );
					return;
				case BT_EDIT:
					pstrTitle->LoadString( IDS_EDIT_BUTTON_TITLE );
					return;
				default:
					return;
			}
		}
	}
}

// basement storage  


