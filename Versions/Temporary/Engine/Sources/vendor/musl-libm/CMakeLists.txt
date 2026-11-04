# musl-libm: the transcendental functions the simulation needs, from a verbatim
# copy of musl's libm, built by each platform's own compiler. See README.md.

# The upstream files are listed once, in MANIFEST; the build compiles every .c
# in it, so an update that adds a file needs no edit here.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST" musl_libm_manifest REGEX "^src/.*\.c$")
list(TRANSFORM musl_libm_manifest PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST")

add_library(musl_libm STATIC ${musl_libm_manifest})
target_include_directories(musl_libm
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    # compat/ first, so libm.h finds the stand-in features.h, endian.h and
    # fp_arch.h ahead of the system's (which they hand on to where one exists).
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/compat" "${CMAKE_CURRENT_SOURCE_DIR}/src/internal")

set(musl_libm_prefix "${CMAKE_CURRENT_SOURCE_DIR}/compat/musl_libm_prefix.h")
if(MSVC)
    # /fp:precise is MSVC's default and is what FLT_EVAL_METHOD 0 needs; it is
    # stated so that a global /fp:fast cannot reach this target. /w because the
    # warnings are upstream's to fix, not ours.
    target_compile_options(musl_libm PRIVATE /FI${musl_libm_prefix} /fp:precise /w)
else()
    # -ffp-contract=off comes from cmake/arch.cmake already; it is repeated so
    # that the library is right on its own, whatever includes it.
    target_compile_options(musl_libm PRIVATE -include ${musl_libm_prefix} -ffp-contract=off -w)
endif()

set_target_properties(musl_libm PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    # Linked into the engine's shared libraries, so it has to be relocatable,
    # and each one keeps its own private copy rather than exporting it.
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden
    FOLDER "third_party/musl-libm")

# Every symbol the library defines must carry the musl_ prefix, and it must call
# none of the platform's own transcendental functions. A name that update.sh
# brought in and compat/musl_libm_prefix.h does not rename fails here rather
# than colliding with glibc or the CRT, or being quietly resolved to them.
if(MSVC)
    get_filename_component(musl_libm_linker_dir "${CMAKE_LINKER}" DIRECTORY)
    find_program(MUSL_LIBM_DUMPBIN dumpbin HINTS "${musl_libm_linker_dir}")
    set(musl_libm_symbol_tool "${MUSL_LIBM_DUMPBIN}")
    set(musl_libm_symbol_kind dumpbin)
else()
    set(musl_libm_symbol_tool "${CMAKE_NM}")
    set(musl_libm_symbol_kind nm)
endif()
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(musl_libm_strip_underscore ON)
else()
    set(musl_libm_strip_underscore OFF)
endif()
if(musl_libm_symbol_tool)
    add_test(NAME musl_libm_symbols
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=${musl_libm_symbol_tool}
            -DKIND=${musl_libm_symbol_kind}
            -DLIB=$<TARGET_FILE:musl_libm>
            -DSTRIP_UNDERSCORE=${musl_libm_strip_underscore}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/check-symbols.cmake")
    set_tests_properties(musl_libm_symbols PROPERTIES LABELS "obk2-test")
endif()

# The agreement test, the results against recorded hashes, is Misc/test/MuslLibm.cpp.
