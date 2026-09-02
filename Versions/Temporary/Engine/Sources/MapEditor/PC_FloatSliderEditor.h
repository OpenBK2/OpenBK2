#pragma once

#include "PC_StringSliderEditor.h"


class CPCFloatSliderEditor : public CPCStringSliderEditor
{
	float fStep;
	int nPrecision;
	int nPowerPrecision;

	OBJECT_NOCOPY_METHODS( CPCFloatSliderEditor );

public:
	CPCFloatSliderEditor();

	//CPCItemEditor
	bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );

private:
	// CPCStringSliderEditor
	void OnChangePos( int nPos );
	void OnChangeEditBox();
public:
	DECLARE_MESSAGE_MAP()
};


