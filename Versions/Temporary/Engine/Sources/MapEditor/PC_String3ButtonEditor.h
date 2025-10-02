#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_3_BUTTON__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_STRING_3_BUTTON__
#pragma once

#include "ResourceDefines.h"
#include "PC_StringMultibuttonEditor.h"


class CPCString3ButtonEditor : public CPCStringMultibuttonEditor
{
public:
	enum EButtonType
	{
		BT_BROWSE	= 0,
		BT_NEW		= 1,
		BT_EDIT		= 2,
	};

	CPCString3ButtonEditor() : CPCStringMultibuttonEditor( 3 ) {}

	// CPCStringMultibuttonEditor
	virtual void GetButtonTitle( CString *pstrTitle, int nButtonIndex );
	virtual void OnButtonPressed( int nButtonIndex );

	//CPCStringBrowseEditor
	virtual void GetButtonTitle( CString *pstrTitle, EButtonType eButtonType );
	virtual void OnBrowse() = 0;
	virtual void OnNew() = 0;
	virtual void OnEdit() = 0;
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_3_BUTTON__)
