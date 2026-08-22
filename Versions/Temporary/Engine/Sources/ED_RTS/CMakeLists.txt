project(ED_RTS)
set(SOURCES
    ED_RTSDll.cpp
    MapEditorModule.cpp
    stdafx.cpp
)
add_library(ED_RTS SHARED ${SOURCES})
add_stdafx_pch(ED_RTS)
add_export_header(ED_RTS)
target_link_libraries(ED_RTS PRIVATE MapEditorLib MemoryLib Misc SceneB2 System winapi)
install(TARGETS ED_RTS DESTINATION bin)
add_version_info(ED_RTS)
