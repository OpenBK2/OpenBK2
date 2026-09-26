add_unit_test(GposKerning FontRaster)
# the font the game ships for its headers, which keeps its kerning only in GPOS
target_compile_definitions(GposKerning_test PRIVATE
    OSWALD_TTF="${CMAKE_SOURCE_DIR}/Versions/Current/Data/Fonts/Files/Oswald-Variable.ttf")

# The replacement fonts must retain the baked fonts' visible size and the UI's
# horizontal scale, without fitting their accents by shrinking every letter.
add_unit_test(FontSizing FontRaster)
target_compile_definitions(FontSizing_test PRIVATE
    FONT_DATA_DIR="${CMAKE_SOURCE_DIR}/Versions/Current/Data/Fonts/Files")
