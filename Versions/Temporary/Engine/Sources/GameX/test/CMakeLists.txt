# Exercise the real standard-library trace writer without loading game modules.
add_unit_test(MPPacketTraceLog SOURCES ../MPPacketTraceLog.cpp)
# Exercise the real game-constants snapshot with generated database records.
add_unit_test(MPDataChecksum GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
add_unit_test(UnitTransferCommand GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
add_unit_test(ReplayPlayerDrop GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
add_unit_test(ReplayCommandTiming GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
add_unit_test(UnitTransferUI UI 3Dmotor libdb System Misc gr2)
# Resolve the flags of an installed game's sides, when one is named (see the source).
add_unit_test(MPSideFlags GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
target_compile_definitions(UnitTransferUI_test PRIVATE
    TRANSFER_UI_GAME_DATA="${CMAKE_SOURCE_DIR}/Versions/Current/Data"
    TRANSFER_UI_PAK="${CMAKE_SOURCE_DIR}/OBK2_Examples/UnitTransferImpl/unit_transfer.pak")
target_compile_definitions(MPDataChecksum_test PRIVATE
    MP_CHECKSUM_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
if(WIN32)
    # GameX loads UI's FFmpeg codecs, including DLLs outside CMake's link graph.
    set_tests_properties(MPDataChecksum_test UnitTransferCommand_test ReplayPlayerDrop_test ReplayCommandTiming_test UnitTransferUI_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET MPDataChecksum_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:GameX>)
endif()
