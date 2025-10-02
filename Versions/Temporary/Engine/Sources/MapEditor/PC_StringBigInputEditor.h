#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_BIG_INPUT__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_STRING_BIG_INPUT__
#pragma once

#include "PC_StringBrowseEditor.h"


class CPCStringBigInputEditor : public CPCStringBrowseEditor
{
	OBJECT_NOCOPY_METHODS( CPCStringBigInputEditor );

private:
	// CPCStringBrowseEditor
	void OnBrowse();

public:
	//CPCItemEditor
	bool CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_BIG_INPUT__)

