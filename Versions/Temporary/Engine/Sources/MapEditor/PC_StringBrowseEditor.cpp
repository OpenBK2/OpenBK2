#include "stdafx.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "wmdefines.h"
#include "pc_constants.h"

#include "PC_StringBrowseEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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

