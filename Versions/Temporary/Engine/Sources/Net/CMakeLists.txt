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
bk2_add_library(Net SHARED ${SOURCES})
target_link_libraries(Net PRIVATE MemoryLib Misc System fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Net PRIVATE Ws2_32)
endif()
add_subdirectory(test)
add_subdirectory(probe)
