project(MapEditor)
set(SOURCES
    AboutViewWx.cpp
    BitFieldView.cpp
    BitFieldViewWx.cpp
    BuilderContainer.cpp
    ChildFrameContainer.cpp
    ComboBox_GDBBrowser.cpp
    CommandHandlerContainer.cpp
    ControllerContainer.cpp
    CreateModView.cpp
    CreateModViewWx.cpp
    EditorContainer.cpp
    Empty_GDBBrowser.cpp
    ExporterContainer.cpp
    FindTextDlg.cpp
    FolderCallback.cpp
    GDBBrowserPane.cpp
    LogPane.cpp
    LogViewWx.cpp
    LuaEditor.cpp
    MainFrameShared.cpp
    MainFrameToolBars.cpp
    MainFrameWx.cpp
    MainFrameWxPanes.cpp
    MainFrameContainer.cpp
    MainFrameParams.cpp
    MapEditorApp.cpp
    MapEditorSingleton.cpp
    MenuDropDownView.cpp
    MenuDropDownViewWx.cpp
    MODContainer.cpp
    NewObjectDialog.cpp
    NewObjectView.cpp
    NewObjectViewWx.cpp
    ObjectCollector.cpp
    OpenModViewWx.cpp
    PC_BaseDialog.cpp
    PC_BinaryBitFieldEditor.cpp
    PC_BoolCheckBoxEditor.cpp
    PC_BoolComboEditor.cpp
    PC_BoolSwitcherEditor.cpp
    PC_DBLinkDialog.cpp
    PC_Dialog.cpp
    PC_EditorButton.cpp
    PC_EditorSlider.cpp
    PC_ExTextFileEditor.cpp
    PC_FloatComboEditor.cpp
    PC_FloatInputEditor.cpp
    PC_FloatSliderEditor.cpp
    PC_GUIDEditor.cpp
    PC_IntColorEditor.cpp
    PC_IntComboEditor.cpp
    PC_IntInputEditor.cpp
    PC_IntSliderEditor.cpp
    PC_ItemEditor.cpp
    PC_MainTreeControl.cpp
    PC_MultilineStringEditor.cpp
    PC_String3ButtonEditor.cpp
    PC_StringBigInputEditor.cpp
    PC_StringBrowseEditor.cpp
    PC_StringComboEditor.cpp
    PC_StringComboRefEditor.cpp
    PC_StringDirRefEditor.cpp
    PC_StringFileRefEditor.cpp
    PC_StringInputEditor.cpp
    PC_StringMultibuttonEditor.cpp
    PC_StringNewBrowseEditor.cpp
    PC_StringNewRefEditor.cpp
    PC_StringRefEditor.cpp
    PC_StringSliderEditor.cpp
    PC_TextFileEditor.cpp
    PC_Vec3ColorEditor.cpp
    ProgressViewWx.cpp
    BuildDataView.cpp
    BuildDataViewWx.cpp
    DBLinkViewWx.cpp
    ObjectBrowserViewWx.cpp
    PropertyButtons.cpp
    PropertyButtonsWx.cpp
    PropertyPaneView.cpp
    PropertyPaneViewWx.cpp
    RefListView.cpp
    RefListViewWx.cpp
    RegisterEditors.cpp
    ReplaceTextDlg.cpp
    SearchObjectViewWx.cpp
    SelectTablesViewWx.cpp
    SortTreeControl.cpp
    stdafx.cpp
    TextEditorView.cpp
    TextEditorViewWx.cpp
    Tree_GDBBrowser.cpp
    Tree_GDBBrowserBase.cpp
    Tree_GDBBrowser_InputState.cpp
    Tree_GDBLinkBrowser.cpp
    UserDataContainer.cpp
    ViewContainer.cpp
)
add_library(MapEditor SHARED ${SOURCES})
add_stdafx_pch(MapEditor)
add_export_header(MapEditor)
target_link_libraries(MapEditor PRIVATE 3DLib 3Dmotor ED_Common Input libdb Main MapEditorLib MemoryLib Misc Scintilla Script stingray System fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(MapEditor PRIVATE Htmlhelp winapi)
endif()
# The property control, the browsers and the main frame: dialogs, menus,
# toolbars, string tables and the bitmaps under res/.
# Windows only: these are Win32 resource scripts.
if(WIN32)
    target_sources(MapEditor PRIVATE MapEditor.rc)
endif()
# The main frame, its panes and the dialogs are wx. No UNICODE defines here
# either: see cmake/wxwidgets.cmake.
target_link_libraries(MapEditor PRIVATE wx::wx)
add_dependencies(MapEditor wxwidgets_external)

install(TARGETS MapEditor DESTINATION bin)
# The application icon as a file, beside the binary that shows it.
#
# The .rc compiles it into the resource section as IDR_MAINFRAME, and that is
# still where the frame and the MFC About box get it. But a resource reached by
# a numeric id is the one form wx cannot load portably -- and cannot load at all
# off Windows -- so the wx About box reads this copy and decodes it with
# NWinImage. Installed rather than copied at build time because it has to sit
# next to the *installed* executable, which is what wxStandardPaths answers with.
#
# Beside the binary rather than at the top of the install like the game's
# main.ico: that one is game data read through NMainLoop::GetBaseDir(), and an
# icon belonging to the editor is not.
install(FILES res/main_frame.ico DESTINATION bin)
add_version_info(MapEditor)
# The wx About box shows the revision, for bug reports (AboutViewWx.cpp).
add_git_revision(MapEditor)
