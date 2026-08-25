project(MemoryLib)
set(SOURCES
    stdafx.cpp
    Trace.cpp
)
if(WIN32)
    list(APPEND SOURCES SymAccess.cpp)
endif()
add_library(MemoryLib SHARED ${SOURCES})
add_stdafx_pch(MemoryLib)
target_link_libraries(MemoryLib PRIVATE Boost::predef)
if(WIN32)
    target_link_libraries(MemoryLib PRIVATE DbgHelp winapi)
else()
    # port/messagebox.h, which stdafx.h pulls in for the debug ASSERT
    target_link_libraries(MemoryLib PRIVATE SDL3::SDL3-shared)
endif()
add_export_header(MemoryLib)
install(TARGETS MemoryLib DESTINATION bin)
add_version_info(MemoryLib)
