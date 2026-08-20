include(FetchContent)

# The game uses mimalloc's process-wide Windows override, so only the shared
# library and its redirect DLL are needed.
set(MI_BUILD_SHARED ON CACHE BOOL mimalloc FORCE)
set(MI_BUILD_STATIC OFF CACHE BOOL mimalloc FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL mimalloc FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL mimalloc FORCE)
set(MI_OVERRIDE ON CACHE BOOL mimalloc FORCE)
set(MI_WIN_REDIRECT ON CACHE BOOL mimalloc FORCE)

# This is not the best solution, but 32 bit ARM on windows is now outdated, other 32 bit archs for windows (PowerPC, MIPS are super niche now too)
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CMAKE_SYSTEM_PROCESSOR x86)
endif()

FetchContent_Declare(
        mimalloc
        GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
        GIT_TAG v3.5.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(mimalloc)

FetchContent_GetProperties(mimalloc SOURCE_DIR MIMALLOC_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(mimalloc_targets ${MIMALLOC_SOURCE_DIR})
foreach(target IN LISTS mimalloc_targets)
    set_target_properties(${target} PROPERTIES FOLDER third_party/mimalloc)
endforeach()
