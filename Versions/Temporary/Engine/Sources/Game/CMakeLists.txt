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

# The crash handler is a separate executable that the game starts at run time,
# by the relative name main.cpp passes to CrashpadClient::StartHandler. Nothing
# was installing it: the copy in an existing Windows install tree got there by
# hand, and a fresh install of either platform had none, so the handler never
# started and crashes went unreported.
if(TARGET crashpad_handler)
    install(PROGRAMS $<TARGET_FILE:crashpad_handler> DESTINATION bin)
endif()

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
#
# main.ico sits beside splash.bmp at the top of the install rather than under
# share, because that is where this game keeps the files it opens at run time and
# the window icon is read from it too. One file, both readers.
if(UNIX AND NOT APPLE)
    configure_file(blitzkrieg2.desktop.in
                   ${CMAKE_CURRENT_BINARY_DIR}/blitzkrieg2.desktop @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/blitzkrieg2.desktop
            DESTINATION share/applications)
    install(FILES main.ico DESTINATION .)
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
else()
    # The same problem as the Windows branch above, and for the same reason:
    # FetchContent builds whole distributions, so naming the pieces by hand goes
    # stale. Boost is the one that bites today, since System links
    # boost_filesystem and nothing was installing it, but the list is not
    # something to maintain by hand.
    #
    # TARGET_RUNTIME_DLLS has no ELF counterpart, so this is the documented
    # equivalent: resolve the closure at install time from the build tree copy of
    # Game, which is the one carrying a RUNPATH that can find everything, and
    # install whatever came from outside the system directories.
    #
    # The engine's own modules are resolved too and installed a second time over
    # the copies their own install(TARGETS) put there. That is harmless, and
    # cheaper than a rule that tries to tell them apart.
    install(CODE "set(GAME_BINARY \"$<TARGET_FILE:Game>\")")
    install(CODE [[
        file(GET_RUNTIME_DEPENDENCIES
             EXECUTABLES "${GAME_BINARY}"
             RESOLVED_DEPENDENCIES_VAR resolved_runtime_deps
             UNRESOLVED_DEPENDENCIES_VAR unresolved_runtime_deps
             POST_EXCLUDE_REGEXES "^/lib" "^/usr/lib" "^/lib64" "^/usr/lib64")
        foreach(dependency IN LISTS resolved_runtime_deps)
            file(INSTALL
                 DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
                 TYPE SHARED_LIBRARY
                 FOLLOW_SYMLINK_CHAIN
                 FILES "${dependency}")
        endforeach()
        # Reported rather than fatal: an unresolved name here is usually a system
        # library the exclusions above did not match, which the loader will find
        # anyway.
        foreach(dependency IN LISTS unresolved_runtime_deps)
            message(STATUS "not installed, expected from the system: ${dependency}")
        endforeach()
    ]])
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
