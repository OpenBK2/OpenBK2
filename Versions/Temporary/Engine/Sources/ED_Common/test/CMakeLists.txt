# Exercise the conversion without loading the editor or a graphics device.
add_unit_test(GrannyGltf gr2 fastgltf::fastgltf SOURCES ../GrannyGltf.cpp)
target_compile_definitions(GrannyGltf_test PRIVATE NOMINMAX)
target_compile_definitions(GrannyGltf_test PRIVATE
    OBK2_DATA_DIR="${CMAKE_SOURCE_DIR}/Versions/Current/Data")

add_unit_test(GrannyGltfTexture wx::wx squish SOURCES ../GrannyGltfTexture.cpp)
target_compile_definitions(GrannyGltfTexture_test PRIVATE NOMINMAX)
add_dependencies(GrannyGltfTexture_test wxwidgets_external)
copy_wx_runtime(GrannyGltfTexture_test)

# Complete XDB -> VFS -> GR2/DDS -> GLB path, without starting the editor UI.
add_unit_test(GrannyModelExport ED_Common 3Dmotor Stats_B2_M1 libdb System Misc gr2 zlib fastgltf::fastgltf)
target_compile_definitions(GrannyModelExport_test PRIVATE NOMINMAX
    OBK2_DATA_DIR="${CMAKE_SOURCE_DIR}/Versions/Current/Data"
    GRANNY_EXPORT_OUTPUT_DIR="${CMAKE_BINARY_DIR}/granny-export-validation")
copy_wx_runtime(GrannyModelExport_test)
if(WIN32)
    set_property(TARGET GrannyModelExport_test APPEND PROPERTY LINK_DEPENDS $<TARGET_FILE:ED_Common>)
    # ED_Common loads UI, whose FFmpeg DLLs have additional codec dependencies
    # outside CMake's target graph. Let the test find the bundled codec DLLs.
    set_tests_properties(GrannyModelExport_test PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:ffmpeg::avcodec>")
endif()
