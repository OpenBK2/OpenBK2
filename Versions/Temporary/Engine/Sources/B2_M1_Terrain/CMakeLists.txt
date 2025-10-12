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
include(GenerateExportHeader)
add_library(B2_M1_Terrain SHARED ${SOURCES})
generate_export_header(B2_M1_Terrain EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/B2_M1_Terrain_export.h)
target_link_libraries(B2_M1_Terrain PRIVATE 3DLib 3Dmotor libdb MemoryLib Misc Sound System zlib winapi)
install(TARGETS B2_M1_Terrain DESTINATION bin)
add_version_info(B2_M1_Terrain)
