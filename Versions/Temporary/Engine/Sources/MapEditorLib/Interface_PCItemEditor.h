#pragma once

#include "libdb/Manipulator.h"
#include "Interface_Controller.h"

// Property-kind enum for the property control. Shared: MapEditorLib,
// MapEditor and ED_B2_M1 all switch on it.
//
// IPCItemEditor itself used to be here. It is in MapEditor/PC_ItemEditor.h
// now: it is declared, implemented, overridden and called only inside that
// module, and its CWnd, CDialog and WPARAM/LPARAM are front-end details
// rather than something the editor is layered on.
//
// Вставить индекс нового типа в EPCIEType
// Изменить bmp в IDB_PC_TYPES_IMAGE_LIST ( в начало и в конец )
// Найти по коду все вхождения подобного индейкса и вставить новые обработчики:
//	[x] 1. CPCIEMnemonics::CPCIEMnemonics() - добавить новую строку
//	[x] 2. GetPCItemStringValue() - получить строку из CVariant
//  [x] 3. GetPCItemValue() - получить CVariant из строки
//  [x] 4. IPCItemEditor* CPCMainTreeControl::CreatePCItemEditor( HTREEITEM hItem ) - вставить вызов конструктора

enum EPCIEType
{
	PCIE_UNKNOWN								= 0,
	PCIE_FOLDER									= 1,
	PCIE_INT_INPUT							= 2,
	PCIE_INT_SLIDER							= 3,
	PCIE_INT_COMBO							= 4,
	PCIE_INT_COLOR							= 5,
	PCIE_INT_COLOR_WITH_ALPHA		= 6,
	PCIE_FLOAT_INPUT						= 7,
	PCIE_FLOAT_SLIDER						= 8,
	PCIE_FLOAT_COMBO						= 9,
	PCIE_BOOL_COMBO							= 10,
	PCIE_BOOL_CHECKBOX					= 11,
	PCIE_BOOL_SWITCHER					= 12,
	PCIE_STRING_REF							= 13,
	PCIE_STRING_MULTI_REF				= 14,
	PCIE_STRING_INPUT						= 15,
	PCIE_STRING_BIG_INPUT				= 16,
	PCIE_STRING_COMBO						= 17,
	PCIE_STRING_COMBO_REF				= 18,
	PCIE_STRING_COMBO_MULTI_REF	= 19,
	PCIE_STRING_FILE_REF				= 20,
	PCIE_STRING_DIR_REF					= 21,
	PCIE_BINARY_BIT_FIELD				= 22,
	PCIE_STRING_NEW_REF					= 23,
	PCIE_STRING_NEW_MULTI_REF		= 24,
	PCIE_GUID										= 25,
	PCIE_TEXT_FILE							= 26,
	PCIE_NEW_TEXT_FILE					= 27,
	PCIE_VEC3_COLOR							= 28,
	PCIE_LIST										= 29,
	PCIE_STRUCT									= 30,
	PCIE_COUNT									= 31,
};
