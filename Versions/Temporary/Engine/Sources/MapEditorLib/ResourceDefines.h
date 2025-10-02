#pragma once


//00001...00135 служебные ID
//00140...00199 Status bar panels
//00200...00299 Child Frames
//00300...00399 DockingWindows
//00400...00999 Trees
//01000...02999 служебные ComandIDs
//03000...09999 ComandIDs
//10000...10199 Toolbars
//10200...10399 Bitmaps
//10400...10599 Cursors
//10600...10799 Menus
//10800...10999 зарезервировано
//11000...11099 служебные Toolbars
//11100...11199 служебные Bitmaps
//11200...11299 служебные Cursors
//11300...11399 служебные Menus
//11400...11499 зарезервировано
//11500...12999 служебные Strings
//13000...19999 Strings
// OT Standard lower resource range 20500 - 20999
// OT Pro lower resource range 21000 - 21499
//22000...22999 служебные диалоги
//23000...43999 диалоги
// OT Standard upper resource range 43000 - 43499
// OT Pro upper resource range 43500 - 43999

// status bar
#define	ID_INDICATOR_0															140
#define ID_INDICATOR_1															141 //140...199 Status bar panels

#define IDR_EDITORTYPE															128

#define IDA_MAIN																		128
#define IDA_MODAL																		129

#define	IDR_CHILD_FRAME_0														200 //200...299 Child Frames

#define	IDC_DOCKING_WINDOW_0												300 //300...399 DockingWindows

#define IDC_GDB_TREE_0															400 //400..899 Trees

#define ID_VIEW_TOOLBAR_MAIN												900
#define ID_VIEW_TOOLBAR_CONTROLLER_CONTAINER				901
#define ID_VIEW_TOOLBAR_SELECTION										902
#define ID_VIEW_TOOLBAR_OBJECT											903
#define ID_VIEW_TOOLBAR_PROPERTY_CONTROL						904
#define ID_VIEW_TOOLBAR_VIEW												905
#define ID_VIEW_DW_PROPERTY_BROWSER									906
#define ID_VIEW_DW_LOG															907
#define ID_VIEW_DW_GDB_BROWSER_NEW									908
#define ID_VIEW_DW_GDB_BROWSER_REMOVE								909
#define ID_VIEW_DW_GDB_BROWSER_FIRST								910
#define ID_VIEW_DW_GDB_BROWSER_LAST									919

#define ID_TOOLS_CUSTOMIZE													920

#define ID_HELP_CONTENTS														930
#define ID_HELP_ABOUT																931

//1000...9999 ComandIDs
#define ID_FIRST_COMMAND_ID													1000 
#define ID_LAST_COMMAND_ID													9999

//13000...19999 Strings
#define IDS_FIRST_STRING_ID													13000
#define IDS_LAST_STRING_ID													19999

//23000...	
#define IDS_FIRST_DIALOG_ID													23000

#define ID_OS_GET_OBJECTSET													1000 //Получить описатель текущего объекта
#define ID_OS_GET_SELECTION 												1002 //Получить список всех выделенных объектов ( и каталогов )

#define ID_VIEW_FIRST_COMMAND_ID										1010
#define ID_VIEW_SHOW_PROPERTY_BROWSER								1010
#define ID_VIEW_SHOW_LOG														1011
#define ID_VIEW_SHOW_GDB_BROWSER										1012
#define ID_VIEW_SAVE_CHANGES												1013
#define ID_VIEW_RELOAD															1014
#define ID_VIEW_LAST_COMMAND_ID											1014
#define ID_VIEW_APPLY_MI_FILTER											1019

#define ID_SCENE_UPDATE															1020 //Перерисовать окно
#define ID_SCENE_RESET_CAMERA												1021 //Поставить камеру в нормальное положение
#define ID_SCENE_UPDATE_CAMERA											1022 //Обновить камеру в соответствии с состоянием клавиатуры
#define ID_SCENE_REMOVE_INPUT												1023 //Убрать все сообщения от устройств ввода
#define ID_SCENE_ENABLE_INPUT												1024 //Запретить / разрешить ввод
#define ID_SCENE_SET_CAMERA_POSITION								1025 //move camera's anchor point to new position
#define ID_SCENE_CLEAR															1026 //Удалить все графические данные зи памяти и очистить сцену
#define ID_SCENE_GET_DIMENSIONS											1027 //Получить размеры экрана
#define ID_SCENE_ENABLE_RUN_MODE									  1028 //Запретить / разрешить основной цикл игры
#define ID_SCENE_ENABLE_RENDER											1029 //Запретить / разрешить отрисовку из OnPaint
#define ID_SCENE_ENABLE_GAME_INPUT									1030 //Enable game input message handling
#define ID_SCENE_DISABLE_GAME_INPUT									1031 //Disable game input message handling
#define ID_SCENE_ENABLE_UPDATE											1032 //Enable / Disable manual scene update
#define ID_SCENE_ENABLE_SCROLLBARS									1033 //Показать/Убрать Scrollbars
#define ID_SCENE_ENABLE_MOUSE_CAPTURE								1034 //Направить поток сообщений от мыши в окно редактора
#define ID_SCENE_SET_FOCUS													1035 //Установить фокус на основное окно
#define ID_SCENE_GET_FOCUS													1036 //Установлен ли фокус
#define ID_SCENE_SHOW_STATISTIC											1037 //показать / отключить статистику (в пареметре - указатель на bool, что учтановили)
#define ID_SCENE_SHOW_MOVIE_BORDERS									1038 //показать / отключить поля в редакторе роликов
#define ID_SCENE_RESIZE_TO_GAME											1039 //4:3

