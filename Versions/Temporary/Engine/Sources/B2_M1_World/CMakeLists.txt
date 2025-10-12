project(B2_M1_World)
set(SOURCES
    AllAnimationsPlayer.cpp
    B2MapObj.cpp
    ClientAckManagerInternal.cpp
    CommonB2M1AI.cpp
    DeadHouseAnimation.cpp
    DeadHouseAnimations.cpp
    IdleMechProcess.cpp
    MapObj.cpp
    MOBridge.cpp
    MOBuilding.cpp
    MOEntrenchment.cpp
    MOFence.cpp
    MOObject.cpp
    MOProjectile.cpp
    MOSquad.cpp
    MOUnit.cpp
    MOUnitHelicopter.cpp
    MOUnitInfantry.cpp
    MOUnitMechanical.cpp
    ParatrooperAnimation.cpp
    SmokeTrailEffect.cpp
    StdAfx.cpp
    UpdatableWorld.cpp
)
include(GenerateExportHeader)
add_library(B2_M1_World SHARED ${SOURCES})
generate_export_header(B2_M1_World EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/B2_M1_World_export.h)
target_link_libraries(B2_M1_World PRIVATE 3DLib 3Dmotor DebugTools Input libdb Main MemoryLib Misc SceneB2 Script Sound Stats_B2_M1 System zlib winapi)
install(TARGETS B2_M1_World DESTINATION bin)
add_version_info(B2_M1_World)
