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
add_executable(B2_MapEditor WIN32 ${SOURCES})
add_stdafx_pch(B2_MapEditor)
target_link_libraries(B2_MapEditor PRIVATE 3DLib 3Dmotor AILogic B2_M1_Terrain B2_M1_World Client Common_RTS_AI DebugTools ED_B2 ED_B2_M1 ED_Common ED_RTS GameX Image Input libdb Main MapEditorLib MapEditor MemoryLib Misc Net Parser SceneB2 Scintilla Script Server_Client_Common Sound Stats_B2_M1 stingray System UISpecificB2 UI zlib)
target_link_libraries(B2_MapEditor PRIVATE crashpad_client)
if(WIN32)
    target_link_libraries(B2_MapEditor PRIVATE winapi)
endif()
# b2_res.rc is the executable's aggregate: it includes MapEditor.rc and
# ED_B2_M1.rc, so the exe carries a second copy of what those two DLLs
# carry. That is deliberate and is what the retail editor did, because MFC
# looks resources up in the module that asks for them and both the exe and
# the DLLs ask. legacy.rc is the main frame menu recovered from the retail
# binary.
# Windows only: these are Win32 resource scripts.
if(WIN32)
    target_sources(B2_MapEditor PRIVATE b2_res.rc legacy.rc)
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
