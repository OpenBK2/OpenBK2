# Exercise internal connection queues and a complete local UDP peer handshake
# without introducing public DLL exports solely for tests.
add_unit_test(ClientConnectionRemoval MemoryLib Misc Net Server_Client_Common System fmt::fmt-header-only SOURCES
    ../Connection.cpp
    ../ConnectionEffort.cpp
    ../ConnectionEffortInternal.cpp
    ../ConnectionInternal.cpp
    ../ConnectServerProcessor.cpp
    ../LinksManager.cpp
    ../PlayGameProcessor.cpp
    ../PeerSessions.cpp)
target_compile_definitions(ClientConnectionRemoval_test PRIVATE CLIENT_STATIC_DEFINE)
if(WIN32)
    target_link_libraries(ClientConnectionRemoval_test PRIVATE winapi)
endif()
