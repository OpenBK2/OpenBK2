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
add_library(Net SHARED ${SOURCES})
add_stdafx_pch(Net)
add_export_header(Net)
target_link_libraries(Net PRIVATE MemoryLib Misc System Ws2_32)
install(TARGETS Net DESTINATION bin)
add_version_info(Net)
