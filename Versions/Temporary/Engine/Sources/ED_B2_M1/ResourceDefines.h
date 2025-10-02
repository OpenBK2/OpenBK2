#if !defined(__ED_B2_M1__RESOURCE_DEFINES__)
#define __ED_B2_M1__RESOURCE_DEFINES__
//
#include "../MapEditorLib/ResourceDefines.h"

// Common IDs
#define IDR_ED_B2_M1_MANIFEST												1


//Map Info context menu numbers
#define MICM_MAPOBJECT_OBJECT_LIST									0
#define MICM_VSO_OBJECT_LIST												1
#define MICM_MINIMAP																2
#define MICM_TERRAIN_HEIGHT_STATE_V3_TILE_LIST			3
#define MICM_MOVIES_EDITOR													4


//Model context menu numbers
#define MCM_STATE																		0


//10200...10399 Bitmaps
#define IDB_TMITH_BITMAP														10200
//
#define IDB_DMOVED_JUMP_FIRST_KEY										10250
#define IDB_DMOVED_JUMP_LAST_KEY										10251
#define IDB_DMOVED_STEP_NEXT_KEY										10252
#define IDB_DMOVED_STEP_PREV_KEY										10253
#define IDB_DMOVED_STOP_MOVIE												10254
#define IDB_DMOVED_PLAY_MOVIE												10255
#define IDB_DMOVED_PAUSE_MOVIE											10256
#define IDB_DMOVED_ADD_SEQ													10257
#define IDB_DMOVED_DEL_SEQ													10258
#define IDB_DMOVED_SETTINGS													10259

#define IDI_DMOVED_JUMP_FIRST_KEY										10260
#define IDI_DMOVED_JUMP_LAST_KEY										10261
#define IDI_DMOVED_STEP_NEXT_KEY										10262
#define IDI_DMOVED_STEP_PREV_KEY										10263
#define IDI_DMOVED_STOP_MOVIE												10264
#define IDI_DMOVED_PLAY_MOVIE												10265
#define IDI_DMOVED_PAUSE_MOVIE											10266
#define IDB_DMOVED_CHECK_RESIZE_WND									10267


//10000...10199 Toolbars
#define IDT_MAPINFO_TOOLS														10000
#define IDT_MAPINFO_VIEW														10001
#define IDT_MODEL																		10010

//10600...10799 Menus
#define IDM_MAIN																		10600
#define IDM_MAPINFO																	10610
#define IDM_MAPINFO_CONTEXT_MENU										10611
#define IDM_MODEL																		10620
#define IDM_MODEL_CONTEXT_MENU											10621
#define IDM_MINIMAP_POPUP														10630

