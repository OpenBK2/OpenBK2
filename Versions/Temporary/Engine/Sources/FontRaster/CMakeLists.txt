project(FontRaster)

# Glyph rasterisation with FreeType, producing a font atlas and metrics in the
# conventions CFontFormatInfo has always used. FontGen bakes atlases with it;
# the game is meant to render text with it at run time later. See
# docs/font-rendering-blur.md.
#
# Static rather than the SHARED library with an export header that every engine
# subsystem is: it has one small interface and no engine dependencies, and a
# static library links into a tool and a DLL alike without an export macro on
# every declaration.
add_library(FontRaster STATIC
    CodePages.cpp
    CodePages.h
    CodePageTables.inc
    FontFace.cpp
    FontFace.h
    FontFinder.cpp
    FontFinder.h
    FontRaster.cpp
    FontRaster.h
)
target_include_directories(FontRaster PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# PRIVATE: no FreeType type appears in FontRaster.h
target_link_libraries(FontRaster PRIVATE freetype)
set_target_properties(FontRaster PROPERTIES POSITION_INDEPENDENT_CODE ON)
