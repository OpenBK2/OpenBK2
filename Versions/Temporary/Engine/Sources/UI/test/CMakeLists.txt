# Exercise real scrollable UI controls without a graphics device or game data.
add_unit_test(ScrollableContent UI System Misc gr2)
if(WIN32)
    # UI loads FFmpeg's codecs, including DLLs outside CMake's link graph.
    set_tests_properties(ScrollableContent_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    # Refresh the test's copied DLL even when its import library is unchanged.
    set_property(TARGET ScrollableContent_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:UI>)
endif()
