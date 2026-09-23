project(MapEditorLib)
set(SOURCES
    BasicDataExtractor.cpp
    BusyCursor.cpp
    BuildDataBuilder.cpp
    BuildDetailsWx.cpp
    BuilderBase.cpp
    BuilderFactory.cpp
    ChildFrameFactory.cpp
    ClipboardWx.cpp
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
    FocusMemoryWx.cpp
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
# ClipboardWx.cpp, CursorsWx.cpp, FocusMemoryWx.cpp and MessageBoxesWx.cpp are
# the only translation units here that include a wx header, and the declarations
# they sit behind (Clipboard.h, Cursors.h, FocusMemory.h, MessageBoxes.h) name
# no toolkit on purpose: a caller
# that includes wx loses windows.h's A/W macros and stops linking against
# NDb::GetObjectA. See cmake/wxwidgets.cmake for why there are no UNICODE
# defines.
target_link_libraries(MapEditorLib PRIVATE wx::wx)
add_dependencies(MapEditorLib wxwidgets_external)
if(WIN32)
    target_link_libraries(MapEditorLib PRIVATE winapi)
endif()
# BuildDetailsWx.cpp names the Granny version the editor runs on, which means
# calling GrannyGetVersionString and evaluating the GrannyVersionsMatch
# predicate: both live in libgr2, so the version report needs it linked.
target_link_libraries(MapEditorLib PRIVATE granny::granny)
# The glTF reader's version. fastgltf publishes none in its headers, so
# cmake/fastgltf.cmake passes the tag it pins. On the target and not on
# BuildDetailsWx.cpp alone: this library is built with a precompiled header, and
# MSVC rejects a translation unit whose preprocessor state differs from the one
# the PCH was created with.
if(DEFINED FASTGLTF_VERSION)
    target_compile_definitions(MapEditorLib PRIVATE FASTGLTF_VERSION_STR="${FASTGLTF_VERSION}")
endif()
install(TARGETS MapEditorLib DESTINATION bin)
add_version_info(MapEditorLib)
# The build details block carries the revision, for bug reports
# (BuildDetailsWx.cpp).
add_git_revision(MapEditorLib)

add_subdirectory(test)
