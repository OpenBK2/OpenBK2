project(MapEditorLib)
set(SOURCES
    BasicDataExtractor.cpp
    BusyCursor.cpp
    BuildDataBuilder.cpp
    BuilderBase.cpp
    BuilderFactory.cpp
    ChildFrameFactory.cpp
    CommonEditorMethods.cpp
    CommonExporterMethods.cpp
    ConfigFile.cpp
    CursorsWx.cpp
    DefaultBuilder.cpp
    DefaultBuilderBase.cpp
    DefaultController.cpp
    DefaultExporter.cpp
    DefaultInputState.cpp
    DefaultView.cpp
    DialogState.cpp
    EditorBase.cpp
    EditorFactory.cpp
    ExporterBase.cpp
    ExporterFactory.cpp
    FolderController.cpp
    InteractiveMaya.cpp
    InteractiveProcess.cpp
    Logger.cpp
    ManipulatorManager.cpp
    MapEditorModule.cpp
    MaskManipulator.cpp
    MessageBoxesWx.cpp
    MultiInputState.cpp
    MultiManipulator.cpp
    ObjectBaseController.cpp
    ObjectController.cpp
    ObjectFilter.cpp
    PCIEMnemonics.cpp
    Progress.cpp
    Resources.cpp
    stdafx.cpp
    StoreInputState.cpp
    StringManager.cpp
    Tools_FreeIDCollector.cpp
    Tools_Geometry.cpp
    Tools_Gradient.cpp
    Tools_Image.cpp
    Tools_IndexCollector.cpp
    Tools_MainFolder.cpp
    Tools_Resources.cpp
    Tools_UniqueList.cpp
)
add_library(MapEditorLib SHARED ${SOURCES})
add_stdafx_pch(MapEditorLib)
add_export_header(MapEditorLib)
# No mfc: the image lists behind IImageList are Win32 ones
# (NativeImageList.h), and Tools_Image makes HBITMAPs rather than CBitmaps.
target_link_libraries(MapEditorLib PRIVATE Image libdb MemoryLib Misc System fmt::fmt-header-only)
# MessageBoxesWx.cpp and CursorsWx.cpp are the only translation units here that
# include a wx header, and the declarations they sit behind (MessageBoxes.h,
# Cursors.h) name no toolkit on purpose: a caller that includes wx loses
# windows.h's A/W macros and stops linking against NDb::GetObjectA. See
# cmake/wxwidgets.cmake for why there are no UNICODE defines.
target_link_libraries(MapEditorLib PRIVATE wx::wx)
add_dependencies(MapEditorLib wxwidgets_external)
if(WIN32)
    target_link_libraries(MapEditorLib PRIVATE winapi)
endif()
install(TARGETS MapEditorLib DESTINATION bin)
add_version_info(MapEditorLib)
