project(Input)
set(SOURCES
    Bind.cpp
    Input.cpp
    stdafx.cpp
)
add_library(Input SHARED ${SOURCES})
add_stdafx_pch(Input)
add_export_header(Input)
target_link_libraries(Input PRIVATE MemoryLib Misc System)
if(WIN32)
    target_link_libraries(Input PRIVATE dxsdk::dinput8 dxsdk::dxguid winapi)
else()
    # the device layer off Windows, see port/dinput.h
    target_link_libraries(Input PRIVATE SDL3::SDL3-shared)
endif()
install(TARGETS Input DESTINATION bin)
add_version_info(Input)
