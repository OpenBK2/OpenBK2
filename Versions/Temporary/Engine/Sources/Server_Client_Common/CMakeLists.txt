project(Server_Client_Common)
set(SOURCES
    Commands.cpp
    CommandsInterface.cpp
    LogSaver.cpp
    Net.cpp
    NetLogger.cpp
    NetPacket.cpp
    NetSaver.cpp
    PacketProcessor.cpp
    PacketsRegistration.cpp
    PrimeNumbers.cpp
    StdAfx.cpp
    ZipPacket.cpp
)
bk2_add_library(Server_Client_Common SHARED ${SOURCES})
target_link_libraries(Server_Client_Common PRIVATE MemoryLib Misc Net System zlib fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Server_Client_Common PRIVATE winapi)
endif()
