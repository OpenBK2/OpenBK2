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
    NativeViewport.cpp
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
# NativeViewport.cpp out of the precompiled header. It is the one file here
# that includes X11's headers, whose None, Status, Bool and True are macros,
# and the prelude every other file shares has no business meeting them. It
# needs nothing from stdafx.h in return.
set_source_files_properties(NativeViewport.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
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
if(NOT WIN32)
    # NativeViewport.cpp, and only it, reaches past wx to the toolkit under it:
    # the viewport has to hand the renderer a window handle, and off Windows
    # that is an SDL_Window wrapping the X11 window of a GTK widget. See the
    # comment at the top of that file, and scripts/port/surfaceprobe.c, which
    # is the standalone proof that the chain works.
    #
    # wx::wx carries wx's own headers and the __WXGTK__ defines but not GTK's
    # include paths, so they are asked for here rather than added to every
    # target that uses wx.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
    pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
    target_link_libraries(ED_Common PRIVATE PkgConfig::GTK3 PkgConfig::X11 SDL3::SDL3-shared)
endif()
# No resource script: its one dialog belonged to CProgressBarWindow, which
# nothing ever created, and went with it. Version info comes from
# add_version_info below, not from a .rc of this module's own.
install(TARGETS ED_Common DESTINATION bin)
add_version_info(ED_Common)
