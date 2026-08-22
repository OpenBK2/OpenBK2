project(B2_M1_Terrain)
set(SOURCES
    DBPreLight.cpp
    DBTerrain.cpp
    DBTerrainSpot.cpp
    DBVSO.cpp
    DBWater.cpp
    fmtVSO.cpp
    stdafx.cpp
    TerraAIObserver.cpp
    TerrUtils.cpp
    TracksManager.cpp
)
add_library(B2_M1_Terrain SHARED ${SOURCES})
add_stdafx_pch(B2_M1_Terrain)
add_export_header(B2_M1_Terrain)
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
install(TARGETS B2_M1_Terrain DESTINATION bin)
add_version_info(B2_M1_Terrain)
