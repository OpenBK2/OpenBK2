project(Net)
set(SOURCES
    LinksManager.cpp
    NetA4.cpp
    NetAcks.cpp
    NetAddress.cpp
    NetLogin.cpp
    NetLowest.cpp
    NetPeer2Peer.cpp
    NetServerInfo.cpp
    NetStream.cpp
    stdafx.cpp
)
include(GenerateExportHeader)
add_library(Net SHARED ${SOURCES})
generate_export_header(Net EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Net_export.h)
target_link_libraries(Net PRIVATE MemoryLib Misc System Ws2_32)
install(TARGETS Net DESTINATION bin)
add_version_info(Net)
