#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_BROWSE__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_STRING_BROWSE__
#pragma once

#include "ResourceDefines.h"
#include "PC_StringMultibuttonEditor.h"


class CPCStringBrowseEditor : public CPCStringMultibuttonEditor
{
public:
	enum EButtonType
	{
		BT_BROWSE	= 0,
	};

	CPCStringBrowseEditor() : CPCStringMultibuttonEditor( 1 ) {}

	// CPCStringMultibuttonEditor
	virtual void GetButtonTitle( CString *pstrTitle, int nButtonIndex );
	virtual void OnButtonPressed( int nButtonIndex );

	//CPCStringBrowseEditor
	virtual void GetButtonTitle( CString *pstrTitle, EButtonType eButtonType );
	virtual void OnBrowse() = 0;
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_BROWSE__)
