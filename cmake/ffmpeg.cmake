include(FetchContent)

# Get FFMPEG video lib - Windows for now
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
