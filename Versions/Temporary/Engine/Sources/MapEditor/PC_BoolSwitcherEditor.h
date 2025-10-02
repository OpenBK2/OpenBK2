#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_BOOL_SWITCHER__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_BOOL_SWITCHER__
#pragma once

#include "PC_ItemEditor.h"


class CPCBoolSwitcherEditor : public CPCItemEditor
{
	OBJECT_NOCOPY_METHODS( CPCBoolSwitcherEditor );

	bool bDefaultValue;

public:
	CPCBoolSwitcherEditor();	
	virtual ~CPCBoolSwitcherEditor();

	//CPCItemEditor
	bool PlaceEditor( const CTRect<int> &rPlaceRect ) { return true; }
	bool ActivateEditor( CDialog *pwndActiveDialog );
	
	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
	//
	void ProcessMessage( UINT nMessage, WPARAM wParam, LPARAM lParam ) {}
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_BOOL_SWITCHER__)
