# Generates the <target>_export.h header that carries a module's <MODULE>_EXPORT
# macro, and puts it on the include path of every target in the build.
#
# The headers are written into one directory in the *build* tree, never into the
# source tree. Several build trees (CLion profiles, the x86/x64 presets) are
# configured against the same sources, sometimes in parallel, and CMake offers
# no locking around configure_file: two configures writing one shared output
# path collide on its temporary file, and the loser aborts with
# "No such file or directory".
#
# One shared directory rather than one per module, because the includes are
# bare file names ("3Dmotor_export.h") and they appear in module *headers*,
# which other modules include in turn. Such an include is resolved in the
# consuming target, so a per-module include directory would only work for
# targets that link that module; the flat directory below works for all of them,
# the way sitting next to the including header used to.
include(GenerateExportHeader)

set(EXPORT_HEADER_DIR ${CMAKE_BINARY_DIR}/exports)

# Included from the top level before any add_subdirectory(), so this reaches
# every target, tests and benchmarks included.
include_directories(${EXPORT_HEADER_DIR})

function(add_export_header target)
    generate_export_header(${target}
        EXPORT_FILE_NAME ${EXPORT_HEADER_DIR}/${target}_export.h)

    # Older configurations generated this header beside the module's sources.
    # Quoted includes find that stale copy before the build-tree include path;
    # an empty export macro then leaves Windows DLLs without an import .lib.
    # Remove only this module's obsolete generated header after generating its
    # replacement, so existing checkouts migrate to the build-tree headers.
    set(legacy_export_header "${CMAKE_CURRENT_SOURCE_DIR}/${target}_export.h")
    if(EXISTS "${legacy_export_header}")
        file(REMOVE "${legacy_export_header}")
    endif()
endfunction()
