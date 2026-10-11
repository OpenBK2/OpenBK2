# Exercise the real standard-library trace writer without loading game modules.
add_unit_test(MPPacketTraceLog SOURCES ../MPPacketTraceLog.cpp)
# Exercise the real game-constants snapshot with generated database records.
add_unit_test(MPDataChecksum GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
target_compile_definitions(MPDataChecksum_test PRIVATE
    MP_CHECKSUM_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
if(WIN32)
    # GameX loads UI's FFmpeg codecs, including DLLs outside CMake's link graph.
    set_tests_properties(MPDataChecksum_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET MPDataChecksum_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:GameX>)
endif()
