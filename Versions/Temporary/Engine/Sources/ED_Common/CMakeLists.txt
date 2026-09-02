project(ED_Common)
set(SOURCES
    AIGeometryExporter.cpp
    BaseUIBuilder.cpp
    BasicExporter.cpp
    BasicSceneExporter.cpp
    ChildFrameBase.cpp
    ChildFrameWndBase.cpp
    dbbulder.cpp
    DefaultUIBuilder.cpp
    FontExporter.cpp
    GameInputInterface.cpp
    Initialize.cpp
    ParticleExporter.cpp
    ProgressBar.cpp
    ProgressBarWindow.cpp
    RunModeInterfaceBase.cpp
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
target_link_libraries(ED_Common PRIVATE 3DLib 3Dmotor Image Input libdb Main MapEditorLib MemoryLib Misc stingray System fmt::fmt-header-only granny::granny)
if(WIN32)
    target_link_libraries(ED_Common PRIVATE winapi)
endif()
install(TARGETS ED_Common DESTINATION bin)
add_version_info(ED_Common)
