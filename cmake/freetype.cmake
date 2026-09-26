include(FetchContent)

# FreeType is the glyph rasteriser behind FontRaster, which bakes the game's
# font atlases in FontGen and is meant to render them at run time later. See
# docs/font-rendering-blur.md.
#
# It replaces GDI for two reasons. It is portable, where CreateFont and TextOut
# are not. And it takes a fractional em size, so a font can be rasterised at
# exactly the cell height the UI asks for: GDI rounds to a whole-pixel em and so
# cannot produce every height, Impact giving 76 or 80 but never 79.
#
# FreeType computes in fixed point, so the same version and font file produce
# the same bitmap bytes on every platform, which keeps baked atlases
# reproducible and comparable across x86, x64 and Linux.
#
# Licensed under the FreeType License (BSD style with a credit clause) or
# GPLv2, at the user's choice. The credit clause applies once the game itself
# ships FreeType; a build tool using it has nothing to attribute.
FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG 0a0221a1347e2f1e07c395263540026e9a0aa7c7 #refs/tags/VER-2-14-3
        GIT_PROGRESS TRUE
)

# Every optional dependency off. zlib, bzip2 and Brotli decompress WOFF and
# gzipped PCF fonts, PNG decodes colour bitmap glyphs and HarfBuzz improves the
# auto-hinter's script coverage; the game's fonts are plain TrueType and need
# none of them. Left on, FreeType's CMake would go looking for whatever the
# build machine happens to have installed, which is how two machines end up
# rasterising differently.
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
# Nothing of FreeType's is installed: it is linked statically into FontRaster.
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)

# Static regardless of what the rest of the tree builds, since FreeType's CMake
# follows BUILD_SHARED_LIBS and a DLL of it would have to be shipped and
# located. Restored afterwards so nothing else is affected.
set(_freetype_saved_shared_libs ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(freetype)
set(BUILD_SHARED_LIBS ${_freetype_saved_shared_libs})

# Linked into FontRaster, which is itself linked into shared libraries later.
set_target_properties(freetype PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(freetype PROPERTIES FOLDER "third_party/FreeType")
