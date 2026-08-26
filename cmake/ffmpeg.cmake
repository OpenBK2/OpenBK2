# On Linux the system FFmpeg is used rather than a build or a binary of this
# project's own, the same arrangement cmake/sdl.cmake makes for SDL3. Windows has
# no distro to ask, so it keeps the pinned prebuilt below.
#
# The worry this settles is whether FFmpeg's API is stable enough to link against
# whatever a distro happens to ship. For the API this tree uses, it is.
# UI/ffmpeg_facade.h is the only file that touches FFmpeg at all, and the forty
# symbols it needs are the modern core and nothing else: the send/receive decode
# API, avformat_open_input, avio_alloc_context for reading out of the game's own
# VFS, and swr_/sws_. Nothing removed or deprecated appears - no av_register_all,
# no avcodec_decode_video2, no av_free_packet, no avcodec_close - and there is not
# one version guard in the file. Checked on 2026-08-26: all forty resolve against
# system FFmpeg 8.x while the facade is written against the 7.1 headers pinned
# below, which is two soname bumps apart with nothing to fix.
#
# There is a floor, and it is real. av_channel_layout_default, av_opt_set_chlayout
# and AVCodecContext::ch_layout are the AVChannelLayout API, which arrived in
# FFmpeg 5.1; before that they do not exist under any spelling. The version
# constraints below are the library versions FFmpeg 5.1.0 shipped, so a system
# that is too old fails at configure time saying so, rather than at link time with
# three undefined symbols. Of the distributions still in support that means only
# Ubuntu 22.04 LTS is below the line.
if(WIN32)
    include(FetchContent)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(FFMPEG_URL "https://github.com/System233/ffmpeg-msvc-prebuilt/releases/download/n7.1-241205/ffmpeg-n7.1-241205-gpl-amd64-shared.zip")
        set(FFMPEG_SHA1 "7d48b59edc2aebdae45e352ca32f6b30c682352e")
    else()
        set(FFMPEG_URL https://github.com/System233/ffmpeg-msvc-prebuilt/releases/download/n7.1-241205/ffmpeg-n7.1-241205-gpl-x86-shared.zip)
        set(FFMPEG_SHA1 "e4496aa09c562f294eb421cb543afbd7ac5b3856")
    endif()

    FetchContent_Declare(
        ffmpeg
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        URL ${FFMPEG_URL}
        URL_HASH SHA1=${FFMPEG_SHA1}
    )
    FetchContent_MakeAvailable(ffmpeg)

    set(FFMPEG_ROOT "${ffmpeg_SOURCE_DIR}")
    set(FFMPEG_INCLUDE "${FFMPEG_ROOT}/include")
    set(FFMPEG_LIB "${FFMPEG_ROOT}/lib")
    set(FFMPEG_BIN "${FFMPEG_ROOT}/bin")

    message(STATUS "FFMPEG root dir = ${FFMPEG_ROOT}")
    message(STATUS "FFMPEG include dir = ${FFMPEG_INCLUDE}")
    message(STATUS "FFMPEG lib dir = ${FFMPEG_LIB}")
    message(STATUS "FFMPEG bin dir = ${FFMPEG_BIN}")

    function(add_ffmpeg_library target_name lib_name)
        set(LIB_VAR "${target_name}_LIB_PATH")
        set(DLL_VAR "${target_name}_DLL_PATH")
        find_library(${LIB_VAR} NAMES ${lib_name} PATHS ${FFMPEG_BIN} NO_DEFAULT_PATH)
        file(GLOB ${DLL_VAR} "${FFMPEG_BIN}/${lib_name}*.dll")
        add_library(${target_name} SHARED IMPORTED)
        set_target_properties(${target_name} PROPERTIES
                IMPORTED_LOCATION "${${DLL_VAR}}"
                IMPORTED_IMPLIB "${${LIB_VAR}}"
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE}"
        )
        message(STATUS "${target_name} = ${${LIB_VAR}} (${${DLL_VAR}})")
    endfunction()

    add_ffmpeg_library(ffmpeg::avcodec avcodec)
    add_ffmpeg_library(ffmpeg::avformat avformat)
    add_ffmpeg_library(ffmpeg::avutil avutil)
    add_ffmpeg_library(ffmpeg::swscale swscale)
    add_ffmpeg_library(ffmpeg::swresample swresample)

    # Copy all FFmpeg DLLs from bin/ to your output folder
    install(DIRECTORY "${FFMPEG_BIN}/"
            DESTINATION bin
            FILES_MATCHING PATTERN "*.dll")
else()
    # Every FFmpeg library installs a .pc file, and CMake ships no FindFFmpeg
    # module, so pkg-config is the route. It carries the version floor for free.
    find_package(PkgConfig REQUIRED)

    # Each gets its own call rather than one grouped call, so that the ffmpeg::
    # target names the rest of the tree links stay one per library and a failure
    # names which library was too old.
    function(add_ffmpeg_system_library target_name module_spec)
        string(REPLACE "::" "_" prefix "${target_name}")
        string(TOUPPER "${prefix}" prefix)
        pkg_check_modules(${prefix} REQUIRED IMPORTED_TARGET GLOBAL "${module_spec}")
        add_library(${target_name} ALIAS PkgConfig::${prefix})
        message(STATUS "${target_name} = system ${module_spec} (${${prefix}_VERSION})")
    endfunction()

    add_ffmpeg_system_library(ffmpeg::avcodec "libavcodec>=59.37.100")
    add_ffmpeg_system_library(ffmpeg::avformat "libavformat>=59.27.100")
    add_ffmpeg_system_library(ffmpeg::avutil "libavutil>=57.28.100")
    add_ffmpeg_system_library(ffmpeg::swscale "libswscale>=6.7.100")
    add_ffmpeg_system_library(ffmpeg::swresample "libswresample>=4.7.100")

    # Nothing to install: the libraries belong to the system, and the packaging
    # that installs the game is what declares the dependency on them.
endif()
