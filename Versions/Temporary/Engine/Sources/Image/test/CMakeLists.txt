add_unit_test(ImageDDSWrite Image System Misc squish)

# ShippedHeadersMatch checks the writer's header against the real textures
# rather than against a restatement of the rule, so it needs to find them. The
# test skips itself if the directory is not there.
target_compile_definitions(ImageDDSWrite_test PRIVATE
    OBK2_DATA_DIR="${CMAKE_SOURCE_DIR}/Versions/Current/Data")
