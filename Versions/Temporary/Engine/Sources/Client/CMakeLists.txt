project(Client)
set(SOURCES
    Connection.cpp
    ConnectionEffort.cpp
    ConnectionEffortInternal.cpp
    ConnectionInternal.cpp
    ConnectServerProcessor.cpp
    LANClient.cpp
    LinksManager.cpp
    PlayGameProcessor.cpp
    ServerClient.cpp
    StdAfx.cpp
)
bk2_add_library(Client SHARED ${SOURCES})
target_link_libraries(Client PRIVATE MemoryLib Misc Net Server_Client_Common System fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Client PRIVATE winapi)
endif()
add_subdirectory(test)
