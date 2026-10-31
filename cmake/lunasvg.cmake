include(FetchContent)

# lunasvg (MIT) rasterizes the SVG flags the UI draws, at the exact pixel size
# of the window showing them, so a flag stays sharp at any screen resolution.
# It was chosen over nanosvg, ThorVG and resvg: it covers what Wikimedia Commons
# flag and coat of arms files use (clipPath, masks, <use>, CSS), which nanosvg
# does not, and depends on nothing but plutovg, its own rasterizer, which ships
# inside the same repository, where ThorVG is larger and resvg would need a Rust
# toolchain in every build.
#
# The upstream CMakeLists files are not used. Both add install() rules, which
# would put lunasvg and plutovg headers, libraries and package files into the
# game's install prefix, and both probe for a system plutovg and for threads.
# SOURCE_SUBDIR points at a directory holding no CMakeLists.txt, which populates
# the sources without calling add_subdirectory on them, and the two static
# targets are declared here instead, as cmake/squish.cmake does for libsquish.
FetchContent_Declare(
        lunasvg
        GIT_REPOSITORY https://github.com/sammycage/lunasvg.git
        GIT_TAG 83c58df8103dc7dca423dfd824992af94d49bed6 #refs/tags/v3.5.0
        GIT_PROGRESS TRUE
        SOURCE_SUBDIR include
)

FetchContent_MakeAvailable(lunasvg)

set(PLUTOVG_SRC_DIR ${lunasvg_SOURCE_DIR}/plutovg)

add_library(plutovg STATIC
        ${PLUTOVG_SRC_DIR}/source/plutovg-blend.c
        ${PLUTOVG_SRC_DIR}/source/plutovg-canvas.c
        ${PLUTOVG_SRC_DIR}/source/plutovg-font.c
        ${PLUTOVG_SRC_DIR}/source/plutovg-matrix.c
        ${PLUTOVG_SRC_DIR}/source/plutovg-paint.c
        ${PLUTOVG_SRC_DIR}/source/plutovg-path.c
        ${PLUTOVG_SRC_DIR}/source/plutovg-rasterize.c
        ${PLUTOVG_SRC_DIR}/source/plutovg-surface.c
        ${PLUTOVG_SRC_DIR}/source/plutovg-ft-math.c
        ${PLUTOVG_SRC_DIR}/source/plutovg-ft-raster.c
        ${PLUTOVG_SRC_DIR}/source/plutovg-ft-stroker.c
)
set_target_properties(plutovg PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
target_include_directories(plutovg
        PRIVATE ${PLUTOVG_SRC_DIR}/source
        PUBLIC ${PLUTOVG_SRC_DIR}/include)
# PLUTOVG_BUILD_STATIC is read in plutovg.h, so every includer needs it.
target_compile_definitions(plutovg PRIVATE PLUTOVG_BUILD PUBLIC PLUTOVG_BUILD_STATIC)
# The font face cache would scan the system font directories. The flags carry
# no text, and the game must not depend on what fonts a machine has installed.
target_compile_definitions(plutovg PRIVATE PLUTOVG_DISABLE_FONT_FACE_CACHE_LOAD)
# HAVE_THREADS_H is left undefined on purpose. It only selects a C11 mutex for
# the font cache off Windows (Windows always uses a critical section), and
# without it the lock is a no-op, which is enough: the UI rasterizes on its own
# thread only. It also spares a configure-time probe for threads.h.
if(NOT MSVC)
    target_link_libraries(plutovg PRIVATE m)
endif()

add_library(lunasvg STATIC
        ${lunasvg_SOURCE_DIR}/source/lunasvg.cpp
        ${lunasvg_SOURCE_DIR}/source/graphics.cpp
        ${lunasvg_SOURCE_DIR}/source/svgelement.cpp
        ${lunasvg_SOURCE_DIR}/source/svggeometryelement.cpp
        ${lunasvg_SOURCE_DIR}/source/svglayoutstate.cpp
        ${lunasvg_SOURCE_DIR}/source/svgpaintelement.cpp
        ${lunasvg_SOURCE_DIR}/source/svgparser.cpp
        ${lunasvg_SOURCE_DIR}/source/svgproperty.cpp
        ${lunasvg_SOURCE_DIR}/source/svgrenderstate.cpp
        ${lunasvg_SOURCE_DIR}/source/svgtextelement.cpp
)
set_target_properties(lunasvg PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(lunasvg
        PRIVATE ${lunasvg_SOURCE_DIR}/source
        PUBLIC ${lunasvg_SOURCE_DIR}/include)
target_compile_definitions(lunasvg PRIVATE LUNASVG_BUILD PUBLIC LUNASVG_BUILD_STATIC)
# The same reason as PLUTOVG_DISABLE_FONT_FACE_CACHE_LOAD above.
target_compile_definitions(lunasvg PRIVATE LUNASVG_DISABLE_LOAD_SYSTEM_FONTS)
target_link_libraries(lunasvg PRIVATE plutovg)

# Static libraries linked into UI, which is SHARED like every other module.
set_target_properties(plutovg lunasvg PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(plutovg lunasvg PROPERTIES FOLDER "third_party/lunasvg")
