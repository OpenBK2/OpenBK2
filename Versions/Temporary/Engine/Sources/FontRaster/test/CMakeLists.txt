add_unit_test(GposKerning FontRaster)
# the font the game ships for its headers, which keeps its kerning only in GPOS
target_compile_definitions(GposKerning_test PRIVATE
    OSWALD_TTF="${CMAKE_SOURCE_DIR}/Versions/Current/Data/Fonts/Files/Oswald-Variable.ttf")
