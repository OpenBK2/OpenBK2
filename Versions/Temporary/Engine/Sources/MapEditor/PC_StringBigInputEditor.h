#pragma once

#include "PC_StringBrowseEditor.h"


class CPCStringBigInputEditor : public CPCStringBrowseEditor
{
	OBJECT_NOCOPY_METHODS( CPCStringBigInputEditor );

private:
	// CPCStringBrowseEditor
	void OnBrowse();

public:
	//CPCItemEditor
	bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
};


