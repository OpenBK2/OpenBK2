# Exercise the generated resource, editor schema, reference and MP checksum together.
# Load the simulation registrations too: database and save/load types share one factory.
add_unit_test(ExtendedStats Stats_B2_M1 AILogic libdb System Misc zlib gr2)
set_tests_properties(ExtendedStats_test PROPERTIES TIMEOUT 60)
target_compile_definitions(ExtendedStats_test PRIVATE
    EXTENDED_STATS_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
if(WIN32)
    # UI's codec dependencies are outside the engine's CMake runtime DLL graph.
    set_tests_properties(ExtendedStats_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()
