project(Net)
set(SOURCES
    LinksManager.cpp
    NetA4.cpp
    NetAcks.cpp
    NetAddress.cpp
    NetInterfaces.cpp
    NetLogin.cpp
    NetLowest.cpp
    NetPeer2Peer.cpp
    NetServerInfo.cpp
    NetStream.cpp
    stdafx.cpp
)
bk2_add_library(Net SHARED ${SOURCES})
# NetInterfaces.cpp needs winsock2.h for IP Helper on Windows, and the stdafx.h
# prelude brings in winsock.h, which winsock2.h cannot follow. See its header.
set_source_files_properties(NetInterfaces.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
target_link_libraries(Net PRIVATE MemoryLib Misc System fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Net PRIVATE Ws2_32 Iphlpapi)
endif()
add_subdirectory(test)
add_subdirectory(probe)
