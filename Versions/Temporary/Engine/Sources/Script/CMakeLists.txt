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
    StdAfx.cpp
)
include(GenerateExportHeader)
add_library(Script SHARED ${SOURCES})
generate_export_header(Script EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Script_export.h)
target_link_libraries(Script PRIVATE MemoryLib Misc System winapi)
install(TARGETS Script DESTINATION bin)
add_version_info(Script)
