project(ED_RTS)
set(SOURCES
    ED_RTSDll.cpp
    MapEditorModule.cpp
    stdafx.cpp
)
include(GenerateExportHeader)
add_library(ED_RTS SHARED ${SOURCES})
generate_export_header(ED_RTS EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/ED_RTS_export.h)
target_link_libraries(ED_RTS PRIVATE MapEditorLib MemoryLib Misc SceneB2 System)
install(TARGETS ED_RTS DESTINATION bin)
