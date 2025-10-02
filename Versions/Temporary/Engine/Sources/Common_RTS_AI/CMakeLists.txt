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
include(GenerateExportHeader)
add_library(Common_RTS_AI SHARED ${SOURCES})
generate_export_header(Common_RTS_AI EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Common_RTS_AI_export.h)
target_link_libraries(Common_RTS_AI PRIVATE DebugTools Image libdb MemoryLib Misc System zlib)
install(TARGETS Common_RTS_AI DESTINATION bin)
