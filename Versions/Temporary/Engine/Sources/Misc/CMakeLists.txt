project(Misc)
set(SOURCES
    Asserts.cpp
    BSAssertDialog.cpp
    BSDialogFunctions.cpp
    BSExceptionDialog.cpp
    BSUInit.cpp
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
add_library(Misc SHARED ${SOURCES})
add_stdafx_pch(Misc)
add_export_header(Misc)
target_link_libraries(Misc PRIVATE MemoryLib Comctl32 winapi)
target_link_libraries(Misc PUBLIC Boost::math fmt::fmt-header-only)
install(TARGETS Misc DESTINATION bin)
add_version_info(Misc)
