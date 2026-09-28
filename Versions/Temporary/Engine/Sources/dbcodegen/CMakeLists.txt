project(dbcodegen)

# The database code generator. It compiles the .cll type descriptions into
# types.xml and into the DB*.h/.cpp sources that carry the structs, their
# serializers, meta info and checksums. The compiler and the code writer live in
# libdb (CompileCLike*, CodeGen*) and the .cll grammar in Parser; this is only
# the driver around them.
#
# The .cll files it compiles are the ones the modules list in their SOURCES.
# The build gathers them into type-descriptions.txt beside the executable (see
# cmake/dbcodegen.cmake), and dbcodegen reads that with --file-list. Run it
# from the root of the sources, or pass --sources-path.
#
# Built but not installed, and run after a .cll change through the
# regenerate-db target (cmake/dbcodegen.cmake), never by the build itself.
# -all rewrites the generated sources and types.xml, each only if its content
# changed; types.xml goes to --types-path, which is Versions/Current/Data. The
# generated files in the tree are exactly its output, so a regeneration diffs
# to nothing but the .cll change. -nocopy writes to the temp directory only.
set(SOURCES
    codegen.cpp
    main.cpp
    stdafx.cpp
)
add_executable(dbcodegen ${SOURCES})
add_stdafx_pch(dbcodegen)

# Boost::program_options replaced System/CmdLine.h, which the port deleted as
# unused by the game.
target_link_libraries(dbcodegen PRIVATE libdb Parser MemoryLib Misc System zlib
                                        Boost::program_options fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(dbcodegen PRIVATE winapi)
endif()

# Run from the source tree it generates into rather than from bin/, so the
# DLLs have to sit beside it in the build tree.
copy_runtime_dlls(dbcodegen)

# -show-version reads REVISION_NUMBER_STR and BUILD_DATE_TIME_STR, which this
# defines. Without it the tool does not compile.
add_version_info(dbcodegen)
