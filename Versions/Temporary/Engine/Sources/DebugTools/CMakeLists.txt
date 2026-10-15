project(DebugTools)
set(SOURCES
    DebugInfoManagerInternal.cpp
    DebugStatsSystem.cpp
    stdafx.cpp
)
bk2_add_library(DebugTools SHARED ${SOURCES})
target_link_libraries(DebugTools PRIVATE MemoryLib Misc System)
if(WIN32)
    target_link_libraries(DebugTools PRIVATE winapi)
endif()
