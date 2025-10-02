project(Stats_B2_M1)
set(SOURCES
    AbilityActions.cpp
    AckTypes.cpp
    ActionsRemap.cpp
    AIUpdates.cpp
    AnimationType.cpp
    Commands_Actions.cpp
    ConstructorInfo.cpp
    DBAnimB2.cpp
    DBAttachedModelVisObj.cpp
    DBCameraConsts.cpp
    DBClientConsts.cpp
    DBConstructorProfile.cpp
    DBMapInfo.cpp
    DBNotifications.cpp
    DBPassProfile.cpp
    DBPlaneManuvers.cpp
    dbreinforcements.cpp
    DBVisObj.cpp
    FeedBackUpdates.cpp
    IconsSet.cpp
    M1Actions.cpp
    M1UnitActions.cpp
    M1UnitSpecific.cpp
    M1UnitType.cpp
    ReinfUpdates.cpp
    RPGStats.cpp
    RPGStatsAddIn.cpp
    RPGStatsAutomagic.cpp
    Season.cpp
    StatusUpdates.cpp
    stdafx.cpp
    SuperWeaponUpdates.cpp
    UIEntries.cpp
    UnitTypes.cpp
    UserActions.cpp
)
include(GenerateExportHeader)
add_library(Stats_B2_M1 SHARED ${SOURCES})
generate_export_header(Stats_B2_M1 EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Stats_B2_M1_export.h)
target_link_libraries(Stats_B2_M1 PRIVATE 3Dmotor B2_M1_Terrain libdb MemoryLib Misc Sound System UI zlib)
install(TARGETS Stats_B2_M1 DESTINATION bin)
