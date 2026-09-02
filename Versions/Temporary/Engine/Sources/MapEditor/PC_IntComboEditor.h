#pragma once

#include "PC_StringComboEditor.h"

struct CPCIntComboEditorCompareItem
{
	bool operator()( const std::string &rszText0, const std::string &rszText1 )
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
	bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );

	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
};


