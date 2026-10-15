project(Input)
set(SOURCES
    Bind.cpp
    Input.cpp
    stdafx.cpp
)
bk2_add_library(Input SHARED ${SOURCES})
target_link_libraries(Input PRIVATE MemoryLib Misc System)
if(WIN32)
    target_link_libraries(Input PRIVATE dxsdk::dinput8 dxsdk::dxguid winapi)
else()
    # the device layer off Windows, see port/dinput.h
    target_link_libraries(Input PRIVATE SDL3::SDL3-shared)
endif()
