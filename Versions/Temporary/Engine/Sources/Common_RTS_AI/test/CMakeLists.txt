# Bound malformed numeric/geometry inputs without stalling a simulation tick.
add_unit_test(HungarianMethod Common_RTS_AI System Misc)
add_unit_test(GroupGeometry AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(HungarianMethod_test GroupGeometry_test PROPERTIES TIMEOUT 30)
if(WIN32)
    set_tests_properties(GroupGeometry_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()
