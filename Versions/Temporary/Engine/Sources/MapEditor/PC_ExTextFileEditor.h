#pragma once

#include "PC_String3ButtonEditor.h"


class CPCExTextFileEditor : public CPCString3ButtonEditor
{
	OBJECT_NOCOPY_METHODS( CPCExTextFileEditor );

public:
	//CPCItemEditor
	void GetValue( CVariant *pValue );

	// CPCString3ButtonEditor
	void OnBrowse();
	void OnNew();
	void OnEdit();

public:
	// Необходимо для работы Multiedit Text Editor
	static bool GetPCItemStringValue( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc );
	static bool GetPCItemValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc );
};


