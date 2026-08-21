project(Misc)
set(SOURCES
    BSUtil.cpp
    Geom.cpp
    HPTimer.cpp
    PlaneGeometry.cpp
    Progress.cpp
    Spline.cpp
    stdafx.cpp
    StrProc.cpp
    Tools.cpp
    Win32Random.cpp
)
if(WIN32)
    list(APPEND SOURCES
    BSUInit.cpp
    Asserts.cpp
    BSAssertDialog.cpp
    BSDialogFunctions.cpp
    BSExceptionDialog.cpp
)
endif()
add_library(Misc SHARED ${SOURCES})
add_stdafx_pch(Misc)
add_export_header(Misc)
target_link_libraries(Misc PRIVATE MemoryLib)
target_link_libraries(Misc PUBLIC Boost::math fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Misc PRIVATE Comctl32 winapi)
endif()
install(TARGETS Misc DESTINATION bin)
add_version_info(Misc)
