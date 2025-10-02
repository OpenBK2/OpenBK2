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
include(GenerateExportHeader)
add_library(Client SHARED ${SOURCES})
generate_export_header(Client EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Client_export.h)
target_link_libraries(Client PRIVATE MemoryLib Misc Net Server_Client_Common System)
install(TARGETS Client DESTINATION bin)
