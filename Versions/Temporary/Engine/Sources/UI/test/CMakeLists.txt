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

# Restore saved buttons and descendants without graphics resources or game data.
# The fixture emits inline input observers and CDBPtr lazy loads from CWindow.
# ELF linkers need their libraries directly, beyond UI's private dependencies.
add_unit_test(WindowAfterLoad UI Input libdb System Misc gr2)
if(WIN32)
    set_tests_properties(WindowAfterLoad_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET WindowAfterLoad_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:UI>)
endif()

# Reproduce the selected unit HUD's numeric labels with the shipped font.
add_unit_test(TextVerticalAlignment UI 3Dmotor FontRaster Image System Misc gr2)
target_compile_definitions(TextVerticalAlignment_test PRIVATE
    GAME_DATA_DIR="${CMAKE_SOURCE_DIR}/Versions/Current/Data")
if(WIN32)
    set_tests_properties(TextVerticalAlignment_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET TextVerticalAlignment_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:UI> $<TARGET_FILE:3Dmotor>)
endif()

# Send Unicode window input through the real edit control in the C locale.
add_unit_test(UnicodeInput UI Input libdb 3Dmotor System Misc gr2)
if(NOT WIN32)
    # port/virtualkey.h includes SDL3; UI and Input keep that dependency private.
    target_link_libraries(UnicodeInput_test PRIVATE SDL3::SDL3-shared)
endif()
target_compile_definitions(UnicodeInput_test PRIVATE
    GAME_DATA_DIR="${CMAKE_SOURCE_DIR}/Versions/Current/Data")
if(WIN32)
    set_tests_properties(UnicodeInput_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET UnicodeInput_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:UI> $<TARGET_FILE:Input> $<TARGET_FILE:System>)
endif()
