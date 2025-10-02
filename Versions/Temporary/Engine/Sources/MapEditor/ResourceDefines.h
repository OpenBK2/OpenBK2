#pragma once
//
#include "MapEditorLib/ResourceDefines.h"

//Main context menu numbers
#define MCMN_DW_GDB_BROWSER															0
#define MCMN_TREE_GDB_BROWSER														1
//
//Property Control context menu numbers
#define PCCMN_NODE																			0
#define PCCMN_MULTI_NODE																1
#define PCCMN_ARRAY																			2
#define PCCMN_ARRAY_NODE																3
#define PCCMN_ARRAY_MULTI_NODE													4
#define PCCMN_ARRAY_ARRAY																5

#define IDR_MANIFEST																		1
#define IDR_MAINFRAME																		129

#define	ID_DW_PROPERTY_BROWSER													130
#define	ID_DW_LOG_WINDOW																131
#define	ID_DW_GDB_BROWSER																136

#define IDC_OBJECT_TREE																	132
#define IDC_EMPTY_GDB_BROWSER														133
#define	IDC_LOG_WINDOW																	134
#define IDC_TREE_GDB_BROWSER														135

#define IDT_MAIN																				11000
#define IDT_SELECTION																		11001
#define IDT_CONTROLLER_CONTAINER												11002
#define IDT_OBJECT																			11003
#define IDT_PROPERTY_CONTROL														11004
#define IDT_VIEW																				11005
#define IDT_TEXT_EDITOR																	11006

#define IDB_EDITOR_STARTUP															11100
#define IDB_PC_TYPES_IMAGE_LIST													11101
#define IDB_PC_HEADER_IMAGE_LIST												11102
#define IDB_TABGDBB_TREE_TYPES_IMAGE_LIST								11103
#define IDB_TABGDBB_TREE_HEADER_IMAGE_LIST							11104
#define IDB_DEFAULT_NORMAL_OBJECT_IMAGE									11105
#define IDB_DEFAULT_SMALL_OBJECT_IMAGE									11106	

#define IDC_DRAG_AND_DROP_MOVE													11200
#define IDC_DRAG_AND_DROP_COPY													11201

#define IDM_MAIN_CONTEXT_MENU														11300
#define IDM_PC_CONTEXT_MENU															11301
#define IDM_TEXT_EDITOR																	11302
#define IDM_LOG_CONTEXT_MENU														11303

#define IDS_REGISTRY_PATH																11500
#define IDS_REGISTRY_KEY																11501
#define IDS_REGISTRY_KEY_MAXIMIZE												11502
#define IDS_REGISTRY_KEY_RECT														11503
#define IDS_REGISTRY_KEY_RECENT_LIST										11504
#define IDS_REGISTRY_KEY_TABLE													11505
#define IDS_REGISTRY_CURRENT_TABLE											11506
#define IDS_REGISTRY_KEY_TOOLBAR												11507
#define IDS_REGISTRY_KEY_WINDOWBAR											11508

#define IDS_REGISTRY_KEY_VERSION												11520

#define IDS_TOOLBAR_CUSTOMIZE_COMMAND										12000
#define IDS_TOOLBAR_MENU																12001
#define IDS_TOOLBAR_MAIN																12002
#define IDS_TOOLBAR_SELECTION														12003
#define IDS_TOOLBAR_CONTROLLER_CONTAINER								12004
#define IDS_TOOLBAR_OBJECT															12005
#define IDS_TOOLBAR_PROPERTY_CONTROL										12006
#define IDS_TOOLBAR_VIEW																12007

#define IDS_DW_GDB_BROWSE_NAME													12010
#define IDS_DW_PROPERTY_BROWSE_NAME											12011
#define IDS_DW_LOG_NAME																	12012

#define IDS_PC_PROPERTY_THN_0														12020
#define IDS_PC_PROPERTY_THN_1														12021
#define IDS_PC_PROPERTY_THN_2														12022
#define IDS_PC_DELETE_MESSAGE														12023
#define IDS_PC_DELETE_ALL_MESSAGE												12024

#define IDS_TABGDBB_PROPERTY_THN_0											12030
#define IDS_TABGDBB_PROPERTY_THN_1											12031

#define IDS_PROGRAM_VERSION															12300
#define IDS_HELP_FILE_NAME															12301
#define IDS_NO_HELP_FILE_MESSAGE												12302
#define IDS_BROWSE_FOR_FOLDER_DIALOG_TITLE							12303
#define IDS_BROWSE_FOR_FILE_DIALOG_TITLE								12304

