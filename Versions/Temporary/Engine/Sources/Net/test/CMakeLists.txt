# Compile the internal trackers and driver so packet timing can be controlled
# without sockets, worker threads, or new DLL exports.
add_unit_test(NetTransport MemoryLib Misc System fmt::fmt-header-only SOURCES
    ../LinksManager.cpp
    ../NetA4.cpp
    ../NetAcks.cpp
    ../NetAddress.cpp
    ../NetInterfaces.cpp
    ../NetLogin.cpp
    ../NetLowest.cpp
    ../NetPeer2Peer.cpp
    ../NetServerInfo.cpp
    ../NetStream.cpp)
target_compile_definitions(NetTransport_test PRIVATE NET_STATIC_DEFINE)
# Exercise retransmission aliases through a complete packet-ID cycle.
add_unit_test(NetPacketLifetime MemoryLib Misc System fmt::fmt-header-only SOURCES
    ../NetAcks.cpp
    ../NetStream.cpp)
target_compile_definitions(NetPacketLifetime_test PRIVATE NET_STATIC_DEFINE)
if(WIN32)
    target_link_libraries(NetTransport_test PRIVATE Ws2_32 Iphlpapi)
endif()
