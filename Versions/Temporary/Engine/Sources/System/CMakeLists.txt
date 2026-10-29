project(System)
set(SOURCES
    AsyncTrace.cpp
    Basic.cpp
    BasicShare.cpp
    BinaryResources.cpp
    BinChunkSaver.cpp
    BitStreams.cpp
    BuildStamp.cpp
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
    GeneralSettings.cpp
    GlobalVars.cpp
    GResource.cpp
    LightXML.cpp
    LightXMLUtils.cpp
    Logger.cpp
    LogStream.cpp
    ObjectFactory.cpp
    PlatformPaths.cpp
    Profiler.cpp
    RandomGenInternal.cpp
    Serialize.cpp
    SecuredSettings.cpp
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
    WinImageFormats.cpp
    WinVFS.cpp
    XMLChunkSaver.cpp
    XMLReader.cpp
    XMLSAXPArser.cpp
    XmlUtils.cpp
    ZipArchieve.cpp
)
bk2_add_library(System SHARED ${SOURCES})
target_link_libraries(System PRIVATE MemoryLib Misc zlib Threads::Threads)
target_link_libraries(System PUBLIC fmt::fmt-header-only Boost::uuid Boost::filesystem Boost::interprocess)
if(WIN32)
    target_sources(System PRIVATE PlatformPathsWindows.cpp SecuredSettingsWindows.cpp)
    set_source_files_properties(PlatformPathsWindows.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    target_link_libraries(System PRIVATE winapi Advapi32 Shell32 Ole32)
else()
    # WinCursor decodes .cur itself and hands the pixels to SDL_CreateColorCursor
    target_sources(System PRIVATE SdlVideo.cpp)
    target_link_libraries(System PRIVATE SDL3::SDL3-shared)
    if(APPLE)
        enable_language(OBJCXX)
        target_sources(System PRIVATE PlatformPathsMac.mm SecuredSettingsMac.cpp)
        set_source_files_properties(PlatformPathsMac.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
        target_link_libraries(System PRIVATE "-framework Foundation" "-framework Security")
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(LIBSECRET REQUIRED IMPORTED_TARGET libsecret-1>=0.19)
        target_sources(System PRIVATE SecuredSettingsLinux.cpp)
        target_link_libraries(System PRIVATE PkgConfig::LIBSECRET)
    endif()
endif()

add_subdirectory(test)
