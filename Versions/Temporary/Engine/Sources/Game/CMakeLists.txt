project(Game)
set(SOURCES
    main.cpp
    stdafx.cpp
    Test.cpp
    WinFrame2Input.cpp
    dpi_aware.manifest
    utf8.manifest
)
add_executable(Game WIN32 ${SOURCES})
add_stdafx_pch(Game)
# Keep mimalloc first so its Windows redirect is loaded before other runtime users.
target_link_libraries(Game PRIVATE mimalloc 3DLib 3Dmotor AILogic B2_M1_Terrain B2_M1_World Client Common_RTS_AI DebugTools GameX Image Input libdb Main MemoryLib Misc Net Parser SceneB2 Script Server_Client_Common Sound Stats_B2_M1 System UISpecificB2 UI zlib fmt::fmt-header-only)
target_link_libraries(Game PRIVATE crashpad_client)
if(WIN32)
    target_link_libraries(Game PRIVATE winapi)
else()
    # port/messagebox.h reports the startup failures main.cpp cannot log
    target_link_libraries(Game PRIVATE SDL3::SDL3-shared)
endif()
install(TARGETS Game DESTINATION bin)

# A desktop entry, so the game has a name and a launcher entry rather than being
# an anonymous binary. It also matters for the window icon: a Wayland compositor
# without xdg_toplevel_icon_v1 takes the icon from the desktop file that matches
# the window's app id, and there is nowhere else for it to look.
#
# Path= is not optional here. The game resolves everything against the working
# directory: main.cpp opens "..\\profiles\\startup.cfg" and
# NMainLoop::GetBaseDir is the parent of the current directory, so launching from
# anywhere else finds no data at all.
#
# Icon= is the absolute path of the installed .ico rather than a themed name.
# The icon theme spec wants PNG, XPM or SVG under share/icons/hicolor, and the
# .ico is the file this tree actually has; GTK and KDE both read it through their
# image loaders. A generated PNG in the themed location is the tidier form and can
# replace this without touching anything else.
if(UNIX AND NOT APPLE)
    configure_file(blitzkrieg2.desktop.in
                   ${CMAKE_CURRENT_BINARY_DIR}/blitzkrieg2.desktop @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/blitzkrieg2.desktop
            DESTINATION share/applications)
    install(FILES main.ico DESTINATION share/blitzkrieg2)
endif()

# Ship the third-party DLLs Game loads at runtime. Boost is the reason this is
# needed: FetchContent builds the whole distribution, so naming the pieces by
# hand would go stale, while TARGET_RUNTIME_DLLS resolves exactly the ones in
# Game's link closure. The engine's own modules install themselves and appear
# here as well, which is harmless.
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:Game> DESTINATION bin)

    # Also put them next to Game.exe in the build tree. Each module builds into
    # its own directory, and Windows looks for a DLL beside the executable or
    # on PATH, never in the working directory, so without this the build tree
    # can only be run after installing it. Setting the working directory does
    # not help: that governs where the game looks for its data.
    add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:Game> $<TARGET_FILE_DIR:Game>
        COMMAND_EXPAND_LISTS
        VERBATIM)
endif()
add_version_info(Game)
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_link_options(Game PRIVATE "/LARGEADDRESSAWARE")
endif()

# https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows
if (MSVC)
    set_property(TARGET Game PROPERTY VS_DPI_AWARE PerMonitor)
endif()

# add icon (for windows)
if(WIN32)
	target_sources(Game PRIVATE icon.rc)
endif()
