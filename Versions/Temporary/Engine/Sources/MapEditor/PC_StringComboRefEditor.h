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
	// Every object in the tables the descriptor may refer to, sorted, with the
	// table name in front for a multi-ref type; appended to *pChoices after
	// whatever is already there. The list's "null" entry is not included.
	// Shared with the wx property grid.
	static void BuildChoices( const SPropertyDesc *pDesc, EPCIEType nType, std::vector<std::string> *pChoices );

	//CPCItemEditor
	bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );

	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
};


