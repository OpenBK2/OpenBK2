project(B2_M1_Terrain)
set(SOURCES
    DBPreLight.cll
    DBPreLight.cpp
    DBTerrain.cll
    DBTerrain.cpp
    DBTerrainSpot.cll
    DBTerrainSpot.cpp
    DBVSO.cll
    DBVSO.cpp
    DBWater.cll
    DBWater.cpp
    fmtVSO.cpp
    stdafx.cpp
    TerraAIObserver.cpp
    TerrUtils.cpp
    TracksManager.cpp
)
bk2_add_library(B2_M1_Terrain SHARED ${SOURCES})
target_link_libraries(B2_M1_Terrain PRIVATE
    3DLib
    3Dmotor
    libdb
    MemoryLib
    Misc
    Sound
    System
    zlib
)
if(WIN32)
    target_link_libraries(B2_M1_Terrain PRIVATE winapi)
endif()
