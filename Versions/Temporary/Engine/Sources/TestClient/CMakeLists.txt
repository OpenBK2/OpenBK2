project(TestClient)

# The load test client and watchdog for the dedicated lobby server. Three
# programs in one binary: an interactive console driving all 33 lobby commands,
# a load generator that runs many simulated clients at once off one timer, and
# a pinger that logs in every minute and drops a server_is_down marker file for
# an external supervisor to act on. It was the live service's own monitoring,
# not only a development tool.
#
# It talks to the server through CServerClient in the Client module, so unlike
# Server it needs no database and none of that work: it could have been built
# before any of it.
#
# The file list is TestClient.vcproj's, which is complete and accurate against
# the directory, case included. What is left out is the MFC shell:
#
#   console.cpp, TestClientApp.cpp   a Scintilla editor control in a CWnd, and
#                                    the CWinApp whose 50 ms Win32 timer is the
#                                    only main loop. Scintilla has no source in
#                                    this tree, so console.cpp could not have
#                                    compiled whatever else was done to it.
#                                    main.cpp does both jobs against stdin and
#                                    stdout instead.
set(SOURCES
    CommandsInterface.cpp
    InternalPackets.cpp
    main.cpp
    MultiTester.cpp
    Pinger.cpp
    StdAfx.cpp
    TestClient.cpp
    TestClientProcessor.cpp
)

# No WIN32, for the reason the server's build file gives: the old project chose
# /SUBSYSTEM:WINDOWS only to have a window for its console.
add_executable(TestClient ${SOURCES})
add_stdafx_pch(TestClient)

target_link_libraries(TestClient PRIVATE
    Client
    MemoryLib
    Misc
    Server_Client_Common
    System
    fmt::fmt-header-only)

if(WIN32)
    target_link_libraries(TestClient PRIVATE winapi)
endif()

# InternalPackets.cpp registers three packet classes of its own under the
# TESTCLIENT module id, and REGISTER_SAVELOAD_CLASS puts a TESTCLIENT_EXPORT on
# what it instantiates. An executable exports nothing, so the macro is empty,
# the same as the server's SERVER_EXPORT.
target_compile_definitions(TestClient PRIVATE TESTCLIENT_EXPORT=)

# The DLLs this loads at run time, beside the executable in the build tree and
# in the install. Each engine module builds into its own directory, and Windows
# looks for a DLL next to the executable or on PATH, never in the working
# directory, so without this the build tree can only be run after installing
# it: the loader fails with STATUS_DLL_NOT_FOUND before main() is reached, and
# the process dies with no message of its own. Game does the same thing for the
# same reason.
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:TestClient> DESTINATION bin)

    add_custom_command(TARGET TestClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:TestClient> $<TARGET_FILE_DIR:TestClient>
        COMMAND_EXPAND_LISTS
        VERBATIM)
endif()

install(TARGETS TestClient DESTINATION bin)
add_version_info(TestClient)
