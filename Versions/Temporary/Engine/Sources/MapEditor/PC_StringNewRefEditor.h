#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_NEW_REF__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_STRING_NEW_REF__
#pragma once

#include "PC_StringNewBrowseEditor.h"


class CPCStringNewRefEditor : public CPCStringNewBrowseEditor
{
	OBJECT_NOCOPY_METHODS( CPCStringNewRefEditor );

public:
	//CPCItemEditor
	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );

private:
	// CPCStringNewBrowseEditor
	void OnNew();
	void OnBrowse();
	
	void SetWindowTextByTypeAndName( const string &szObjectTypeName, const string &szObjectName );
	//void GetUniqueName( const string &szObjectTypeName, string *pszObjectName );
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_STRING_NEW_REF__)
