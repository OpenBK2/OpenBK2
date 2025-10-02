#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "wmdefines.h"
#include "pc_constants.h"

#include "PC_StringNewBrowseEditor.h"

void CPCStringNewBrowseEditor::GetButtonTitle( CString *pstrTitle, int nButtonIndex )
{
	if ( nButtonIndex == 0 )
	{
		GetButtonTitle( pstrTitle, BT_BROWSE );
	}
	else if ( nButtonIndex == 1 )
	{
		GetButtonTitle( pstrTitle, BT_NEW );
	}
}


void CPCStringNewBrowseEditor::OnButtonPressed( int nButtonIndex )
{
	if ( nButtonIndex == 0 )
	{
		OnBrowse();
	}
	else if ( nButtonIndex == 1 )
	{
		OnNew();
	}
}


void CPCStringNewBrowseEditor::GetButtonTitle( CString *pstrTitle, EButtonType eButtonType )
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
				default:
					return;
			}
		}
	}
}

// basement storage  


