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
# No mfc: the viewport's CWinApp::OnIdle call was the last use.
target_link_libraries(ED_Common PRIVATE 3DLib 3Dmotor Image Input libdb Main MapEditorLib MemoryLib Misc System fmt::fmt-header-only granny::granny fastgltf::fastgltf)
if(WIN32)
    target_link_libraries(ED_Common PRIVATE winapi)
endif()
# The viewport's window is wx. See cmake/wxwidgets.cmake for why there are no
# UNICODE defines.
target_link_libraries(ED_Common PRIVATE wx::wx)
add_dependencies(ED_Common wxwidgets_external)
# No resource script: its one dialog belonged to CProgressBarWindow, which
# nothing ever created, and went with it. Version info comes from
# add_version_info below, not from a .rc of this module's own.
install(TARGETS ED_Common DESTINATION bin)
add_version_info(ED_Common)
