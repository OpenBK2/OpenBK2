project(FontGen)

# The font baker. It rasterises a character set with GDI into a glyph atlas,
# written as a TGA, and a binary blob of metrics, which is one CFontFormatInfo
# and the same class id the engine reads in 3Dmotor/FontFormat.h. The game does
# no glyph rasterisation of its own, so whatever this produces is what reaches
# the screen. See docs/font-rendering-blur.md.
#
# It had no CMake wiring at all, and the copy shipped in Tools/ is x86 only, so
# an x64 editor could not run it. ED_Common/FontExporter.cpp starts FontGen.exe
# by name, which means the editor's font export has been dead in this port.
#
# Two rasterisers. The original is CreateFont and TextOut into a DIB; -ft
# selects FontRaster, which is FreeType and bakes at an exact cell height. The
# tool stays Windows only while the GDI path and the code page translation it
# shares with -ft use Windows calls.
#
# A plain executable, not the SHARED library with an export header that every
# engine subsystem is: nothing links a tool, so it exports nothing.
if(NOT WIN32)
    return()
endif()

set(SOURCES
    FontGen.cpp
    FontFormat.cpp
    stdafx.cpp
)
add_executable(FontGen ${SOURCES})
add_stdafx_pch(FontGen)

# REGISTER_SAVELOAD_CLASS( FONTGEN, ... ) in FontFormat.cpp expands through
# BASIC_REGISTER_CLASS to FONTGEN_EXPORT, which would come from a generated
# export header. An executable has none and exports nothing, so the tag is
# defined away. Server does the same with SERVER_EXPORT.
target_compile_definitions(FontGen PRIVATE FONTGEN_EXPORT=)

# Image and Misc are header-only here: Targa.h, ImageConvertor.h and 2Darray.h
# are templates, which is why the original project linked neither.
target_link_libraries(FontGen PRIVATE Misc System zlib fmt::fmt-header-only FontRaster)
target_link_libraries(FontGen PRIVATE winapi)

install_runtime_dlls(FontGen)

install(TARGETS FontGen DESTINATION bin)

# -show-version reads REVISION_NUMBER_STR and BUILD_DATE_TIME_STR, which this
# defines. Without it the tool does not compile.
add_version_info(FontGen)
