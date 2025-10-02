#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_FLOAT_COMBO__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_FLOAT_COMBO__
#pragma once

#include "PC_StringComboEditor.h"

struct CPCFloatComboEditorCompareItem
{
	bool operator()( const string &rszText0, const string &rszText1 )
	{ 
		float fValue0 = 0.0f;
		float fValue1 = 0.0f;
		sscanf( rszText0.c_str(), "%g", &fValue0 );
		sscanf( rszText1.c_str(), "%g", &fValue1 );
		return ( fValue1 > fValue0 );
	}
};


class CPCFloatComboEditor : public CPCStringComboEditor
{
	OBJECT_NOCOPY_METHODS( CPCFloatComboEditor );

	int nPrecision;

public:
	CPCFloatComboEditor();	

	//CPCItemEditor
	bool CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );

	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_FLOAT_COMBO__)
