project(Common_RTS_AI)
set(SOURCES
    AIAngle.cpp
    AIMap.cpp
    AIObjectBase.cpp
    BasePathUnit.cpp
    CheckSumSaver.cpp
    CollisionInternal2.cpp
    CommonPathFinder.cpp
    GroupSmoothPath.cpp
    HungarianMethod.cpp
    InFormationPath.cpp
    InGroupPath.cpp
    PassMarkers.cpp
    Pathfinders.cpp
    PathSearchFuncs.cpp
    StandartDirectPath.cpp
    StandartDirPath.cpp
    StandartPath2.cpp
    StandartSmoothMechPath.cpp
    StandartSmoothPath.cpp
    StaticMapHeights.cpp
    StaticPathInternal.cpp
    StdAfx.cpp
    TerraAIObserver.cpp
    Terrain.cpp
    Tools.cpp
)
add_library(Common_RTS_AI SHARED ${SOURCES})
add_stdafx_pch(Common_RTS_AI)
add_export_header(Common_RTS_AI)
target_link_libraries(Common_RTS_AI PRIVATE DebugTools Image libdb MemoryLib Misc System zlib winapi)
install(TARGETS Common_RTS_AI DESTINATION bin)
add_version_info(Common_RTS_AI)
