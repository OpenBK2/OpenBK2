# Exercise the real 3D scene through a recording device, without opening an audio device.
add_unit_test(SceneVolume Sound Main System Misc zlib gr2)
set_tests_properties(SceneVolume_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(SceneVolume_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()