#define IDS_TREE_GDB_BROWSE_NEW_FOLDER									12400
#define IDS_TREE_GDB_BROWSE_NEW_RESOURCE								12401
#define IDS_TREE_GDB_BROWSE_NEW_MAIN_OBJECT							12402
#define IDS_TREE_GDB_BROWSE_DELETE_OBJECTS_MESSAGE			12403
#define IDS_TREE_GDB_BROWSE_NO_OBJECT_FOUND_MESSAGE			12404
#define IDS_TREE_GDB_BROWSE_CFM													12405 
#define IDS_TREE_GDB_BROWSE_CFM_OBJECT_FOUND						12406
#define IDS_TREE_GDB_BROWSE_CFM_OBJECTS_FOUND						12407
#define IDS_TREE_GDB_BROWSE_CFM_NO_OBJECT_FOUND					12408
#define IDS_TREE_GDB_BROWSE_LOAD												12409
#define IDS_TREE_GDB_LINK_BROWSE_SELECT									12410
#define IDS_TREE_GDB_BROWSE_CHECK_WARNING								12411
#define IDS_TREE_GDB_BROWSE_EXPORT_WARNING							12412
#define IDS_TREE_GDB_BROWSE_EXPORT_NO_REF_WARNING				12413
#define IDS_TREE_GDB_BROWSE_EXPORT_FORCE_WARNING				12414
#define IDS_TREE_GDB_BROWSE_EXPORT_NO_REF_FORCE_WARNING	12415
#define IDS_PM_CREATE_EDITOR														12416
#define IDS_PM_DESTROY_EDITOR														12417
#define IDS_PM_RELOAD_EDITOR														12418
#define IDS_PM_SAVE																			12419
#define IDS_PM_REDO																			12420
#define IDS_PM_UNDO																			12421
#define IDS_PM_CREATE_MOD																12422
#define IDS_PM_OPEN_MOD																	12423
#define IDS_PM_CLOSE_MOD																12424


#define IDS_PC_BD_DIALOG_TITLE													12500
#define IDS_PC_OPEN_DIALOG_EMPTY_TITLE									12501
#define IDS_PC_OPEN_DIALOG_TITLE												12502
#define IDS_PC_LINK_DIALOG_EMPTY_TITLE									12503
#define IDS_PC_LINK_DIALOG_TITLE												12504
#define IDS_RECENT_EMPTY																12505
#define	IDS_DW_GDB_BROWSE_MENU_LABEL										12506	
#define IDS_DW_GDB_BROWSE_MENU_LABEL_SHORT							12507
#define IDS_DW_GDB_BROWSE_EMPTY_MENU_LABEL							12508
#define IDS_PC_LUA_EDITOR_TITLE													12509
#define IDS_PC_TXT_EDITOR_TITLE													12510


#define IDS_REF_LIST_NO_ITEMS														12600
#define IDS_REF_LIST_SET_EMPTY_FAILURE									12601
#define IDS_REF_LIST_EMPTY_ALL_LONG_TIME_WARNING				12602


#define IDS_BROWSE_BUTTON_TITLE													12610
#define IDS_NEW_BUTTON_TITLE														12611
#define IDS_EDIT_BUTTON_TITLE														12612


#define IDD_ABOUT																				22000
#define IDC_ABOUT_ICON																	22001
#define IDC_ABOUT_VERSION_LABEL_LEFT										22002
#define IDC_ABOUT_VERSION_LABEL_RIGHT										22003
#define IDC_ABOUT_COPYRIGHT_LABEL												22004
#define IDC_ABOUT_PROGRAM_TITLE_LABEL										22005

#define IDD_PC																					22010
#define IDC_PC_STATUSBAR																22011
#define IDC_PC_TREE																			22012
#define IDC_PC_ACTIVE_ITEM_EDITOR												22013

#define IDD_PC_DB_LINK																	22020
#define IDD_PC_DB_LINK_EX																22021
#define IDC_PC_DBL_TAB_LABEL														22022
#define IDC_PC_DBL_TREE_LABEL														22023
#define IDC_PC_DBL_TAB_PLACEHOLDER											22024
#define IDC_PC_DBL_TREE																	22025
#define IDC_PC_DBL_TREE_STATUSBAR												22026
#define IDC_PC_DBL_CUR_SEL_LABEL_LEFT										22027
#define IDC_PC_DBL_CUR_SEL_LABEL_RIGHT									22028
#define IDC_PC_DBL_PREV_SEL_LABEL_LEFT									22029
#define IDC_PC_DBL_PREV_SEL_LABEL_RIGHT									22030
#define IDC_PC_DBL_SET_EMPTY_BUTTON											22031
#define IDC_PC_DBL_EDITOR_LABEL													22032
#define IDC_PC_DBL_EDITOR_PLACEHOLDER										22033
#define IDC_PC_DBL_EDITOR_STATUSBAR											22034
#define IDC_PC_DBL_EDITOR																22035
#define IDC_PC_DBL_TAB																	22036

