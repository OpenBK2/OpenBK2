project(B2_MapEditor)
set(SOURCES
    main.cpp
    stdafx.cpp
    VFSDbObserver.cpp
)
# The UTF-8 active code page, which the game has and this did not. Every
# narrow string in the tree is UTF-8, and without this the process code page
# is the system one -- 1251 here -- so every ...A call in MFC and the common
# controls decoded those bytes as Cyrillic and unit names came out mojibake.
# Listed as a source because that is how the linker merges a manifest in, and
# it merges with the MANIFESTDEPENDENCY below rather than replacing it.
list(APPEND SOURCES utf8.manifest)
# The entry point: wx's WinMain and the wxApp that runs the editor.
list(APPEND SOURCES WxHost.cpp)
add_executable(B2_MapEditor WIN32 ${SOURCES})
add_stdafx_pch(B2_MapEditor)
# No mfc, here or in any module the editor loads: wx owns WinMain.
target_link_libraries(B2_MapEditor PRIVATE 3DLib 3Dmotor AILogic B2_M1_Terrain B2_M1_World Client Common_RTS_AI DebugTools ED_B2 ED_B2_M1 ED_Common GameX Image Input libdb Main MapEditorLib MapEditor MemoryLib Misc Net Parser SceneB2 Script Server_Client_Common Sound Stats_B2_M1 System UISpecificB2 UI zlib)
target_link_libraries(B2_MapEditor PRIVATE crashpad_client)
if(WIN32)
    target_link_libraries(B2_MapEditor PRIVATE winapi)
endif()
# b2_res.rc includes MapEditor.rc, which is what is left of the editor's
# resource scripts: the application icons, which the shell reads out of the
# PE. Everything else is generated C++ tables in the module that owns it.
#
# legacy.rc went with it. It carried the main frame menu recovered from the
# retail binary, which nothing has loaded since MFC's LoadFrame did: the menu
# bars come from IDM_MAIN, IDM_MAPINFO and IDM_MODEL through AddMenuResources.
# Windows only: this is a Win32 resource script.
if(WIN32)
    target_sources(B2_MapEditor PRIVATE b2_res.rc)
endif()
# Visual styles. Without the Common-Controls 6.0 dependency in the manifest
# every dialog, button and tree in the editor comes up in the Windows 95 theme.
#
# ED_B2_M1.rc used to carry this as an RT_MANIFEST resource at id 1, which put
# an application manifest inside a DLL, where it means nothing, and a second
# copy in this executable through b2_res.rc, which CVTRES rejects outright.
# Asked for here it lands once, in the binary it belongs to, and
# processorArchitecture is * rather than the X86 that manifest named, which
# does not resolve on x64.
# https://learn.microsoft.com/en-us/windows/win32/controls/cookbook-overview
if(MSVC)
    target_link_options(B2_MapEditor PRIVATE
        "/MANIFESTDEPENDENCY:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'")
endif()

# wx inside the MFC application; wx::wx brings the headers, the import library
# and WXUSINGDLL. Note there is deliberately no UNICODE or _UNICODE here: this
# target stays MBCS like every other, so there is one MFC in the process. wx
# does not want either of them. See cmake/wxwidgets.cmake.
target_link_libraries(B2_MapEditor PRIVATE wx::wx)
add_dependencies(B2_MapEditor wxwidgets_external)
add_custom_command(TARGET B2_MapEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -DWX_LIB_DIR=${WX_LIB_DIR}
            -DWX_DESTINATION=$<TARGET_FILE_DIR:B2_MapEditor>
            -P ${CMAKE_SOURCE_DIR}/cmake/copy_wx_runtime.cmake
    COMMENT "Copying the wxWidgets runtime beside B2_MapEditor"
    VERBATIM)
# And into the install tree, because the installed editor links wx.
# A directory glob for the same reason the build-tree copy is one: the DLL
# name carries wx's version, the compiler and the architecture.
# Off Windows the same library is libwx_gtk3u-3.3.so plus its versioned names,
# found at run time through the $ORIGIN rpath. Only the top of lib/ there: its
# wx/ subdirectory holds headers and the wx-config data, not runtime files.
if(WIN32)
    install(DIRECTORY ${WX_LIB_DIR}/ DESTINATION bin FILES_MATCHING PATTERN "*.dll")
else()
    install(DIRECTORY ${WX_LIB_DIR}/ DESTINATION bin FILES_MATCHING PATTERN "libwx_*.so*"
        PATTERN "wx" EXCLUDE PATTERN "cmake" EXCLUDE PATTERN "pkgconfig" EXCLUDE)
endif()

install(TARGETS B2_MapEditor DESTINATION bin)

# The crash handler is a separate executable that main.cpp starts at run time
# by the relative name it passes to CrashpadClient::StartHandler, so it has to
# be beside the editor. Game installs it too; installing the same file twice
# into the same place is harmless, and asking for it here is what makes an
# editor-only install tree complete rather than one that silently never starts
# a handler.
if(TARGET crashpad_handler)
    install(PROGRAMS $<TARGET_FILE:crashpad_handler> DESTINATION bin)
endif()
add_version_info(B2_MapEditor)
