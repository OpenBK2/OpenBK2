#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_BINARY_BIT_FIELD__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_BINARY_BIT_FIELD__
#pragma once

#include "PC_StringBrowseEditor.h"


class CPCBinaryBitFieldEditor : public CPCStringBrowseEditor
{
	OBJECT_NOCOPY_METHODS( CPCBinaryBitFieldEditor );
public:
	//CPCItemEditor
	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );
private:
	// CPCStringBrowseEditor
	void OnBrowse();
	// Необходимо для работы Multiedit Text Editor
public:
	static bool GetPCItemStringValue( string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc );
	static bool GetPCItemValue( CVariant *pValue, const string &rszValue, const SPropertyDesc *pPropertyDesc );
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_BINARY_BIT_FIELD__)
