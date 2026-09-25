include(FetchContent)

# All participating DLLs must use the same allocator instance: engine objects
# and STL containers are routinely created in one module and deleted in another.
set(MI_BUILD_SHARED ON CACHE BOOL mimalloc FORCE)
set(MI_BUILD_STATIC OFF CACHE BOOL mimalloc FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL mimalloc FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL mimalloc FORCE)
if(WIN32)
    # Never patch the process CRT. Windows colour management mixes CRT and COM
    # allocation internally, which crashes in fullscreen D3D9 with the redirect.
    set(MI_OVERRIDE OFF CACHE BOOL mimalloc FORCE)
    set(MI_WIN_REDIRECT OFF CACHE BOOL mimalloc FORCE)
    # mi_new must throw std::bad_alloc/call the C++ new handler on exhaustion;
    # the C-only implementation aborts instead.
    set(MI_USE_CXX ON CACHE BOOL mimalloc FORCE)
else()
    # Preserve the existing ELF allocator interposition.
    set(MI_OVERRIDE ON CACHE BOOL mimalloc FORCE)
    set(MI_WIN_REDIRECT ON CACHE BOOL mimalloc FORCE)
endif()

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

include(${CMAKE_CURRENT_LIST_DIR}/get_all_targets.cmake)

get_all_targets(mimalloc_targets ${MIMALLOC_SOURCE_DIR})
foreach(target IN LISTS mimalloc_targets)
    set_target_properties(${target} PROPERTIES FOLDER third_party/mimalloc)
endforeach()

if(WIN32)
    include(${CMAKE_CURRENT_LIST_DIR}/project_allocator.cmake)
endif()
