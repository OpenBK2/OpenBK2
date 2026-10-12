# Exercise the real deterministic trajectory and its save/load representation.
add_unit_test(BallisticOffset AILogic Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(BallisticOffset_test PROPERTIES TIMEOUT 60)
add_unit_test(TowingPrecision AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(TowingPrecision_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(BallisticOffset_test TowingPrecision_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()
