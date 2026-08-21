project(Script)
set(SOURCES
    CommonFunctions.cpp
    lapi.cpp
    lcode.cpp
    ldebug.cpp
    ldo.cpp
    lfunc.cpp
    lgc.cpp
    llex.cpp
    lmem.cpp
    lobject.cpp
    lparser.cpp
    lsaver.cpp
    lstate.cpp
    lstring.cpp
    ltable.cpp
    ltm.cpp
    lundump.cpp
    lvm.cpp
    lzio.cpp
    Script.cpp
    scriptCommon.cpp
    scriptPtr.cpp
    ScriptWrapperInternal.cpp
    stdafx.cpp
)
add_library(Script SHARED ${SOURCES})
add_stdafx_pch(Script)
add_export_header(Script)
target_link_libraries(Script PRIVATE MemoryLib Misc System fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Script PRIVATE winapi)
endif()
install(TARGETS Script DESTINATION bin)
add_version_info(Script)