//03000...09999 ComandIDs
// MIS = MAPINFO STATE
#define ID_MIS_CHANGE_STATE													3000
// Configuration window commands
#define ID_GET_EDIT_PARAMETERS											3020	// сконфигурировать настроечный диалог
#define ID_SET_EDIT_PARAMETERS											3021	// получить данные из настроечного диалога
#define ID_UPDATE_EDIT_PARAMETERS										3022	// обновить данные и сконфигурировать настроечный диалог
//
//	Common States commands
#define ID_WINDOW_GET_DIALOG_DATA										3081
#define ID_WINDOW_SET_DIALOG_DATA										3082
//
//	CPointsListState commands
#define ID_POINTS_LIST_DLG_CHANGE_STATE							3100
#define ID_BUILDING_CHANGE_STATE										3103
#define ID_BUILDING_POINTS_CHANGE_STATE							3104
//
//	CFormationsState commands
#define ID_FORMATION_WINDOW_CHANGE_STATE						3202
#define ID_SQUAD_CHANGE_STATE												3203
//
//	CScriptAreaState commands
#define ID_SCRIPT_AREA_WINDOW_UI_EVENT							3302	
//
//  CCameraPositionState commands
#define ID_CPE_ON_PLAYER_CHANGED										3410
#define ID_CPW_PARAM_TYPE_CHANGED										3411
#define ID_CPW_ON_SAVE															3412
//
//	CUnitStartCmdState commands
#define ID_UNIT_START_CMD_WINDOW_UI_EVENT						3420
#define ID_UNIT_START_CMD_REFRESH_WINDOW						3421
//
//	CScriptCameraState commands
#define ID_SCRIPT_CAMERA_WINDOW_UI_EVENT						3430
#define ID_SCRIPT_CAMERA_SHOW_MANUAL_CONTROLS				3431
#define ID_SCRIPT_CAMERA_GET_YAW										3432
#define ID_SCRIPT_CAMERA_SET_YAW										3433
#define ID_SCRIPT_CAMERA_GET_PITCH									3434
#define ID_SCRIPT_CAMERA_SET_PITCH									3435
#define ID_SCRIPT_CAMERA_GET_FOV										3436
#define ID_SCRIPT_CAMERA_SET_FOV										3437
#define ID_SCRIPT_CAMERA_MOV_ED_UI_EVENT						3438
//
//	CAIGeneralPointsState commands
#define ID_AIGEN_POINTS_WINDOW_UI_EVENT							3440
//
#define ID_MI_VIEW_MINIMAP													3450
#define ID_MI_VIEW_TOOL															3451
#define ID_MI_VIEW_VIEW_TOOLBAR											3452
#define ID_MI_VIEW_TOOLS_TOOLBAR										3453
//
#define ID_MI_VIEW_MOVIE_EDITOR											3454
//	CMapObjectState and CMapObjectWindow commands
#define ID_MIMO_SWITCH_ADD_STATE										3500
#define ID_MIMO_CLEAR_SELECTION											3501
#define ID_MIMO_SWITCH_MULTI_STATE									3502
//
//	CMapObjectState and CMapObjectWindow commands
#define ID_MIVSO_SWITCH_ADD_STATE										3510
#define ID_MIVSO_CLEAR_SELECTION										3511
#define ID_MIVSO_SWITCH_MULTI_STATE									3512
#define ID_MIVSO_ENABLE_HEIGHT											3513
//
//	CAdvClipboardState and CAdvClipboardWindow commands
#define ID_ADV_CLIPBOARD_WINDOW_CHANGE_STATE				3550
//
//	CReinfPointsState and CReinfPointsWindow commands
#define ID_REINF_POINTS_WINDOW_CHANGE_STATE					3560
#define ID_REINF_POINTS_WINDOW_ADD									3561
#define ID_REINF_POINTS_WINDOW_DEL									3562
#define ID_REINF_POINTS_WINDOW_SAVE									3563
//
//	MAP INFO MAP OBJECT OBJECT LIST CONTEXT MENU
#define ID_MIMOOLCM_LIST														3610
#define ID_MIMOOLCM_THUMBNAILS											3611
#define ID_MIMOOLCM_PROPERTIES											3612
//	MAP INFO VSO OBJECT LIST CONTEXT MENU
#define ID_MIVSOOLCM_LIST														3620
#define ID_MIVSOOLCM_THUMBNAILS											3621
#define ID_MIVSOOLCM_PROPERTIES											3622
//	MAP INFO MINIMAP CONTEXT MENU
#define ID_MIMCO_GENERATE_MINIMAP_IMAGE							3630
//	MAP INFO MOVIES EDITOR CONTEXT MENU
#define ID_MIMOVED_INSERT_KEY												3635
#define ID_MIMOVED_SAVE_KEY													3636
#define ID_MIMOVED_KEY_SETTINGS											3637
#define ID_MIMOVED_DELETE_KEYS											3638
//	MAP INFO TERRAIN HEIGHT V3 MESSAGES
#define ID_MITHV3_SET_TIMER													3644
#define ID_MITHV3_KILL_TIMER												3645
#define ID_MITHV3_ON_TIMER													3646
#define ID_MITHV3_UPDATE_HEIGHT											3647
#define ID_MITHV3_LIST															3648
#define ID_MITHV3_THUMBNAILS												3649
#define ID_MITHV3_PROPERTIES												3650

//	MOVIES EDITOR MESSAGES
#define ID_MOV_ED_SET_TIMER													3651
#define ID_MOV_ED_KILL_TIMER												3652
#define ID_MOV_ED_ON_TIMER													3653
#define ID_MOV_ED_RESET_DIALOG											3654

