project(Game)
set(SOURCES
    Initialization.cpp
    main.cpp
    stdafx.cpp
    SysKeys.cpp
    Test.cpp
    WinFrame2Input.cpp
)
add_executable(Game WIN32 ${SOURCES})
target_link_libraries(Game PRIVATE 3DLib 3Dmotor AILogic B2_M1_Terrain B2_M1_World Client Common_RTS_AI DebugTools GameX Image Input libdb Main MemoryLib Misc Net Parser SceneB2 Script Server_Client_Common Sound Stats_B2_M1 System UISpecificB2 UI zlib winapi)
install(TARGETS Game DESTINATION bin)
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_link_options(Game PRIVATE "/LARGEADDRESSAWARE")
endif()

# add icon (for windows)
if(WIN32)
	target_sources(Game PRIVATE icon.rc)
endif()
