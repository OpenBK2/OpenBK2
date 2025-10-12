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
include(GenerateExportHeader)
add_library(Misc SHARED ${SOURCES})
generate_export_header(Misc EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Misc_export.h)
target_link_libraries(Misc PRIVATE MemoryLib Comctl32 winapi)
target_link_libraries(Misc PUBLIC Boost::math)
install(TARGETS Misc DESTINATION bin)
add_version_info(Misc)
