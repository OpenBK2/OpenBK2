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
bk2_add_library(Script SHARED ${SOURCES})
target_link_libraries(Script PRIVATE MemoryLib Misc System fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Script PRIVATE winapi)
endif()
