# Real map-object audio ownership, with a recording sound scene and no renderer/audio device.
add_unit_test(MovementSound B2_M1_World Sound Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(MovementSound_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(MovementSound_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET MovementSound_test APPEND PROPERTY LINK_DEPENDS $<TARGET_FILE:B2_M1_World>)
endif()
