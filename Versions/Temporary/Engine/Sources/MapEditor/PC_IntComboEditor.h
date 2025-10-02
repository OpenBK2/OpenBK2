#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_INT_COMBO__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_INT_COMBO__
#pragma once

#include "PC_StringComboEditor.h"

struct CPCIntComboEditorCompareItem
{
	bool operator()( const string &rszText0, const string &rszText1 )
	{ 
		int nValue0 = 0;
		int nValue1 = 0;
		sscanf( rszText0.c_str(), "%d", &nValue0 );
		sscanf( rszText1.c_str(), "%d", &nValue1 );
		return ( nValue1 > nValue0 );
	}
};


class CPCIntComboEditor : public CPCStringComboEditor
{
	OBJECT_NOCOPY_METHODS( CPCIntComboEditor );

public:
	//CPCItemEditor
	bool CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );

	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_INT_COMBO__)
