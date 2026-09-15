project(ED_Common)
set(SOURCES
    AIGeometryExporter.cpp
    BaseUIBuilder.cpp
    BasicExporter.cpp
    BasicSceneExporter.cpp
    GltfExporter.cpp
    ChildFrameBase.cpp
    ChildFrameWndBase.cpp
    dbbulder.cpp
    DefaultUIBuilder.cpp
    FontExporter.cpp
    GameInputInterface.cpp
    Initialize.cpp
    ParticleExporter.cpp
    RunModeInterfaceBase.cpp
    SceneSurfaceMfc.cpp
    SceneSurfaceWx.cpp
    stdafx.cpp
    StoreObjectExporter.cpp
    TempAttributesTool.cpp
    TextureEditor.cpp
    TextureExporter.cpp
    TextureInterface.cpp
    TextureState.cpp
    Tools_Granny.cpp
    UIRunModeInterface.cpp
    UIRunModeState.cpp
    UIWindow.cpp
    WindowMSButtonSharedBuilder.cpp
    WindowScreenSharedEditor.cpp
    WindowSimpleSharedBuilder.cpp
    WindowSimpleSharedEditor.cpp
    WindowSimpleSharedState.cpp
)
add_library(ED_Common SHARED ${SOURCES})
add_stdafx_pch(ED_Common)
add_export_header(ED_Common)
# mfc and not stingray: nothing in this module uses Stingray any more, but it
# does use MFC, and linking mfc is what keeps it on the shared MFC DLL.
target_link_libraries(ED_Common PRIVATE 3DLib 3Dmotor Image Input libdb Main MapEditorLib MemoryLib Misc mfc System fmt::fmt-header-only granny::granny fastgltf::fastgltf)
if(WIN32)
    target_link_libraries(ED_Common PRIVATE winapi)
endif()
# The viewport's wx window. SceneSurfaceWx.cpp compiles to nothing without
# OBK2_WITH_WX, so it is listed unconditionally. See cmake/wxwidgets.cmake for
# why there are no UNICODE defines.
if(BUILD_WX_EDITOR)
    target_compile_definitions(ED_Common PRIVATE OBK2_WITH_WX)
    target_link_libraries(ED_Common PRIVATE wx::wx)
    add_dependencies(ED_Common wxwidgets_external)
endif()
# No resource script: its one dialog belonged to CProgressBarWindow, which
# nothing ever created, and went with it. Version info comes from
# add_version_info below, not from a .rc of this module's own.
install(TARGETS ED_Common DESTINATION bin)
add_version_info(ED_Common)
