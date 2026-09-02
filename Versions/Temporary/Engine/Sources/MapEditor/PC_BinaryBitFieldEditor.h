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
	static bool GetPCItemStringValue( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc );
	static bool GetPCItemValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc );
};


