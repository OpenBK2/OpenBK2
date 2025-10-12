project(DebugTools)
set(SOURCES
    DebugInfoManagerInternal.cpp
    DebugStatsSystem.cpp
    stdafx.cpp
)
include(GenerateExportHeader)
add_library(DebugTools SHARED ${SOURCES})
generate_export_header(DebugTools EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/DebugTools_export.h)
target_link_libraries(DebugTools PRIVATE MemoryLib Misc System winapi)
install(TARGETS DebugTools DESTINATION bin)
add_version_info(DebugTools)
