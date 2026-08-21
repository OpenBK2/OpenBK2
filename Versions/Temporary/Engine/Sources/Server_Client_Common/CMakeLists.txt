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
add_library(Server_Client_Common SHARED ${SOURCES})
add_stdafx_pch(Server_Client_Common)
add_export_header(Server_Client_Common)
target_link_libraries(Server_Client_Common PRIVATE MemoryLib Misc Net System zlib fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Server_Client_Common PRIVATE winapi)
endif()
install(TARGETS Server_Client_Common DESTINATION bin)
add_version_info(Server_Client_Common)
