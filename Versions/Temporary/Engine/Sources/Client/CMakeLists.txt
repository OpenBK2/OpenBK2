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
add_library(Client SHARED ${SOURCES})
add_stdafx_pch(Client)
add_export_header(Client)
target_link_libraries(Client PRIVATE MemoryLib Misc Net Server_Client_Common System fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Client PRIVATE winapi)
endif()
install(TARGETS Client DESTINATION bin)
add_version_info(Client)
