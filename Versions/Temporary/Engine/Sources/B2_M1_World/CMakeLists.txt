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
add_library(B2_M1_World SHARED ${SOURCES})
add_stdafx_pch(B2_M1_World)
add_export_header(B2_M1_World)
target_link_libraries(B2_M1_World PRIVATE 3DLib 3Dmotor DebugTools Input libdb Main MemoryLib Misc SceneB2 Script Sound Stats_B2_M1 System zlib fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(B2_M1_World PRIVATE winapi)
endif()
install(TARGETS B2_M1_World DESTINATION bin)
add_version_info(B2_M1_World)
