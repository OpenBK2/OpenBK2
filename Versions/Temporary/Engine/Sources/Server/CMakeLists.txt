project(Server)

# NivalNET, the dedicated lobby server: accounts, chat, the custom games list
# and ladder matchmaking. It is not a game host and links none of the
# simulation: the players' machines run the match peer to peer between
# themselves, and this only introduces them and relays for the ones NAT keeps
# apart.
#
# The file list comes from Server.vcproj, with main.cpp in place of four things
# that were deleted rather than ported:
#
#   console.cpp, ServerApp.cpp  the MFC shell and its Scintilla console window.
#                               Scintilla has no source in this tree, only
#                               prebuilt DLLs, so console.cpp could not have
#                               compiled in any case, and the window it opened
#                               had already stopped showing server output.
#   server.rc, Resource.h       three lines of dead resource script, and the
#   main.ico, server.ico        icons and splash a daemon has no use for.
#
# CommandsInterface.cpp duplicates the file of the same name in
# Server_Client_Common on purpose: CCommandsBase carries no export macro, so
# each module that uses it compiles its own copy of those definitions.
set(SOURCES
    Chat.cpp
    ClientsLogGameResult.cpp
    clients.cpp
    CommandsInterface.cpp
    ControlLobby.cpp
    CustomLobby.cpp
    DatabaseMariaDb.cpp
    DatabaseSqlite.cpp
    GameLobby.cpp
    HashMapConvertor.cpp
    InternalPackets.cpp
    LadderLobby.cpp
    main.cpp
    Server.cpp
    StdAfx.cpp
    Statistics.cpp
    Terminal.cpp
    UpdatableList.cpp
)

# No WIN32: the original was /SUBSYSTEM:WINDOWS because the console was an MFC
# window. A daemon wants the console subsystem.
add_executable(Server ${SOURCES})
add_stdafx_pch(Server)

# Not libdb: Server.cpp includes libdb/Logger.h but never names CLogger, and
# DebugTrace comes from Misc/Tools.h.
target_link_libraries(Server PRIVATE
    MemoryLib
    Misc
    Net
    Server_Client_Common
    System
    mariadbclient
    sqlite3
    zlib
    fmt::fmt-header-only)

if(WIN32)
    # Terminal.cpp still calls WSAStartup and the Winsock spellings directly.
    # It moves onto port/socket.h in step 3, at which point this guard is what
    # keeps the POSIX build from asking for a library that is not there.
    target_link_libraries(Server PRIVATE Ws2_32 winapi)
endif()

# BASIC_REGISTER_CLASS( SERVER, X ) expands to a SERVER_EXPORT on the cast
# helpers it instantiates, the way every module spells its own export macro.
# Server is an executable and exports nothing, and generate_export_header only
# has a library to describe, so the macro is defined empty here rather than
# given a header of its own.
target_compile_definitions(Server PRIVATE SERVER_EXPORT=)

# The DLLs this loads at run time, beside the executable in the build tree and
# in the install. Each engine module builds into its own directory, and Windows
# looks for a DLL next to the executable or on PATH, never in the working
# directory, so without this the build tree can only be run after installing
# it: the loader fails with STATUS_DLL_NOT_FOUND before main() is reached, and
# the process dies with no message of its own. Game does the same thing for the
# same reason.
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:Server> DESTINATION bin)

    add_custom_command(TARGET Server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:Server> $<TARGET_FILE_DIR:Server>
        COMMAND_EXPAND_LISTS
        VERBATIM)
endif()

install(TARGETS Server DESTINATION bin)
add_version_info(Server)
