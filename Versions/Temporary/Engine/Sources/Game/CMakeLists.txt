project(Game)
set(SOURCES
    main.cpp
    stdafx.cpp
    SysKeys.cpp
    Test.cpp
    WinFrame2Input.cpp
    dpi_aware.manifest
)
add_executable(Game WIN32 ${SOURCES})
add_stdafx_pch(Game)
# Keep mimalloc first so its Windows redirect is loaded before other runtime users.
target_link_libraries(Game PRIVATE mimalloc 3DLib 3Dmotor AILogic B2_M1_Terrain B2_M1_World Client Common_RTS_AI DebugTools GameX Image Input libdb Main MemoryLib Misc Net Parser SceneB2 Script Server_Client_Common Sound Stats_B2_M1 System UISpecificB2 UI zlib fmt::fmt-header-only)
target_link_libraries(Game PRIVATE crashpad_client)
if(WIN32)
    target_link_libraries(Game PRIVATE winapi)
endif()
install(TARGETS Game DESTINATION bin)

# Ship the third-party DLLs Game loads at runtime. Boost is the reason this is
# needed: FetchContent builds the whole distribution, so naming the pieces by
# hand would go stale, while TARGET_RUNTIME_DLLS resolves exactly the ones in
# Game's link closure. The engine's own modules install themselves and appear
# here as well, which is harmless.
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:Game> DESTINATION bin)
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
