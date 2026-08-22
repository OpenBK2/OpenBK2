project(System)
set(SOURCES
    Basic.cpp
    BasicShare.cpp
    BinaryResources.cpp
    BinChunkSaver.cpp
    BitStreams.cpp
    CheckSumLog.cpp
    ChunklessSaver.cpp
    CombinerVFS.cpp
    Commands.cpp
    ConsoleBufferInternal.cpp
    Cruncher.cpp
    Dg.cpp
    FastMath.cpp
    FilePath.cpp
    FileReaders.cpp
    FileUtils.cpp
    Formatting.cpp
    FreeIDs.cpp
    GameDB.cpp
    GlobalVars.cpp
    GResource.cpp
    LightXML.cpp
    LightXMLUtils.cpp
    Logger.cpp
    LogStream.cpp
    ObjectFactory.cpp
    Profiler.cpp
    RandomGenInternal.cpp
    Serialize.cpp
    Singleton.cpp
    SplashScreen.cpp
    stdafx.cpp
    Streams.cpp
    Text.cpp
    Time.cpp
    VFS.cpp
    VFSOperations.cpp
    WinCursor.cpp
    WinFrame.cpp
    WinVFS.cpp
    XMLChunkSaver.cpp
    XMLReader.cpp
    XMLSAXPArser.cpp
    XmlUtils.cpp
    ZipArchieve.cpp
)
add_library(System SHARED ${SOURCES})
add_stdafx_pch(System)
add_export_header(System)
target_link_libraries(System PRIVATE MemoryLib Misc zlib Threads::Threads)
target_link_libraries(System PUBLIC fmt::fmt-header-only Boost::uuid Boost::filesystem Boost::interprocess)
if(WIN32)
    target_link_libraries(System PRIVATE winapi)
endif()
install(TARGETS System DESTINATION bin)
add_version_info(System)

add_subdirectory(test)
