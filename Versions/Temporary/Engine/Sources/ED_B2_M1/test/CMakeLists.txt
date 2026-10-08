# Exercise the real builder and database without opening editor windows.
add_unit_test(VisObjBuilder ED_Common MapEditorLib 3Dmotor Stats_B2_M1 libdb System Misc gr2 zlib fastgltf::fastgltf
    SOURCES ../VisObjBuilder.cpp ../SeasonMnemonics.cpp
    ../../MapEditor/FolderCallback.cpp ../../MapEditor/ViewContainer.cpp
    ../../MapEditor/CommandHandlerContainer.cpp)
target_compile_definitions(VisObjBuilder_test PRIVATE NOMINMAX
    VISOBJ_SCHEMA="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml"
    VISOBJ_MODEL="${CMAKE_SOURCE_DIR}/OBK2_Examples/GLB_format_usage/ExampleTankUnit/model.glb")
copy_wx_runtime(VisObjBuilder_test)
if(WIN32)
    set_tests_properties(VisObjBuilder_test PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:ffmpeg::avcodec>")
endif()

# Exercise the DLL's metadata importer against a real editor database, without dialogs.
add_unit_test(SkeletonExporter ED_B2_M1 ED_Common MapEditorLib 3Dmotor Stats_B2_M1 libdb System Misc gr2 zlib fastgltf::fastgltf
    SOURCES ../../MapEditor/FolderCallback.cpp ../../MapEditor/ViewContainer.cpp
    ../../MapEditor/CommandHandlerContainer.cpp)
target_compile_definitions(SkeletonExporter_test PRIVATE NOMINMAX
    SKELETON_SCHEMA="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
copy_wx_runtime(SkeletonExporter_test)
if(WIN32)
    set_property(TARGET SkeletonExporter_test APPEND PROPERTY LINK_DEPENDS $<TARGET_FILE:ED_B2_M1>)
    set_tests_properties(SkeletonExporter_test PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:ffmpeg::avcodec>")
endif()
