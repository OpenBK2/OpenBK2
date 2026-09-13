#pragma once

#include "PC_StringComboEditor.h"

struct CPCFloatComboEditorCompareItem
{
	bool operator()( const std::string &rszText0, const std::string &rszText1 )
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
	// The values the list offers, from "values:" and "step:", formatted with
	// "precision:" and sorted by number; *pnPrecision is the precision to start
	// from and is left as the one used. False when there is nothing to offer, or
	// a value does not parse. Shared with the wx property grid.
	static bool BuildChoices( const SPropertyDesc *pDesc, std::vector<std::string> *pChoices, int *pnPrecision );

	CPCFloatComboEditor();

	//CPCItemEditor
	bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );

	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
};


