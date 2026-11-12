# Exercise the real standard-library trace writer without loading game modules.
add_unit_test(MPPacketTraceLog SOURCES ../MPPacketTraceLog.cpp)
# Exercise host handoff and removal boundaries without loading game modules.
add_unit_test(MPPlayerDrop)
# Exercise the real game-constants snapshot with generated database records.
add_unit_test(MPDataChecksum GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
# Check data-backed online/LAN settings and compatibility with older XML and binary resources.
add_unit_test(MPConnectionSettings GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
target_compile_definitions(MPConnectionSettings_test PRIVATE
    MP_CONNECTION_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(MPConnectionSettings_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(MPConnectionSettings_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET MPConnectionSettings_test APPEND PROPERTY LINK_DEPENDS $<TARGET_FILE:GameX>)
endif()
add_unit_test(MPCoopRules GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
add_unit_test(MessageParts GameX UISpecificB2 UI libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
# Exercise persistent objective controls and the real Lua-to-client feedback queue without a graphics device.
# The texture fixture's dynamic_cast needs 3Dmotor's RTTI directly on the Linux link line.
add_unit_test(ObjectiveNotifications GameX UISpecificB2 UI 3Dmotor SceneB2 B2_M1_World Common_RTS_AI Script libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
target_compile_definitions(ObjectiveNotifications_test PRIVATE
    OBJECTIVE_NOTIFICATION_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
if(WIN32)
    set_tests_properties(ObjectiveNotifications_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET ObjectiveNotifications_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:GameX> $<TARGET_FILE:AILogic>)
endif()
target_compile_definitions(MessageParts_test PRIVATE
    MESSAGE_PARTS_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml"
    MESSAGE_PARTS_PAK="${CMAKE_SOURCE_DIR}/OBK2_Examples/MessageParts/messages.pak")
target_compile_definitions(MPCoopRules_test PRIVATE
    MP_COOP_RULES_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
add_unit_test(CoopScenario GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
target_compile_definitions(CoopScenario_test PRIVATE
    COOP_SCENARIO_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
add_unit_test(UnitTransferCommand GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
add_unit_test(ReplayPlayerDrop GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
add_unit_test(MPHostHandoff GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
# Exercise real pause packets, input recovery and allowance accounting without a map.
add_unit_test(MPPause GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2
    SOURCES ../MultiplayerCommandProcessor.cpp)
add_unit_test(ReplayCommandTiming GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
# Exercise real replay rows and click dispatch without a graphics device.
add_unit_test(ReplayPlayerSelection GameX UI libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
target_compile_definitions(ReplayPlayerSelection_test PRIVATE
    REPLAY_SELECTION_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
# Exercise the real host difficulty dropdown and submitted create-game message.
add_unit_test(MPDifficultySelection GameX UI libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
target_compile_definitions(MPDifficultySelection_test PRIVATE
    MP_DIFFICULTY_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
add_unit_test(UnitTransferUI UI 3Dmotor libdb System Misc gr2)
# Resolve the flags of an installed game's sides, when one is named (see the source).
add_unit_test(MPSideFlags GameX libdb System Misc Main AILogic Stats_B2_M1 zlib gr2)
target_compile_definitions(UnitTransferUI_test PRIVATE
    TRANSFER_UI_GAME_DATA="${CMAKE_SOURCE_DIR}/Versions/Current/Data"
    TRANSFER_UI_PAK="${CMAKE_SOURCE_DIR}/OBK2_Examples/UnitTransferImpl/unit_transfer.pak")
target_compile_definitions(MPDataChecksum_test PRIVATE
    MP_CHECKSUM_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
if(WIN32)
    set_tests_properties(MessageParts_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    # GameX loads UI's FFmpeg codecs, including DLLs outside CMake's link graph.
    set_tests_properties(MPDataChecksum_test MPCoopRules_test CoopScenario_test UnitTransferCommand_test ReplayPlayerDrop_test MPHostHandoff_test MPPause_test ReplayCommandTiming_test ReplayPlayerSelection_test MPDifficultySelection_test UnitTransferUI_test MPSideFlags_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET MPDataChecksum_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:GameX>)
    set_property(TARGET ReplayPlayerSelection_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:GameX> $<TARGET_FILE:UI>)
    set_property(TARGET MPDifficultySelection_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:GameX> $<TARGET_FILE:UI>)
endif()
