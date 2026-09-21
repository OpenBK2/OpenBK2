# Granny comes from libgr2, this tree's own replacement for RAD Game Tools'
# proprietary granny2.dll, rather than from the DLL vendored under
# third_party/uesp-esoapps. That DLL is Windows only and non-redistributable,
# and it was the last thing keeping the engine off other platforms.
#
# The engine now also *compiles* against libgr2 rather than against RAD's
# header. granny211.h used to be on the include path here and used to be what
# every translation unit included, by way of the one-line redirect at
# Versions/Temporary/Engine/Sources/vendor/granny/include/granny.h; that
# redirect now points at vendor/libgr2/include/gr2/granny.h, which declares the
# same ABI: the same entry points with the same signatures and decoration, the
# same record layouts with the same member names, and the same version macros.
# src/Structures.h asserts its own copy of those layouts against the public ones
# in both directions, so the two cannot drift.
#
# Nothing here puts granny211.h on the include path any more, which is what
# makes that claim testable rather than a comment: a translation unit that still
# wanted RAD's header would now fail to compile.
#
# On Windows the header declares the entry points __declspec(dllimport) and
# __stdcall, and libgr2 exports them __declspec(dllexport) and __stdcall under
# the same file name the engine loads by, granny2.dll on x86 and granny2_x64.dll
# on x64. Off Windows both sides are plain C linkage with default visibility.
#
# What third_party/uesp-esoapps is still needed for is the verification harness,
# not the build: scripts/port/gr2diff.py, gr2info.py and granny_dll_oracle.py
# drive the real granny2.dll out of that submodule as the reference every claim
# about libgr2 was measured against. See docs/GrannyReplacement.md.

# libgr2 is a standalone project, so it configures and builds on its own as well
# as from here. As a subdirectory its tests stay off and it reuses this tree's
# spdlog rather than fetching another, which is why this file is included after
# cmake/spdlog.cmake rather than before it.
add_subdirectory(
    "${CMAKE_SOURCE_DIR}/Versions/Temporary/Engine/Sources/vendor/libgr2"
    "${CMAKE_BINARY_DIR}/vendor/libgr2")

# gr2 carries its own include directory as a PUBLIC usage requirement, so
# linking it is all a consumer needs for <gr2/granny.h> to resolve.
add_library(granny INTERFACE)
target_link_libraries(granny INTERFACE gr2)

# Beside the executable, under both names, since the engine loads it by name.
install(TARGETS gr2 RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

add_library(granny::granny ALIAS granny)
