#pragma once

#include "PC_StringBrowseEditor.h"


class CPCStringFileRefEditor : public CPCStringBrowseEditor
{
	CPCStringFileRefEditor() {}
	OBJECT_NOCOPY_METHODS( CPCStringFileRefEditor );

	std::string szObjectTypeName;
public:
	CPCStringFileRefEditor( const std::string &rszObjectTypeName );

	//CPCItemEditor
	void GetValue( CVariant *pValue );

private:
	// CPCStringBrowseEditor
	void OnBrowse();
};