#define ID_MAIN_FIRST_COMMAND_ID										1040
#define ID_MAIN_NEW																	1040
#define ID_MAIN_OPEN																1041
#define ID_MAIN_CLOSE																1042
#define ID_MAIN_NEW_RESOURCE												1043
#define ID_MAIN_OPEN_RESOURCE												1044
#define ID_MAIN_SAVE																1045
#define ID_MAIN_RELOAD															1046
#define ID_MAIN_SELECT															1047
#define ID_MAIN_RECENT_0														1048
#define ID_MAIN_RECENT_1														1049
#define ID_MAIN_RECENT_2														1050
#define ID_MAIN_RECENT_3														1051
#define ID_MAIN_RECENT_4														1052
#define ID_MAIN_RECENT_5														1053
#define ID_MAIN_RECENT_6														1054
#define ID_MAIN_RECENT_7														1055
#define ID_MAIN_RECENT_8														1056
#define ID_MAIN_RECENT_9														1057
#define ID_MAIN_RECENT_RESOURCE_0										1058
#define ID_MAIN_RECENT_RESOURCE_1										1059
#define ID_MAIN_RECENT_RESOURCE_2										1060
#define ID_MAIN_RECENT_RESOURCE_3										1061
#define ID_MAIN_RECENT_RESOURCE_4										1062
#define ID_MAIN_RECENT_RESOURCE_5										1063
#define ID_MAIN_RECENT_RESOURCE_6										1064
#define ID_MAIN_RECENT_RESOURCE_7										1065
#define ID_MAIN_RECENT_RESOURCE_8										1066
#define ID_MAIN_RECENT_RESOURCE_9										1067
#define ID_MAIN_GETLATEST														1070
#define ID_MAIN_CHECKOUT														1071
#define ID_MAIN_CHECKIN															1072
#define ID_MAIN_NEW_MOD															1073
#define ID_MAIN_OPEN_MOD														1074
#define ID_MAIN_CLOSE_MOD														1075
#define ID_MAIN_OBJECT_LOCATE												1076
#define ID_MAIN_LAST_COMMAND_ID											1079

#define ID_SELECTION_FIRST_COMMAND_ID								1080
#define ID_SELECTION_CUT														1080
#define ID_SELECTION_COPY														1081
#define ID_SELECTION_PASTE													1082
#define ID_SELECTION_RENAME													1083
#define ID_SELECTION_NEW														1084
#define ID_SELECTION_CLEAR													1085
#define ID_SELECTION_SELECT_ALL											1086
#define ID_SELECTION_FIND														1087
#define ID_SELECTION_PROPERTIES											1088
#define ID_SELECTION_LAST_COMMAND_ID								1089

#define ID_CC_FIRST_COMMAND_ID											1090
#define ID_CC_UNDO																	1090
#define ID_CC_REDO																	1091
#define ID_CC_UNDO_ARROW														1092
#define ID_CC_REDO_ARROW														1093
#define ID_CC_LAST_COMMAND_ID												1093

#define ID_OBJECT_FIRST_COMMAND_ID									1100
#define ID_OBJECT_LOAD															1100
#define ID_OBJECT_LOCATE														1101
#define ID_OBJECT_NEW_FOLDER												1102
#define ID_OBJECT_NEW																1103
#define ID_OBJECT_NEW_FOLDER_AT_ROOT								1104
#define ID_OBJECT_NEW_AT_ROOT												1105
#define ID_OBJECT_CHECK															1106
#define ID_OBJECT_EXPORT														1107
#define ID_OBJECT_EXPORT_FORCE											1108
#define ID_OBJECT_EXPORT_NO_REF											1109
#define ID_OBJECT_EXPORT_NO_REF_FORCE								1110
#define ID_OBJECT_COLOR															1111
#define ID_OBJECT_REF_LOOKUP												1112
#define ID_OBJECT_LAST_COMMAND_ID										1112

#define ID_PC_FIRST_COMMAND_ID											1120
#define ID_PC_EXPAND_ALL														1120
#define ID_PC_EXPAND																1121
#define ID_PC_COLLAPSE															1122
#define ID_PC_COLLAPSE_ALL													1123
#define ID_PC_OPTIMAL_WIDTH													1124
#define ID_PC_REFRESH																1125
#define ID_PC_ADD_NODE															1126
#define ID_PC_DELETE_ALL_NODES											1127
#define ID_PC_INSERT_NODE														1128
#define ID_PC_DELETE_NODE														1129
#define ID_PC_SHOW_HIDDEN														1130
#define ID_PC_LAST_COMMAND_ID												1130

#define ID_PC_DIALOG_GET_VIEW												1140
#define ID_PC_DIALOG_GET_COMMAND_HANDLER						1141
#define ID_PC_DIALOG_CREATE_TREE										1142
#define ID_PC_DIALOG_UPDATE_VALUES									1143

#define ID_TE_SAVE																	1180
#define ID_TE_CLOSE																	1181
#define ID_TE_UNDO																	1182
#define ID_TE_REDO																	1183
#define ID_TE_CUT																		1184
#define ID_TE_COPY																	1185
#define ID_TE_PASTE																	1186
#define ID_TE_CLEAR																	1187
#define ID_TE_SELECT_ALL														1188
#define ID_TE_FIND																	1189

#define ID_LOG_FIRST_COMMAND_ID											1190
#define ID_LOG_SHOW_MESSAGES												1190
#define ID_LOG_SHOW_WARNINGS												1191
#define ID_LOG_SHOW_ERRORS													1192
#define ID_LOG_CLEAR_ALL														1193
#define ID_LOG_LAST_COMMAND_ID											1193


