project(MapEditor)
set(SOURCES
    AboutViewWx.cpp
    AppProfile.cpp
    BitFieldView.cpp
    BitFieldViewWx.cpp
    BuilderContainer.cpp
    ChildFrameContainer.cpp
    CommandHandlerContainer.cpp
    ControllerContainer.cpp
    CreateModView.cpp
    CreateModViewWx.cpp
    EditorContainer.cpp
    ExporterContainer.cpp
    FileDialogsWx.cpp
    FolderCallback.cpp
    GDBBrowserPane.cpp
    LogPane.cpp
    LogViewWx.cpp
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
    NewObjectView.cpp
    NewObjectViewWx.cpp
    ObjectCollector.cpp
    ResourceBinary.cpp
    ResourceMenus.cpp
    ResourceStrings.cpp
    OpenModViewWx.cpp
    PC_BaseDialog.cpp
    ProgressViewWx.cpp
    BuildDataView.cpp
    BuildDataViewWx.cpp
    DBLinkViewWx.cpp
    ObjectBrowserViewWx.cpp
    PropertyButtons.cpp
    PropertyButtonsWx.cpp
    PropertyPaneView.cpp
    PropertyPaneViewWx.cpp
    PropertyValues.cpp
    RefListView.cpp
    RefListViewWx.cpp
    RegisterEditors.cpp
    SearchObjectViewWx.cpp
    SelectTablesViewWx.cpp
    stdafx.cpp
    TextEditorView.cpp
    TextEditorViewWx.cpp
    UserDataContainer.cpp
    ViewContainer.cpp
    XdbValidationWx.cpp
)
add_library(MapEditor SHARED ${SOURCES})
add_stdafx_pch(MapEditor)
add_export_header(MapEditor)
# No mfc: CEditorApp is no longer a CWinApp, and NAppProfile (AppProfile.h)
# keeps the registry profile CWinApp kept.
target_link_libraries(MapEditor PRIVATE 3DLib 3Dmotor ED_Common Input libdb Main MapEditorLib MemoryLib Misc Script System fmt::fmt-header-only)
if(WIN32)
    # comctl32: InitCommonControls in MapEditorApp, which MFC's headers used to
    # link through #pragma comment. The object collector's image lists wanted it
    # too until they became wxImageLists.
    # shlwapi went with SHDeleteKey, which wiped the profile key until
    # wxConfig took the profile over.
    target_link_libraries(MapEditor PRIVATE Htmlhelp winapi comctl32)
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
