# Granny comes from libgr2 now, this tree's own replacement for RAD Game Tools'
# proprietary granny2.dll, rather than from the DLL vendored under
# third_party/uesp-esoapps. That DLL is Windows only and non-redistributable,
# and it was the last thing keeping the engine off other platforms.
#
# granny211.h stays on the include path and stays the header the engine compiles
# against. It is the ABI libgr2 reproduces: the layouts in
# vendor/libgr2/src/Structures.h are asserted against the sizes it declares, and
# every entry point matches its signature and its decoration. So nothing on the
# engine side changes; only what answers the calls does.
#
# On Windows granny211.h declares the entry points __declspec(dllimport) and
# __stdcall, and libgr2 exports them __declspec(dllexport) and __stdcall under
# the same file name the engine loads by, granny2.dll on x86 and granny2_x64.dll
# on x64. Off Windows both sides are plain C linkage with default visibility.
#
# The old vendored DLL and the stub that stood in for it off Windows are gone.
# third_party/uesp-esoapps is still a submodule because granny211.h lives in it;
# see docs/GrannyReplacement.md for what remains before it can be dropped too.

set(GRANNY_ROOT "${CMAKE_SOURCE_DIR}/third_party/uesp-esoapps/common/granny")

# libgr2 is a standalone project, so it configures and builds on its own as well
# as from here. As a subdirectory its tests stay off and it reuses this tree's
# spdlog rather than fetching another, which is why this file is included after
# cmake/spdlog.cmake rather than before it.
add_subdirectory(
    "${CMAKE_SOURCE_DIR}/Versions/Temporary/Engine/Sources/vendor/libgr2"
    "${CMAKE_BINARY_DIR}/vendor/libgr2")

add_library(granny INTERFACE)
target_include_directories(granny INTERFACE ${GRANNY_ROOT})
target_link_libraries(granny INTERFACE gr2)

# Beside the executable, under both names, since the engine loads it by name.
install(TARGETS gr2 RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

add_library(granny::granny ALIAS granny)
