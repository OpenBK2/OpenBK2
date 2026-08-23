project(Input)
set(SOURCES
    Bind.cpp
    Input.cpp
    stdafx.cpp
)
add_library(Input SHARED ${SOURCES})
add_export_header(Input)
target_link_libraries(Input PRIVATE MemoryLib Misc System dinput8 dxguid winapi)
install(TARGETS Input DESTINATION bin)
add_version_info(Input)
