#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_FILE_REF__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_STRING_FILE_REF__
#pragma once

#include "PC_StringBrowseEditor.h"


class CPCStringFileRefEditor : public CPCStringBrowseEditor
{
	CPCStringFileRefEditor() {}
	OBJECT_NOCOPY_METHODS( CPCStringFileRefEditor );

	string szObjectTypeName;
public:
	CPCStringFileRefEditor( const string &rszObjectTypeName );

	//CPCItemEditor
	void GetValue( CVariant *pValue );

private:
	// CPCStringBrowseEditor
	void OnBrowse();
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_FILE_REF__)
