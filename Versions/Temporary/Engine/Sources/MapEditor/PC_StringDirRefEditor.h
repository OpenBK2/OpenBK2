#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_DIR_REF__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_STRING_DIR_REF__
#pragma once

#include "PC_StringBrowseEditor.h"


class CPCStringDirRefEditor : public CPCStringBrowseEditor
{
	static const char FOLDER_PATH_LABEL[];

	CPCStringDirRefEditor() {}
	OBJECT_NOCOPY_METHODS( CPCStringDirRefEditor );

	string szObjectTypeName;
public:
	CPCStringDirRefEditor( const string &rszObjectTypeName );

	//CPCItemEditor
	void GetValue( CVariant *pValue );

private:
	// CPCStringBrowseEditor
	void OnBrowse();
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_REF__)

