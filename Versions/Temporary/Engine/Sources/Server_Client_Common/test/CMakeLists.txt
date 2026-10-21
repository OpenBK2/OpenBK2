# Compile the net saver into the test rather than linking the whole module, the
# way Net/test does with the transport: CreateNetSaver is all this needs, and
# the module's own link line pulls in the server and client packet machinery.
add_unit_test(NetSaverWideStrings MemoryLib Misc System zlib fmt::fmt-header-only SOURCES
    ../NetSaver.cpp
    ../NetPacket.cpp)
target_compile_definitions(NetSaverWideStrings_test PRIVATE SERVER_CLIENT_COMMON_STATIC_DEFINE NET_STATIC_DEFINE)
