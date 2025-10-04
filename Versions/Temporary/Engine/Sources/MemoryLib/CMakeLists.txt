project(MemoryLib)
set(SOURCES
    stdafx.cpp
    SymAccess.cpp
    Trace.cpp
)
include(GenerateExportHeader)
add_library(MemoryLib SHARED ${SOURCES})
target_link_libraries(MemoryLib PRIVATE DbgHelp)
generate_export_header(MemoryLib EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/MemoryLib_export.h)
install(TARGETS MemoryLib DESTINATION bin)