#define IDD_PC_DB_NEW_LINK															22040
#define IDC_PC_DBNL_PARENT_LABEL												22041
#define IDC_PC_DBNL_PARENT_EDIT													22042
#define IDC_PC_DBNL_TYPE_LABEL													22043
#define IDC_PC_DBNL_TYPE_COMBO													22044
#define IDC_PC_DBNL_TYPE_EDIT														22045
#define IDC_PC_DBNL_NAME_LABEL													22046
#define IDC_PC_DBNL_NAME_EDIT														22047

#define IDD_CHOOSE_TABLES																22050
#define IDC_CT_TABLES_LIST															22051

#define IDD_MENU_DROP_DOWN_LIST													22060
#define IDC_MDDL_OPERATIONS_LIST												22061

#define IDD_BIT_FIELD																		22070
#define IDC_CT_FIELDS																		22071

#define IDD_SEARCH_OBJECT																22080
#define IDC_SO_TEXT_LABEL																22081
#define IDC_SO_TEXT_EDIT																22082

#define IDD_PC_BD																				22090
#define IDC_PC_BD_NAME_EDIT															22091
#define IDC_PC_BD_TREE																	22092
#define IDC_PC_BD_NAME_LABEL														22093
#define IDC_PC_BD_TREE_LABEL														22094
#define IDC_PC_BD_TREE_STATUSBAR												22095
#define IDC_PC_BD_NEED_EXPORT_CHEKBOX										22096

#define IDD_REF_LIST																		22100
#define IDC_REF_LIST_OBJECTS														22101
#define IDC_REF_LIST_FIELDS															22102
#define IDC_REF_LIST_EMPTY_CURRENT											22103
#define IDC_REF_LIST_EMPTY_ALL													22104
#define IDD_REF_LIST_WAIT																22105

#define IDD_TEXT_EDITOR																	22110
#define IDC_TE_STATUSBAR																22111
#define IDC_TE_TEXT_PLACEHOLDER													22112
#define IDC_TE_TOOLBAR																	22113
#define	IDC_TE_TEXT																			22114


#define IDD_NEW_OBJECT																	22120
#define IDC_NO_TYPE_LABEL																22121
#define IDC_NO_TYPE_COMBO																22122
#define IDC_NO_NAME_LABEL																22123
#define IDC_NO_NAME_EDIT																22124
#define IDC_NO_NEED_EXPORT_CHEKBOX											22125
#define IDC_NO_ADD_TYPE_CHECKBOX												22126

#define IDS_CONFIRM_SAVE_MESSAGE_SHORT									22127
#define IDS_CONFIRM_SAVE_MESSAGE_LONG										22128

#define IDD_FINDTEXT																		22130
#define IDD_REPLACETEXT																	22131
#define IDD_DIALOG1																			22132
#define IDD_SCRIPT_EDITOR																22133
#define IDC_EDIT_TEXT																		22134
#define IDC_MATCHWHOLEWORD															22135
#define IDC_MATCHCASE																		22136
#define IDC_ERRLOG																			22137
#define IDC_FINDTEXT																		22138
#define IDC_REPLACETEXT																	22139
#define ID_FINDNEXT																			22140
#define ID_REPLACE																			22141
#define ID_REPLACEALL																		22142

#define IDD_PROGRESS																		22150
#define IDC_PROGRESS_LOG																22151
#define IDC_PROGRESS_LINE																22152


#define IDD_PROGRESS_SIMPLE															22155
#define IDC_PROGRESS_BAR																22156
#define IDC_PROGRESS_LABEL															22157


#define IDD_OPEN_MOD																		22160
#define IDC_OM_NAME_LABEL																22161
#define IDC_OM_NAME_COMBO																22162
#define IDC_OM_DESC_LABEL																22163
#define IDC_OM_DESC_EDIT																22164

#define IDD_CREATE_MOD																	22170
#define IDC_CM_FOLDER_LABEL															22171
#define IDC_CM_FOLDER_EDIT															22172
#define IDC_CM_NAME_LABEL																22173
#define IDC_CM_NAME_EDIT																22174
#define IDC_CM_DESC_LABEL																22175
#define IDC_CM_DESC_EDIT																22176


