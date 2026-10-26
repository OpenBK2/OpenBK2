# Exercise the real deterministic trajectory and its save/load representation.
add_unit_test(BallisticOffset AILogic Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(BallisticOffset_test PROPERTIES TIMEOUT 60)
add_unit_test(TowingPrecision AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
add_unit_test(TowingApproach AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(TowingApproach_test PRIVATE
    TOWING_APPROACH_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(TowingApproach_test PROPERTIES TIMEOUT 60)
add_unit_test(UnitTransfer System Misc zlib)
# Exercise the real unit selection refresh after recursive ownership changes.
add_unit_test(UnitTransferSelection AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
# Traverse the real registry and saved train/towing attachment graph.
add_unit_test(UnitTransferAttachments AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
add_unit_test(RepairTarget AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(RepairTarget_test PRIVATE
    REPAIR_TARGET_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(RepairTarget_test PROPERTIES TIMEOUT 60)
set_tests_properties(TowingPrecision_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(BallisticOffset_test TowingPrecision_test TowingApproach_test UnitTransferSelection_test UnitTransferAttachments_test RepairTarget_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(BoredAcknowledgments AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(BoredAcknowledgments_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(BoredAcknowledgments_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(ReinforcementSelection)

add_unit_test(RestStateMovement AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(RestStateMovement_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(RestStateMovement_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(ResupplyTarget AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(ResupplyTarget_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(ResupplyTarget_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(GroupMoverLifetime AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(GroupMoverLifetime_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(GroupMoverLifetime_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(QueueUnitIDs AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(QueueUnitIDs_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(QueueUnitIDs_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(BridgePaths AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(BridgePaths_test PRIVATE
    BRIDGE_PATHS_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(BridgePaths_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(BridgePaths_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(ScriptGroupWait AILogic Common_RTS_AI Stats_B2_M1 Script System Misc zlib gr2)
set_tests_properties(ScriptGroupWait_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(ScriptGroupWait_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()
