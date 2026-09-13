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
	// The values the list offers, from the descriptor's "values:" and "step:",
	// sorted by number. False when there is nothing to offer, or a value does
	// not parse -- the editor refuses to open then. The wx property grid builds
	// its list with this too, so the two offer the same values.
	static bool BuildChoices( const SPropertyDesc *pDesc, std::vector<std::string> *pChoices );

	//CPCItemEditor
	bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );

	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
};


