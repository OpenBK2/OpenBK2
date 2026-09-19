#pragma once

#include "MapEditorLib/Interface_PCItemEditor.h"

#include <string>
#include <vector>

class CObjectBaseController;

// A property's value as text and back, and the lists a combo row offers: the
// rules the property grid and its buttons follow for each editor type.
//
// These were statics on the MFC property tree's item editors (CPCIntInputEditor,
// CPCBinaryBitFieldEditor, CPCGUIDEditor, the text file editors, the three combo
// editors and CPCVec3ColorEditor) and two free functions in PC_ItemEditor.cpp,
// which dispatched to them by type. They moved here unchanged when the MFC
// property tree was taken out; none of them draws anything.

// The text a value is shown as, by the property's editor type: nothing about a
// multivariant but "...", a long string cut at its first line break unless
// bMultiline. False, with *pszValue = rszDefaultValue, for a type with no text.
bool GetPCItemStringValue( std::string *pszValue,
													 const CVariant &rValue,
													 const std::string &rszDefaultValue,
													 EPCIEType nType,
													 const SPropertyDesc *pDesc,
													 bool bMultiline );

// Text turned back into a value by the type's rules. False, with *pValue =
// rDefaultValue, when it does not parse.
bool GetPCItemValue( CVariant *pValue,
										 const std::string &rszValue,
										 const CVariant &rDefaultValue,
										 EPCIEType nType,
										 const SPropertyDesc *pDesc );

namespace NPropertyValues
{
	// A bit field's bytes as hex digits, two per byte, and back.
	bool BitFieldString( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc );
	bool BitFieldValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc );

	// The int and float combos' "values:" lists -- single numbers and ranges,
	// stepped by "step:" -- sorted by value. False for a list that does not
	// parse or is empty. The float one reads "precision:" into *pnPrecision,
	// starting from what is there.
	bool BuildIntChoices( const SPropertyDesc *pDesc, std::vector<std::string> *pChoices );
	bool BuildFloatChoices( const SPropertyDesc *pDesc, std::vector<std::string> *pChoices, int *pnPrecision );
	// Every object of the tables a reference may point at, sorted, appended to
	// *pChoices; "table:name" for a multi-table reference.
	void BuildRefChoices( const SPropertyDesc *pDesc, EPCIEType nType, std::vector<std::string> *pChoices );

	// A vec3_color is three float fields, .x .y .z in 0..1, shown as one ARGB
	// colour. Read, and written as three changes on the controller.
	bool GetVec3Color( int *pnColor, IManipulator *pManipulator, const std::string &rszName );
	bool AddVec3ColorChange( const std::string &rszName, const int nColor, CObjectBaseController *pObjectController, IManipulator *pManipulator );
}
