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

install(TARGETS TestClient DESTINATION bin)
add_version_info(TestClient)
