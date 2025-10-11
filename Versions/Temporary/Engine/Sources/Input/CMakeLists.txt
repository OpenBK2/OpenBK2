project(Input)
set(SOURCES
    Bind.cpp
    Input.cpp
    stdafx.cpp
)
include(GenerateExportHeader)
add_library(Input SHARED ${SOURCES})
generate_export_header(Input EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Input_export.h)
target_link_libraries(Input PRIVATE MemoryLib Misc System dinput8 dxguid winapi)
install(TARGETS Input DESTINATION bin)
