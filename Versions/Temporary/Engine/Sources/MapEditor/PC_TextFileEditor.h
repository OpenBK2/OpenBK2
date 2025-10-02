#pragma once

#include "PC_StringNewBrowseEditor.h"


class CPCTextFileEditor : public CPCStringNewBrowseEditor
{
	OBJECT_NOCOPY_METHODS( CPCTextFileEditor );

public:
	//CPCItemEditor
	void GetValue( CVariant *pValue );

private:
	// CPCStringNewBrowseEditor
	void GetButtonTitle( CString *pstrTitle, EButtonType eButtonType )
	{
		if ( pstrTitle )
		{
			switch( eButtonType )
			{
				case BT_BROWSE:
					pstrTitle->LoadString( IDS_BROWSE_BUTTON_TITLE );
					return;
				case BT_NEW:
					pstrTitle->LoadString( IDS_EDIT_BUTTON_TITLE );
					return;
				default:
					return;
			}
		}
	}
	void OnNew();
	void OnBrowse();
	
public:
	// Необходимо для работы Multiedit Text Editor
	static bool GetPCItemStringValue( string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc );
	static bool GetPCItemValue( CVariant *pValue, const string &rszValue, const SPropertyDesc *pPropertyDesc );
};


