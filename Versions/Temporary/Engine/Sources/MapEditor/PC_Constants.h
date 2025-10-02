#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_CONSTANTS__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_CONSTANTS__
#pragma once



//IC = ITEM CHANGE
#define IC_KILL_FOCUS								0
#define IC_VALUE_CHANGED						1

//PC = PROPERTY CONTROL
#define PC_TEMPORARY_EDITOR					0
#define PC_MULTILINE_STRING_EDITOR	1

//EBC = EDITOR BUTTON CHANGE
#define EBC_KILL_FOCUS							0
#define EBC_PRESSED									1

//ESC = EDITOR SLIDER CHANGE
#define ESC_KILL_FOCUS							0
#define ESC_POSITION_CHANGED				1

// Property Control String Param Label
#define PCSPL_MIN										"min:"
#define PCSPL_MAX										"max:"
#define PCSPL_STEP									"step:"
#define PCSPL_PAGE									"page:"
#define PCSPL_PRECISION							"precision:"
#define PCSPL_VALUES								"values:"
#define PCSPL_EDITOR								"editor:"
#define PCSPL_MASK									"mask:"
#define PCSPL_WIDTH									"width:"
#define PCSPL_HEIGHT								"height:"

// Property Control String Param
#define PCSP_DIVIDERS								";: ,|\t"
#define PCSP_STRONG_DIVIDERS				";:|"
#define PCSP_SOFT_DIVIDERS					" ,\t"
#define PCSP_MASK_DIVIDERS					";:"
#define PCSP_RANGE_DIVIDERS					"~!@#$%^&*_=?<>"
#define PCSP_NUMBERS								"-0123456789."

// Property Control String Value
#define PCSV_TRUE										"true"
#define PCSV_FALSE									"false"
#define PCSV_CHECK									"[ X ]"
#define PCSV_UNCHECK								"[    ]"
#define PCSV_NULL										"null"

#define PCSV_MAX_RECISION						6
#define PCSV_DEFAULT_RECISION				-1

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_CONSTANTS__)
