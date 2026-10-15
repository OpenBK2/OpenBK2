project(MemoryLib)
set(SOURCES
    stdafx.cpp
    Trace.cpp
)
if(WIN32)
    list(APPEND SOURCES SymAccess.cpp)
endif()
bk2_add_library(MemoryLib SHARED ${SOURCES})
target_link_libraries(MemoryLib PRIVATE Boost::predef)
if(WIN32)
    target_link_libraries(MemoryLib PRIVATE DbgHelp winapi)
else()
    # port/messagebox.h, which stdafx.h pulls in for the debug ASSERT
    target_link_libraries(MemoryLib PRIVATE SDL3::SDL3-shared)
endif()
if(WIN32 AND USE_MIMALLOC)
    add_subdirectory(test)
endif()
