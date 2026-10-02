project(Stats_B2_M1)
set(SOURCES
    AbilityActions.cpp
    AckTypes.cll
    AckTypes.cpp
    ActionsRemap.cpp
    AIUpdates.cpp
    AnimationType.cll
    AnimationType.cpp
    Commands_Actions.cpp
    ConstructorInfo.cpp
    DBAnimB2.cll
    DBAnimB2.cpp
    DBAttachedModelVisObj.cll
    DBAttachedModelVisObj.cpp
    DBCameraConsts.cll
    DBCameraConsts.cpp
    DBClientConsts.cll
    DBClientConsts.cpp
    DBConstructorProfile.cll
    DBConstructorProfile.cpp
    DBMapInfo.cll
    DBMapInfo.cpp
    DBNotifications.cll
    DBNotifications.cpp
    DBPassProfile.cll
    DBPassProfile.cpp
    DBPlaneManuvers.cll
    DBPlaneManuvers.cpp
    dbreinforcements.cll
    dbreinforcements.cpp
    DBVisObj.cll
    DBVisObj.cpp
    FeedBackUpdates.cpp
    IconsSet.cll
    IconsSet.cpp
    M1Actions.cll
    M1Actions.cpp
    M1UnitActions.cll
    M1UnitActions.cpp
    M1UnitSpecific.cll
    M1UnitSpecific.cpp
    M1UnitType.cll
    M1UnitType.cpp
    ReinfUpdates.cpp
    RPGStats.cll
    RPGStats.cpp
    RPGStatsAddIn.cpp
    RPGStatsAutomagic.cpp
    Season.cll
    Season.cpp
    StatusUpdates.cpp
    StdAfx.cpp
    SuperWeaponUpdates.cpp
    UIEntries.cll
    UIEntries.cpp
    UnitTypes.cpp
    UserActions.cll
    UserActions.cpp
)
add_library(Stats_B2_M1 SHARED ${SOURCES})
# The generated serializers exceed COFF's section limit in x64 Debug, now
# also built by the editor's database export integration test. Leave the
# generated source alone and enable MSVC's larger object format for it.
if(MSVC)
    set_property(SOURCE RPGStats.cpp APPEND PROPERTY COMPILE_OPTIONS /bigobj)
endif()
add_stdafx_pch(Stats_B2_M1)
add_export_header(Stats_B2_M1)
target_link_libraries(Stats_B2_M1 PRIVATE 3Dmotor B2_M1_Terrain libdb MemoryLib Misc Sound System UI zlib fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Stats_B2_M1 PRIVATE winapi)
endif()
install(TARGETS Stats_B2_M1 DESTINATION bin)
add_version_info(Stats_B2_M1)
