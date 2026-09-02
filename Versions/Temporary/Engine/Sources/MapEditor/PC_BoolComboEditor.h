#pragma once

#include "PC_StringComboEditor.h"

class CPCBoolComboEditor : public CPCStringComboEditor
{
	OBJECT_NOCOPY_METHODS( CPCBoolComboEditor );

public:
	//CPCItemEditor
	bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );

	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
};


