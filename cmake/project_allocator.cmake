# Windows module-local C++ allocation. Link one object directly into each
# participating binary, so it cannot be discarded as an unused archive member.
# All implementations call the same mimalloc DLL.
add_library(obk2_mimalloc_new_delete OBJECT
    ${CMAKE_CURRENT_LIST_DIR}/../Versions/Temporary/Engine/Sources/MemoryLib/MimallocNewDelete.cpp)
target_link_libraries(obk2_mimalloc_new_delete PRIVATE mimalloc)
target_compile_features(obk2_mimalloc_new_delete PRIVATE cxx_std_17)
target_compile_definitions(obk2_mimalloc_new_delete PRIVATE MI_SHARED_LIB)
if(MSVC)
    # mimalloc-new-delete.h uses __cplusplus to enable C++17 aligned overloads.
    target_compile_options(obk2_mimalloc_new_delete PRIVATE /Zc:__cplusplus)
endif()
set_target_properties(obk2_mimalloc_new_delete PROPERTIES FOLDER MemoryLib)

function(add_project_mimalloc target)
    get_target_property(kind ${target} TYPE)
    get_target_property(system_allocator ${target} OBK2_USE_SYSTEM_ALLOCATOR)
    if(system_allocator OR NOT kind MATCHES "^(EXECUTABLE|SHARED_LIBRARY|MODULE_LIBRARY)$")
        return()
    endif()
    # PRIVATE prevents overrides from leaking through public headers or onto
    # unrelated consumers. Static libraries use their final module's pair.
    target_sources(${target} PRIVATE $<TARGET_OBJECTS:obk2_mimalloc_new_delete>)
    if(ARGV1 STREQUAL "BUNDLED")
        # Dependencies such as Google Benchmark export their build targets.
        # A file link keeps our project targets out of their export sets.
        target_link_libraries(${target} PRIVATE "$<TARGET_LINKER_FILE:mimalloc>")
        add_dependencies(${target} mimalloc)
    else()
        # Preserve CMake's runtime DLL closure for our copy/install rules.
        target_link_libraries(${target} PRIVATE mimalloc)
    endif()
endfunction()

function(enable_project_mimalloc)
    get_all_targets(engine_targets ${CMAKE_SOURCE_DIR}/Versions/Temporary/Engine/Sources)
    foreach(target IN LISTS engine_targets)
        get_target_property(source_dir ${target} SOURCE_DIR)
        if(NOT source_dir MATCHES "/vendor/")
            add_project_mimalloc(${target})
        endif()
    endforeach()

    # These bundled C++ libraries exchange ownership of STL objects/callbacks
    # with our modules. Both sides of the ABI need the same new/delete pair.
    # wxWidgets is an ExternalProject; its equivalent hook is wx_mimalloc.
    foreach(target IN LISTS boost_targets googletest_targets googlebenchmark_targets)
        add_project_mimalloc(${target} BUNDLED)
    endforeach()
    foreach(target fastgltf fmt spdlog)
        if(TARGET ${target})
            add_project_mimalloc(${target} BUNDLED)
        endif()
    endforeach()
endfunction()