//	TOOLS MESSAGES
#define ID_TOOLS_RESET_CAMERA												3660
#define ID_TOOLS_UPDATE_VSO													3661
#define ID_TOOLS_FIT_TO_GRID												3662
#define ID_TOOLS_ROTATE_90													3663
#define ID_TOOLS_DRAW_SHOOT_AREAS										3664
#define ID_TOOLS_DRAW_AI_MAP												3665
#define ID_TOOLS_DRAW_PASSABILITY										3666
#define ID_TOOLS_SHOW_GRID													3667
#define ID_VIEW_FILTER															3668
//
#define ID_TOOLS_RUN_GAME														3669
#define ID_TOOLS_REGEN_VSO_NORMALS									3670
//
#define ID_TOOLS_DEBUG_CHECK_MAP										3671
#define ID_TOOLS_REGEN_GEOMETRY											3672
#define ID_TOOLS_CREATE_INF_ANIMS										3673
#define ID_TOOLS_CREATE_ACK_SETS										3674
#define ID_TOOLS_CREATE_VIS_OBJ											3675
//
#define ID_UPDATE_SCENE_SIZE												3676
#define ID_UPDATE_SCENE_VIEW												3677


#define ID_MODEL_VIEW_TOOLBAR												3700
#define ID_MODEL_VIEW_TOOL													3701
#define ID_MODEL_RELOAD_EDITOR											3702
#define ID_MODEL_DRAW_TERRAIN												3703
#define ID_MODEL_DRAW_ANIMATIONS										3704
#define ID_MODEL_DRAW_AI_GEOMETRY										3705
#define ID_MODEL_SET_LIGHT													3706
#define ID_MODEL_CENTER_CAMERA											3707
#define ID_MODEL_SAVE_CAMERA												3708
#define ID_MODEL_RESET_CAMERA												3709
#define ID_MODEL_SPEED_DOWN													3710
#define ID_MODEL_SPEED_UP														3711
		

//13000...19999 Strings
#define IDS_TOOLBAR_MAPINFO_TOOLS										13000
#define IDS_TOOLBAR_MAPINFO_VIEW										13001

#define IDS_TOOLBAR_MODEL														13010
#define IDS_MODEL_TOOL_WINDOW_NAME									13011
#define IDS_MODEL_SAVE_CAMERA_MESSAGE								13012

#define IDS_IS_TERRAIN_LABEL												14000
#define IDS_IS_OBJECT_LABEL													14001
#define IDS_IS_GAMEPLAY_LABEL												14002
#define IDS_IS_SCRIPT_LABEL													14003
#define IDS_IS_ADVANCED_LABEL												14004
//
#define IDS_TERRAIN_ISS_TILE_LABEL									14010
#define IDS_TERRAIN_ISS_HEIGHT_LABEL								14011
#define IDS_TERRAIN_ISS_HEIGHT_V2_LABEL							14012
#define IDS_TERRAIN_ISS_HEIGHT_V3_LABEL							14013
#define IDS_TERRAIN_ISS_FIELD_LABEL									14014
//
#define IDS_OBJECT_ISS_MAP_OBJECT_LABEL							14020
#define IDS_OBJECT_ISS_VSO_LABEL										14021
//
#define IDS_GP_ISS_REINF_POINTS_LABEL								14030
#define IDS_GP_ISS_START_CAMERA_LABEL								14031
#define IDS_GP_ISS_AIGENERAL_LABEL									14032
#define IDS_GP_ISS_UNIT_START_CMD_LABEL							14033
//
#define IDS_SCRIPT_ISS_SCRIPT_AREAS_LABEL						14040
#define IDS_SCRIPT_ISS_SCRIPT_MOVIES_LABEL					14041
//
#define IDS_ADV_ISS_CLIPBOARD												14050

#define IDS_SMOKE_POINTS														14060
#define IDS_ENTRANCE_POINTS													14061
#define IDS_FIRE_POINTS															14062
#define IDS_BUILDING_POINTS													14063
#define IDS_SURFACE_POINTS													14064
#define IDS_DAMAGE_LEVELS														14065

#define IDS_MIMO_DELETE_OBJECTS_MESSAGE							14071
#define IDS_MIMO_DELETE_OBJECT_MESSAGE							14072

#define IDS_MOV_EDITOR_ISS_EDITOR_LABEL							14080
#define IDS_STATUS_STRING_OBJECT										14081
#define IDS_STATUS_STRING_OBJECTS										14082

#define IDS_PM_PLACE_FIELD													14090

