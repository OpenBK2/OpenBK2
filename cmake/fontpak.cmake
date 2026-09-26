# obk2_fonts.pak: the game's open fonts and the font records that use them, as
# a patch archive that works over any existing install of the game.
#
# The game renders a font itself, at the exact size the UI asks for, when its
# Font.xdb record names a FontFile (3Dmotor/GRuntimeFont.h). A retail or GOG
# install carries its font records in texts.pak, which predate that field, so
# new binaries over an old install still draw the old blurry baked fonts. This
# archive supplies the three open font files and the four records with
# FontFile set. Dropped into Data beside the retail paks it takes over,
# because the VFS lets the entry with the newer timestamp inside its archive
# win. In the Russian retail install examined, with its 2017 patches, the only
# font records were texts.pak's, dated 2005.
#
# One archive serves every language edition. The records' charset differs by
# edition, but a runtime font does not use it: glyphs are looked up by code
# point and every font's cell is fitted to the same characters. The records
# also point at the baked texture and metrics a font falls back to if its file
# cannot be used, by paths and ids that come from the one game database and so
# should be the same in every edition; not yet checked against one other than
# Russian.
#
# Built with cmake -E tar, so it needs nothing beyond CMake. Every entry gets
# the same fixed modification time, which is all the VFS compares, so which
# archive wins does not depend on when this one was built; bump it only if a
# later archive must override this one. The bytes are not reproducible even
# so: libarchive also stores each file's access and change times in an extra
# field that cmake -E tar gives no way to leave out.
set(OBK2_FONT_PAK "${CMAKE_BINARY_DIR}/obk2_fonts.pak")
set(OBK2_FONT_PAK_TIME "2026-09-26 00:00:00")
set(OBK2_FONT_DATA "${CMAKE_SOURCE_DIR}/Versions/Current/Data")
# Listed rather than globbed, as every source list in this tree is
set(OBK2_FONT_PAK_FILES
    Fonts/Files/PTSans-Regular.ttf
    Fonts/Files/PTSans-OFL.txt
    Fonts/Files/Oswald-Variable.ttf
    Fonts/Files/Oswald-OFL.txt
    Fonts/Files/LiberationSans-Regular.ttf
    Fonts/Files/LiberationSans-OFL.txt
    Fonts/Body/Font.xdb
    Fonts/Header1/Font.xdb
    Fonts/Header2/Font.xdb
    Fonts/Numeric/Font.xdb
)
list(TRANSFORM OBK2_FONT_PAK_FILES PREPEND "${OBK2_FONT_DATA}/" OUTPUT_VARIABLE OBK2_FONT_PAK_DEPENDS)

# A checkout without the game data, a sparse one as CI's used to be, cannot
# make the archive. Better a build without it, said out loud, than no build:
# the engine itself does not need it, and draws baked fonts where it is absent.
foreach(OBK2_FONT_PAK_INPUT IN LISTS OBK2_FONT_PAK_DEPENDS)
    if(NOT EXISTS "${OBK2_FONT_PAK_INPUT}")
        message(WARNING "obk2_fonts.pak is not built: ${OBK2_FONT_PAK_INPUT} is missing from this checkout")
        return()
    endif()
endforeach()

# Paths inside the archive are relative to Data, which is what the VFS looks
# them up by, hence the working directory.
add_custom_command(
    OUTPUT "${OBK2_FONT_PAK}"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${OBK2_FONT_PAK}"
    COMMAND ${CMAKE_COMMAND} -E tar cf "${OBK2_FONT_PAK}" --format=zip "--mtime=${OBK2_FONT_PAK_TIME}" -- ${OBK2_FONT_PAK_FILES}
    WORKING_DIRECTORY "${OBK2_FONT_DATA}"
    DEPENDS ${OBK2_FONT_PAK_DEPENDS}
    COMMENT "Packing the open fonts and their font records into obk2_fonts.pak"
    VERBATIM)
add_custom_target(fonts_pak ALL DEPENDS "${OBK2_FONT_PAK}")
set_target_properties(fonts_pak PROPERTIES FOLDER "data")

install(FILES "${OBK2_FONT_PAK}" DESTINATION Data)
