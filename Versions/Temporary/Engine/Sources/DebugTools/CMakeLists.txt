project(DebugTools)
set(SOURCES
    DebugInfoManagerInternal.cpp
    DebugStatsSystem.cpp
    stdafx.cpp
)
add_library(DebugTools SHARED ${SOURCES})
add_stdafx_pch(DebugTools)
add_export_header(DebugTools)
target_link_libraries(DebugTools PRIVATE MemoryLib Misc System winapi)
install(TARGETS DebugTools DESTINATION bin)
add_version_info(DebugTools)
