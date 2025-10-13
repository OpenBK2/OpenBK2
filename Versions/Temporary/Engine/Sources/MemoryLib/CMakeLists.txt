project(MemoryLib)
set(SOURCES
    stdafx.cpp
    SymAccess.cpp
    Trace.cpp
)
add_library(MemoryLib SHARED ${SOURCES})
add_stdafx_pch(MemoryLib)
target_link_libraries(MemoryLib PRIVATE DbgHelp winapi Boost::predef)
add_export_header(MemoryLib)
install(TARGETS MemoryLib DESTINATION bin)
add_version_info(MemoryLib)
