#pragma once

#include "PC_StringComboEditor.h"

struct CPCStringComboRefEditorCompareItem
{
	bool operator()( const std::string &rszText0, const std::string &rszText1 )
	{ 
		return ( rszText1 > rszText0 );
	}
};


class CPCStringComboRefEditor : public CPCStringComboEditor
{
	OBJECT_NOCOPY_METHODS( CPCStringComboRefEditor );

public:
	//CPCItemEditor
	bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );

	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
};


