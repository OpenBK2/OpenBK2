include(FetchContent)
include(GenerateExportHeader)

# libsquish (MIT) is the BC1/BC2/BC3 block encoder behind NImage's DDS writer.
#
# It stands in for the proprietary S3TC library the original texture pipeline
# linked, which was removed from this tree because it could not be shipped, and
# for the D3DX path that replaced it, which only ever existed on Windows. Of the
# surviving libsquish forks this is the only one carrying all three things this
# port needs:
#
#   * float* metric     the original S3TCencode call weighted colour error by
#                       { 0.309, 0.609, 0.082 }, and those weights carry over
#                       unchanged, so the encoder keeps the perceptual bias the
#                       shipped textures were compressed with
#   * kSourceBGRA       CArray2D<uint32_t> is already B,G,R,A in memory, so the
#                       image feeds straight in with no swizzle pass
#   * a pitch overload  a CArray2D can be handed over row by row
#
# It also exposes ComputeMSE and DecompressImage, which the writer's tests use
# to measure quality against the shipped corpus and to cross-check the engine's
# own GUnpackDXT decoder.
#
# Its own CMakeLists is deliberately not used. The top level calls
# fetch_ob_cmake(), a second configure-time FetchContent of oblivioncth/OBCMake,
# and lib/CMakeLists.txt is written entirely against that framework
# (ob_add_standard_library, ob_get_system_bitness). Adopting it would add a
# network dependency we do not control, force CMAKE_CXX_STANDARD to 98 on a
# C++17 tree, and turn on an OpenMP option that the sources never use. Pointing
# SOURCE_SUBDIR at a directory holding no CMakeLists.txt populates the sources
# without calling add_subdirectory on them, so the target is declared here
# instead.
FetchContent_Declare(
        squish
        GIT_REPOSITORY https://github.com/oblivioncth/libsquish.git
        GIT_TAG 8208355f3b7e3a0c2a3380d19520ae8d47423529 #refs/tags/v1.15.2
        GIT_PROGRESS TRUE
        SOURCE_SUBDIR lib/src
)

FetchContent_MakeAvailable(squish)

set(SQUISH_SRC_DIR ${squish_SOURCE_DIR}/lib/src)

add_library(squish STATIC
        ${SQUISH_SRC_DIR}/alpha.cpp
        ${SQUISH_SRC_DIR}/clusterfit.cpp
        ${SQUISH_SRC_DIR}/colourblock.cpp
        ${SQUISH_SRC_DIR}/colourfit.cpp
        ${SQUISH_SRC_DIR}/colourset.cpp
        ${SQUISH_SRC_DIR}/maths.cpp
        ${SQUISH_SRC_DIR}/rangefit.cpp
        ${SQUISH_SRC_DIR}/singlecolourfit.cpp
        ${SQUISH_SRC_DIR}/squish.cpp
)

# SQUISH_USE_SSE is left undefined on purpose, which selects the scalar
# simd_float.h path. The SSE and scalar fits do not reduce their float
# arithmetic in the same order, so they can disagree in the last bit and emit
# different endpoints for the same block. Exported textures are build artifacts
# that get compared and checked in, so x86, x64 and Linux producing identical
# bytes is worth more here than the encoder's speed: this runs once per texture
# from the editor, not per frame.

# squish.h includes "squish/squish_export.h", which the fork's own framework
# generates. Build it here instead, into the same build-tree directory the
# engine's export headers use, and keep the squish/ prefix the include names.
generate_export_header(squish
        BASE_NAME SQUISH
        EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/squish/squish_export.h)

target_include_directories(squish PUBLIC
        ${squish_SOURCE_DIR}/lib/include
        ${CMAKE_BINARY_DIR}/exports)

# The generated header resolves SQUISH_EXPORT to a dllimport unless it is told
# this is a static library, and CMake does not define squish_EXPORTS for one,
# so without this both squish and its consumers would import symbols nothing
# exports. PUBLIC because the macro is read in squish.h, at every call site.
target_compile_definitions(squish PUBLIC SQUISH_STATIC_DEFINE)

# A static library linked into Image, which is SHARED like every other module.
set_target_properties(squish PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(squish PROPERTIES FOLDER "third_party/Squish")
