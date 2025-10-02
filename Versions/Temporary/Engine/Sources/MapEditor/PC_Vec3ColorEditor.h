#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_VEC3_COLOR__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_VEC3_COLOR__
#pragma once

#include "PC_StringBrowseEditor.h"


class CPCVec3ColorEditor : public CPCStringBrowseEditor
{
	OBJECT_NOCOPY_METHODS( CPCVec3ColorEditor );

public:
	//CPCItemEditor
	void SetValue( const CVariant &rValue );
	void GetValue( CVariant *pValue );

	// Необходимо для работы Multiedit Text Editor
	static bool GetPCItemStringValue( string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc ) { return false; }
	static bool GetPCItemValue( CVariant *pValue, const string &rszValue, const SPropertyDesc *pPropertyDesc ) { return false; }

	static bool GetColorValue( int *pnColor, IManipulator *pManipulator, const string &rszName );
	static bool AddChangeOperation( const string &rszName, const int nColor, class CObjectBaseController *pObjectController, IManipulator *pManipulator );

private:
	// CPCStringBrowseEditor
	void OnBrowse();
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_VEC3_COLOR__)
