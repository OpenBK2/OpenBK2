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

	//void GetUniqueName( const std::string &szObjectTypeName, std::string *pszObjectName );
};


