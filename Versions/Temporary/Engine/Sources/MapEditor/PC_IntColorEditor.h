#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_INT_COLOR__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_INT_COLOR__
#pragma once

#include "PC_StringBrowseEditor.h"


class CPCIntColorEditor : public CPCStringBrowseEditor
{
	OBJECT_NOCOPY_METHODS( CPCIntColorEditor );

public:
	//CPCItemEditor
	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );

private:
	// CPCStringBrowseEditor
	void OnBrowse();
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_REF__)

