project(Server)
set(SOURCES
        StdAfx.cpp
        server.rc
        CommandsInterface.cpp
        Console.cpp
        Server.cpp
        ControlLobby.cpp
        CustomLobby.cpp
        GameLobby.cpp
        LadderLobby.cpp
        UpdatableList.cpp
        clients.cpp
        ClientsLogGameResult.cpp
        HashMapConvertor.cpp
        HashMapConvertor.h
        InternalPackets.cpp
        Chat.cpp
        Statistics.cpp
        Statistics.hpp
        Terminal.cpp
        Terminal.h
)
add_executable(Server WIN32 ${SOURCES})
target_link_libraries(Server PRIVATE Misc System MemoryLib Scintilla Net Server_Client_Common zlib crashpad_client)
target_compile_definitions(Server PUBLIC __AFX__ _AFXDLL)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Server PUBLIC _AFX_FULL_DEBUG)
endif()
install(TARGETS Server DESTINATION bin)
add_version_info(Server)
