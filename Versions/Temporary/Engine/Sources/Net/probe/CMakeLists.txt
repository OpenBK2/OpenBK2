# NetProbe: the game's LAN driver run from a terminal, so multiplayer discovery
# and login can be checked without two game instances. Usage is in the comment
# at the top of NetProbe.cpp.
#
# It compiles the Net sources in, the way NetTransport_test does, rather than
# linking the module: CNodeAddressSet and the driver internals it reports on
# are not exported. The sources are the module's own, so the behaviour is the
# game's.
bk2_add_executable(NetProbe NO_PCH
    NetProbe.cpp
    ../LinksManager.cpp
    ../NetA4.cpp
    ../NetAcks.cpp
    ../NetAddress.cpp
    ../NetLogin.cpp
    ../NetLowest.cpp
    ../NetPeer2Peer.cpp
    ../NetServerInfo.cpp
    ../NetStream.cpp)
target_compile_definitions(NetProbe PRIVATE NET_STATIC_DEFINE)
target_link_libraries(NetProbe PRIVATE MemoryLib Misc System fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(NetProbe PRIVATE Ws2_32)
endif()
install_runtime_dlls(NetProbe)
