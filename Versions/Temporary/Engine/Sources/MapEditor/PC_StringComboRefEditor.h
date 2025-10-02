#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_COMBO_REF__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_STRING_COMBO_REF__
#pragma once

#include "PC_StringComboEditor.h"

struct CPCStringComboRefEditorCompareItem
{
	bool operator()( const string &rszText0, const string &rszText1 )
	{ 
		return ( rszText1 > rszText0 );
	}
};


class CPCStringComboRefEditor : public CPCStringComboEditor
{
	OBJECT_NOCOPY_METHODS( CPCStringComboRefEditor );

public:
	//CPCItemEditor
	bool CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );

	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_COMBO_REF__)

