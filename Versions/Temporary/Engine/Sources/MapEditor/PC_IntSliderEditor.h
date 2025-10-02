#pragma once

#include "PC_StringSliderEditor.h"


class CPCIntSliderEditor : public CPCStringSliderEditor
{
	int nStep;

	OBJECT_NOCOPY_METHODS( CPCIntSliderEditor );

public:
	CPCIntSliderEditor();

	//CPCItemEditor
	bool CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );

private:
	// CPCStringSliderEditor
	void OnChangePos( int nPos );
	void OnChangeEditBox();
public:
	DECLARE_MESSAGE_MAP()
};


