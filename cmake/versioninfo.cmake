set(VI_MAJOR 0)
set(VI_MINOR 0)
set(VI_PATCH 0)
if(DEFINED ENV{GITHUB_RUN_NUMBER})
    set(VI_BUILD "$ENV{GITHUB_RUN_NUMBER}")
    string(TIMESTAMP BUILD_DATE_TIME "%Y-%m-%d %H:%M:%S")
else()
    set(VI_BUILD "0")
    # fake timestamp for local builds, otherwise it will cause unnecessary rebuilds constantly
    set(BUILD_DATE_TIME "1970-01-01 00:00:00")
endif()

set(VI_PRODUCT_NAME "Blitzkrieg 2")
set(VI_COMPANY_NAME "OpenBK2")
set(VI_FILE_DESCRIPTION "Blitzkrieg 2 is a 2005 real-time tactics video game based on the events of World War II and is the second title in the Blitzkrieg series")

if(CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
    set(VI_FILEFLAGS "VS_FF_DEBUG")
else()
    set(VI_FILEFLAGS "0x0L")
endif()

set(REVISION_NUMBER "${VI_MAJOR}.${VI_MINOR}.${VI_PATCH}.${VI_BUILD}")

function(add_version_info target)
    get_target_property(TARGET_TYPE ${target} TYPE)
    if(TARGET_TYPE STREQUAL "EXECUTABLE")
        set(VI_FILETYPE "VFT_APP")
        set(VI_EXT "exe")
    elseif(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
        set(VI_FILETYPE "VFT_APP")
        set(VI_EXT "dll")
    else()
        message(WARNING "add_version_info: target ${target} is type ${TARGET_TYPE}, no version information will be generated")
        return()
    endif()
    set(TARGET_NAME "${target}")

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/versioninfo.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc
        @ONLY)

    target_sources(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc)
    target_compile_definitions(${target} PRIVATE
        REVISION_NUMBER_STR="${REVISION_NUMBER}"
        BUILD_DATE_TIME_STR="${BUILD_DATETIME}"
    )
endfunction()
