# Hold the river depth that terrain generation gives every tile vertex to the original
# algorithm, bit for bit, since those heights reach the simulation.
add_unit_test(RiverHeight B2_M1_Terrain Stats_B2_M1 libdb System Misc zlib gr2)

# Shared capture math, bundled corrections and editor/game camera metadata readers.
add_unit_test(UnitIconRendering SceneB2 3Dmotor Stats_B2_M1 libdb System Misc gr2 zlib)
target_compile_definitions(UnitIconRendering_test PRIVATE NOMINMAX
    ICON_CAMERA_SCHEMA="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
if(WIN32)
    set_property(TARGET UnitIconRendering_test APPEND PROPERTY LINK_DEPENDS
        $<TARGET_FILE:SceneB2> $<TARGET_FILE:libdb> $<TARGET_FILE:System>)
    set_tests_properties(UnitIconRendering_test PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:ffmpeg::avcodec>")
endif()
