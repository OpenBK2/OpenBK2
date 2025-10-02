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
include(GenerateExportHeader)
add_library(Server_Client_Common SHARED ${SOURCES})
generate_export_header(Server_Client_Common EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Server_Client_Common_export.h)
target_link_libraries(Server_Client_Common PRIVATE MemoryLib Misc Net System zlib)
install(TARGETS Server_Client_Common DESTINATION bin)