//23000...43999 диалоги
#define IDD_TAB_MI_TERRAIN_HEIGHT_V3								23080
#define IDC_TMITHV3_BRUSH_LABEL											23081
#define IDC_TMITHV3_BRUSH_TILE											23082
#define IDC_TMITHV3_BRUSH_UP												23083
#define IDC_TMITHV3_BRUSH_DOWN											23084
#define IDC_TMITHV3_BRUSH_ROUND											23085
#define IDC_TMITHV3_BRUSH_PLATO											23086
#define IDC_TMITHV3_BRUSH_SIZE_LABEL								23087
#define IDC_TMITHV3_BRUSH_SIZE_0										23088
#define IDC_TMITHV3_BRUSH_SIZE_1										23089
#define IDC_TMITHV3_BRUSH_SIZE_2										23090
#define IDC_TMITHV3_BRUSH_SIZE_3										23091
#define IDC_TMITHV3_BRUSH_SIZE_4										23092
#define IDC_TMITHV3_BRUSH_TYPE_LABEL								23093
#define IDC_TMITHV3_BRUSH_TYPE_CIRCLE								23094
#define IDC_TMITHV3_BRUSH_TYPE_SQUARE								23095
#define IDC_TMITHV3_UPDATE_HEIGHTS									23096
#define IDC_TMITHV3_TILE_LABEL											23097
#define IDC_TMITHV3_TILE_LIST												23098
//
#define IDD_TAB_MI_TERRAIN_FIELD										23100
#define IDC_TMITF_MOVE_SINGLE_RADIO									23101
#define IDC_TMITF_MOVE_MULTIPLE_RADIO								23102
#define IDC_TMITF_MOVE_ALL_RADIO										23103
#define IDC_TMITF_DELIMITER_0												23104
#define IDC_TMITF_FIELD_LABEL												23105
#define IDC_TMITF_FIELD_COMBO												23106
#define IDC_TMITF_RANDOMIZE_CHECK_BOX								23108
#define IDC_TMITF_DELIMITER_1												23115
#define IDC_TMITF_FILL_TERRAIN_CHECK_BOX						23116
#define IDC_TMITF_FILL_OBJECTS_CHECK_BOX						23117
#define IDC_TMITF_FILL_HEIGHTS_CHECK_BOX						23118
//
#define IDC_OW_PLAYER_LABEL													23131
#define IDC_OW_PLAYER_COMBO_BOX											23132
#define IDC_OW_SET_BUTTON														23133
#define IDC_OW_POSITION_ONLY_RADIO									23134
#define IDC_OW_ALL_PARAMS_RADIO											23135
//
#define IDD_TAB_BLD_POINTS													23400
#define IDC_POINTS_LIST															23401
#define IDC_SETTING_SELECT_COMBO										23402
#define IDC_CHECK_PROPMASK													23403
#define IDC_CHECK_PASSABILITY												23404
#define IDC_POINTS_LIST_TITLE												23405
#define IDC_BUTTON_RESET_STATE											23406
//
#define IDD_TAB_SQD_FORMATION												23500
#define IDC_LIST_FORMATIONS													23501
//
#define IDD_TAB_MI_SCRIPT_AREA											23800
#define IDD_DLG_AREA_NAME														23801
#define IDC_RADIO_CIRCLE														23802
#define IDC_RADIO_RECT															23803
#define IDC_EDIT_NAME																23804
#define IDC_LIST_AREA_NAMES													23805
#define IDC_SCRIPT_AREA_LABEL												23806
#define IDC_BUTTON_DEL															23807
#define IDC_BUTTON_SELECT														23808
#define IDC_STATIC_LABEL														23809
//
#define IDD_MINES_WINDOW														23900
#define IDD_DLG_EDVAL																23901
#define IDC_EDIT_VAL																23902
#define IDC_LIST_MINES_SET													23903
#define IDC_BUTTON_REFRESH													23904
#define IDC_BUTTON_APPLY														23905
#define IDC_BUTTON_DEL_MINEFIELD										23906
#define IDC_BUTTON_DEL_ALL_MINES										23907
//
#define IDD_TAB_MI_START_CAMERA											24020
#define IDC_BUTTON_SAVE															24021
//
#define IDD_DLG_MAPINFO_VIEW_FILTER									24030
#define IDC_LIST_OBJ_TYPES													24031
#define IDC_CHECK_WF																24032
#define IDC_CHECK_BB																24033
#define IDC_CHECK_GRID															24034
#define IDC_COMBO_GRID_SIZE													24035
#define IDC_CHECK_TERRAF														24036
#define IDC_CHECK_SHADOWSF													24037
#define IDC_CHECK_WARFOGF														24038
#define IDC_CHECK_STATSF														24039
#define IDC_CHECK_MIPMAPF														24040
#define IDC_CHECK_OVERDRAWF													24041
#define IDC_BUTTON_DEFAULT													24045
//
#define IDD_TAB_MI_UNIT_START_CMD										24050
#define IDD_DLG_UNIT_START_CMD											24051
#define IDC_LIST_UNIT_CMD														24052
#define IDC_BUTTON_ADD_CMD													24053
#define IDC_BUTTON_DEL_CMD													24054
#define IDC_COMBO_CMD_TYPE													24056
#define IDC_EDIT_CMD_PARAM													24057
#define IDC_BUTTON_UP																24058
#define IDC_BUTTON_DOWN															24059
#define IDC_EDIT_TARGET_UNIT												24060
#define IDC_BUTTON_CLEAR														24061
//
#define IDD_TAB_MI_MAPOBJECT_FULL										24100
#define IDD_TAB_MI_MAPOBJECT_NO_BUTTONS							24101
#define IDC_TMIMO_PLAYER_LABEL											24102
#define IDC_TMIMO_PLAYER_COMBO											24103
#define IDC_TMIMO_DIRECTION_RANDOM_COMBO						24104
#define IDC_TMIMO_DIRECTION_CUSTOM_COMBO						24105
#define IDC_TMIMO_DIRECTION_CUSTOM_EDIT							24106
#define IDC_TMIMO_DIRECTION_LABEL										24107
#define IDC_TMIMO_DIRECTION_CUSTOM_LABEL						24108
#define IDC_TMIMO_DELIMITER_0												24109
#define IDC_TMIMO_FILTER_LABEL											24110
#define IDC_TMIMO_FILTER_SHORTCUT_0									24111
#define IDC_TMIMO_FILTER_SHORTCUT_1									24112
#define IDC_TMIMO_FILTER_SHORTCUT_2									24113
#define IDC_TMIMO_FILTER_SHORTCUT_3									24114
#define IDC_TMIMO_FILTER_SHORTCUT_4									24115
#define IDC_TMIMO_FILTER_SHORTCUT_5									24116
#define IDC_TMIMO_FILTER_SHORTCUT_6									24117
#define IDC_TMIMO_FILTER_SHORTCUT_7									24118
#define IDC_TMIMO_FILTER_SHORTCUT_8									24119
#define IDC_TMIMO_FILTER_SHORTCUT_9									24120
#define IDC_TMIMO_FILTER_COMBO											24121
#define IDC_TMIMO_OBJECT_LIST												24122
//
#define IDD_TAB_MI_VSO															24130
#define IDC_TMIVSO_SINGLE_RADIO											24131
#define IDC_TMIVSO_MULTI_RADIO											24132
#define IDC_TMIVSO_ALL_RATIO												24133
#define IDC_TMIVSO_PREDEFINED_STATS_RADIO						24134
#define IDC_TMIVSO_CUSTOM_STATS_RADIO								24135
#define IDC_TMIVSO_DELIMITER_0											24136
#define IDC_TMIVSO_WIDTH_LABEL_LEFT									24137
#define IDC_TMIVSO_WIDTH														24138
#define IDC_TMIVSO_WIDTH_LABEL_RIGHT								24139
#define IDC_TMIVSO_OPACITY_LABEL_LEFT								24140
#define IDC_TMIVSO_OPACITY													24141
#define IDC_TMIVSO_OPACITY_LABEL_RIGHT							24142
#define IDC_TMIVSO_DELIMITER_1											24143
#define IDC_TMIVSO_FILTER_LABEL											24144
#define IDC_TMIVSO_FILTER_COMBO											24145
#define IDC_TMIVSO_OBJECT_LIST											24146
//
#define IDD_MAPINFO_AI_MARKERS											24150
#define IDC_COMBO_PLAYER														24151
#define IDC_LIST_UNIT_TYPE													24152
#define IDC_CHECK_SELECTION													24153
//
#define IDD_TAB_MI_ADV_CLIPBOARD										24160
#define IDC_BUTTON_COPY															24161
#define IDC_BUTTON_PASTE														24162
#define IDC_BUTTON_SAVE_CLIP												24163
#define IDC_LIST_CLIPS															24164
#define IDC_BUTTON_LOAD_CLIP												24165
//
#define IDD_TAB_MI_REINF_POINTS											24170
#define IDC_LIST_REINF_POINTS												24171
#define IDC_BUTTON_REINF_POINTS_ADD									24172
#define IDC_BUTTON_REINF_POINTS_DEL									24173
#define IDC_BUTTON_REINF_POINTS_DEPLOY							24174
#define IDC_BUTTON_REINF_POINTS_TYPED								24175
//
#define IDD_DLG_REINFPTS_TEMPLATES									24180
#define IDC_LIST_REINF_POINTS_TYPED_TEMPLATE				24181
#define IDC_REINF_TYPED_ADD													24182
#define IDC_TYPED_REMOVE														24183
#define IDC_TYPED_TYPE															24184
#define IDC_TYPED_TEMPLATE													24185
#define IDC_RP_DELIMITER_0													24186
//
#define IDD_DLG_REINFPTS_ADD_TEMPLATE								24190
#define IDC_TYPED_TEMPL_LINK												24191
#define IDC_EDIT_TEMPLATE														24192
//
#define IDD_TAB_MI_SCRIPT_MOVIES										24200
#define IDC_SMOV_BUTTON_ADD    											24201
#define IDC_SMOV_BUTTON_DEL 												24202
#define IDC_SMOV_BUTTON_SAVE												24203
#define IDC_SMOV_BUTTON_RUN													24204
#define IDC_SMOV_LIST_LABEL													24205
#define IDC_SMOV_LIST         											24206
#define IDC_SMOV_YAW_LABEL_LEFT											24207
#define IDC_SMOV_YAW_EDIT														24208
#define IDC_SMOV_YAW_LABEL_RIGHT										24209
#define IDC_SMOV_PITCH_LABEL_LEFT										24210
#define IDC_SMOV_PITCH_EDIT													24211
#define IDC_SMOV_PITCH_LABEL_RIGHT									24212
#define IDC_SMOV_FOV_LABEL_LEFT											24213
#define IDC_SMOV_FOV_EDIT														24214
#define IDC_SMOV_FOV_LABEL_RIGHT										24215
//
#define IDD_DLG_SCRIPT_CAMERA_ADD										24230
#define IDC_SCAD_EDIT_NAME													24231
#define IDC_SCAD_LABEL_NAME													24232
//
#define IDD_DLG_SCRIPT_CAMERA_RUN										24240
#define IDC_SCRUN_COMBO_START												24241
#define IDC_SCRUN_COMBO_FINISH											24242
#define IDC_SCRUN_COMBO_TYPE												24243
#define IDC_SCRUN_EDIT_TIME													24244
#define IDC_SCRUN_EDIT_LSPEED												24245
#define IDC_SCRUN_EDIT_ASPEED												24246
#define IDC_SCRUN_LABEL_START												24247
#define IDC_SCRUN_LABEL_FINISH											24248
#define IDC_SCRUN_LABEL_TYPE												24249
#define IDC_SCRUN_LABEL_TIME												24250
#define IDC_SCRUN_LABEL_LSPEED											24251
#define IDC_SCRUN_LABEL_ASPEED											24252
#define IDC_SCRUN_EDIT_TARGET												24253
#define IDC_SCRUN_LABEL_TARGET											24254
#define IDC_SCRUN_EDIT_SPLINE1											24255
#define IDC_SCRUN_LABEL_SPLINE1											24256
#define IDC_SCRUN_EDIT_SPLINE2											24257
#define IDC_SCRUN_LABEL_SPLINE2											24258
#define IDC_SCRUN_DELIMITER0												24259
//
#define IDD_TAB_MI_AIGENERAL												24260
#define IDC_AIGEN_LIST_PARCELS											24261
#define IDC_AIGEN_LIST_IDS													24262
#define IDC_AIGEN_DELIMITER_0												24263
#define IDC_AIGEN_DELIMITER_1												24264
#define IDC_AIGEN_BUTTON_ADD_PARCEL									24265
#define IDC_AIGEN_BUTTON_DEL_PARCEL									24266
#define IDC_AIGEN_BUTTON_ADD_ID											24267
#define IDC_AIGEN_BUTTON_DEL_ID											24268
#define IDC_AIGEN_LABEL_ID													24269
#define IDC_AIGEN_LABEL_PARCELS											24270
#define IDC_AIGEN_LABEL_IDS													24271
//
#define IDD_DLG_AIGEN_PARCEL												24280
#define IDC_AIGEN_LABEL_PTYPE												24281
#define IDC_AIGEN_COMBO_PTYPE												24282
#define IDC_AIGEN_LABEL_IMPORTANCE									24283
#define IDC_AIGEN_EDIT_IMPORTANCE										24284
//
#define IDD_DLG_AIGEN_MOBILE_ID											24290
#define IDC_AIGEN_LABEL_MOBILE_ID										24291
#define IDC_AIGEN_EDIT_MOBILE_ID										24292
//
#define IDD_TAB_MODEL_TOOL													24300
#define IDC_MODEL_LIGHT_LABEL												24301
#define IDC_MODEL_LIGHT_COMBO												24302
#define IDC_MODEL_LIGHT_BUTTON											24303
#define IDC_MODEL_SCENE_COLOR_LABEL									24304
#define IDC_MODEL_SCENE_COLOR_EDIT									24305
#define IDC_MODEL_SCENE_COLOR_BUTTON								24306
#define IDC_MODEL_FOV_LABEL_LEFT										24307
#define IDC_MODEL_FOV_EDIT													24308
#define IDC_MODEL_FOV_LABEL_RIGHT										24309
#define IDC_MODEL_DELIMITER_0												24310
#define IDC_MODEL_TERRAIN_CHECK											24311
#define IDC_MODEL_TERRAIN_SIZE_LABEL								24312
#define IDC_MODEL_TERRAIN_SIZE_COMBO								24313
#define IDC_MODEL_TERRAIN_COLOR_LABEL								24314
#define IDC_MODEL_TERRAIN_COLOR_EDIT								24315
#define IDC_MODEL_TERRAIN_COLOR_BUTTON							24316
#define IDC_MODEL_TERRAIN_COLOR_OPACITY_LABEL_LEFT	24317
#define IDC_MODEL_TERRAIN_COLOR_OPACITY_EDIT				24318
#define IDC_MODEL_TERRAIN_COLOR_OPACITY_LABEL_RIGHT	24319
#define IDC_MODEL_TERRAIN_DOUBLESIDED_CHECK					24320
#define IDC_MODEL_TERRAIN_GRID_CHECK								24321
#define IDC_MODEL_DELIMITER_1												24322
#define IDC_MODEL_ANIM_CHECK												24323
#define IDC_MODEL_ANIM_COUNT_LABEL									24324
#define IDC_MODEL_ANIM_COUNT_COMBO									24325
#define IDC_MODEL_ANIM_SPEED_LABEL									24326
#define IDC_MODEL_ANIM_SPEED_COMBO									24327
#define IDC_MODEL_ANIM_SPEED_BUTTON_DOWN						24328
#define IDC_MODEL_ANIM_SPEED_BUTTON_UP							24329
#define IDC_MODEL_ANIM_CIRCLE_RADIO									24330
#define IDC_MODEL_ANIM_LINE_RADIO										24331
#define IDC_MODEL_ANIM_CIRCLE_RADIUS_LABEL					24332
#define IDC_MODEL_ANIM_CIRCLE_RADIUS_COMBO					24333
#define IDC_MODEL_ANIM_LINE_DISTANCE_LABEL					24334
#define IDC_MODEL_ANIM_LINE_DISTANCE_COMBO					24335
#define IDC_MODEL_DELIMITER_2												24336
#define IDC_MODEL_AI_GEOMETRY												24337
#define IDC_MODEL_AI_GEOMETRY_TRANSPARENT_RADIO			24338
#define IDC_MODEL_AI_GEOMETRY_SOLID_RADIO						24339
//
#define IDD_DLG_MOVIES_EDITOR												24430
#define IDC_MOVIES_TIMELINE													24431
#define IDC_DMOVED_VIEW_MODE_CHECK_BOX							24432
#define IDC_DMOVED_TIMELINE_COMBO										24433
#define IDC_DMOVED_TIMELINE_LABEL										24434
#define IDC_DMOVED_PKEY_RESIZE_LABEL								24435
#define IDC_DMOVED_CKEY_RESIZE_LABEL								24436
#define IDC_DMOVED_NKEY_RESIZE_LABEL								24437
#define IDC_DMOVED_PKEY_EDIT												24438
#define IDC_DMOVED_CKEY_EDIT												24439
#define IDC_DMOVED_NKEY_EDIT												24440
#define IDC_DMOVED_POUT_LABEL												24441
#define IDC_DMOVED_CIN_LABEL												24442
#define IDC_DMOVED_COUT_LABEL												24443
#define IDC_DMOVED_NIN_LABEL												24444
#define IDC_DMOVED_POUT_FLAT_RADIO									24445
#define IDC_DMOVED_POUT_TANGENT_RADIO								24446
#define IDC_DMOVED_CIN_FLAT_RADIO										24447
#define IDC_DMOVED_CIN_TANGENT_RADIO								24448
#define IDC_DMOVED_COUT_FLAT_RADIO									24449
#define IDC_DMOVED_COUT_TANGENT_RADIO								24450
#define IDC_DMOVED_NIN_FLAT_RADIO										24451
#define IDC_DMOVED_NIN_TANGENT_RADIO								24452
#define IDC_DMOVED_FLAT_LABEL												24453
#define IDC_DMOVED_TANGENT_LABEL										24454
#define IDC_DMOVED_FLAT_LABEL2											24455
#define IDC_DMOVED_TANGENT_LABEL2										24456
#define IDC_DMOVED_FLAT_LABEL3											24457
#define IDC_DMOVED_TANGENT_LABEL3										24458
#define IDC_DMOVED_MOV_SELECT_COMBO									24459
#define IDC_DMOVED_MOV_LABEL												24460
#define IDC_DMOVED_TIME_SLIDER_PLACE								24461
#define IDC_DMOVED_CHECK_RESIZE_WND									24462
//
#define IDC_DMOVED_TIME_SLIDER											24470
#define IDC_DMOVED_JUMP_FIRST_KEY_BUTTON						24471
#define IDC_DMOVED_JUMP_LAST_KEY_BUTTON							24472
#define IDC_DMOVED_STEP_PREV_KEY_BUTTON							24473
#define IDC_DMOVED_STEP_NEXT_KEY_BUTTON							24474
#define IDC_DMOVED_STOP_MOVIE_BUTTON								24475
#define IDC_DMOVED_PLAY_PAUSE_MOVIE_BUTTON					24476
#define IDC_DMOVED_ADD_SEQ_BUTTON										24477
#define IDC_DMOVED_DEL_SEQ_BUTTON										24478
#define IDC_DMOVED_LOG															24479
#define IDC_DMOVED_SCALE_SLIDER											24480
#define IDC_DMOVED_SETTINGS_BUTTON									24481
#define IDC_DMOVED_TIME_EDIT												24482
#define IDC_MOVED_SPEED_COMBO												24483
#define IDC_DMOVED_SPEED_LABEL											24484
//
#define IDD_DLG_MOVED_SETTINGS											24500
#define IDC_MOVEDS_LEN_LABEL												24501
#define IDC_MOVEDS_LEN_EDIT													24502
//
#define IDD_DLG_MOVED_KEY_SETTINGS									24510
#define IDC_MOVEDKEY_NAME               						24511
#define IDC_MOVEDKEY_STATIC_IN          						24512
#define IDC_MOVEDKEY_STATIC_OUT          						24513
#define IDC_MOVEDKEY_RADIO_IN_FLAT      						24514
#define IDC_MOVEDKEY_RADIO_IN_TANG      						24515
#define IDC_MOVEDKEY_RADIO_OUT_FLAT     						24516
#define IDC_MOVEDKEY_RADIO_OUT_TANG     						24517
#define IDC_MOVEDKEY_PARAM_LABEL        						24518
#define IDC_MOVEDKEY_PARAM_EDIT         						24519


// MapInfo window IDs
#define ID_MAPINFO_EDITOR_SHORTCUT_DW								(IDC_DOCKING_WINDOW_0 + 0)
#define ID_MAPINFO_EDITOR_MINIMAP_DW								(IDC_DOCKING_WINDOW_0 + 1)
#define ID_MAPINFO_EDITOR_SHORTCUT_PANE_0						(IDC_DOCKING_WINDOW_0 + 2)
//
#define ID_BUILDING_EDITOR_DW												(IDC_DOCKING_WINDOW_0 + 20)
#define ID_BUILDING_EDITOR_SHORTCUT_PANE_0					(IDC_DOCKING_WINDOW_0 + 21)
//
#define ID_SQUAD_EDITOR_DW													(IDC_DOCKING_WINDOW_0 + 30)
#define ID_SQUAD_EDITOR_SHORTCUT_PANE_0							(IDC_DOCKING_WINDOW_0 + 31)
//
#define ID_MODEL_EDITOR_DW													(IDC_DOCKING_WINDOW_0 + 40)
//
#define ID_MOVIES_EDITOR_DW													(IDC_DOCKING_WINDOW_0 + 50)

#endif // !defined(__ED_B2_M1__RESOURCE_DEFINES__)
