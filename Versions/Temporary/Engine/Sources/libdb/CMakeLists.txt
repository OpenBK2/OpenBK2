project(libdb)
set(SOURCES
    Bind.cpp
    BindArray.cpp
    BindProcessor.cpp
    BindProcessorSaveLoad.cpp
    Checksum.cpp
    CodeGenField.cpp
    CodeGenFile.cpp
    CodeGenForward.cpp
    CodeGenMisc.cpp
    CodeGenNamespace.cpp
    CodeGenType.cpp
    CodeStructure.cpp
    CompileCLike.cpp
    CompileCLikeVisitor.cpp
    Database.cpp
    DBObserverContainer.cpp
    EditorDatabase.cpp
    GameDatabase.cpp
    GameDBIterator.cpp
    Hungarian.cpp
    Logger.cpp
    ObjectRecordIDAllocator.cpp
    RenameNode.cpp
    ReportMetaInfo.cpp
    stdafx.cpp
    StructMetaInfo.cpp
    StructMetaInfoSetGet.cpp
    TestType.cpp
    Type.cpp
    TypeDef.cpp
    TypeDefType.cpp
    Variant.cpp
    Wrapper/FolderManipulator.cpp
    Wrapper/ObjectManipulator.cpp
    Wrapper/ResourceManagerInternal.cpp
    Wrapper/TableManipulator.cpp
)

# DBWatcherClient.cpp is the COM client for the XDBWatcher service, and it reaches the
# type library with `#import "../XDBWatcherClient/XDBWatcherClient.tlb"`. That is an
# MSVC extension: MSVC reads the type library and generates the C++ wrappers. Every
# other compiler either rejects it or, in GCC's case, does something worse - GCC has
# `#import` as its own deprecated "include once" directive, so it lexes the binary
# type library as source text and reports a stray character per byte. That one file
# is over a thousand of the errors in a Linux build.
#
# The guard is MSVC rather than WIN32 on purpose. `#import` of a .tlb is a property of
# the compiler, not the platform, and comdef.h, _com_ptr_t and bstr_t come with it;
# clang-cl and MinGW would fail here too.
#
# Excluding the translation unit is enough because DBWatcherClient.h holds only the
# interface and one declaration, no COM. The three built users of it - EditorDatabase,
# RenameNode and Wrapper/ResourceManagerInternal - reach the implementation through
# Singleton<IDBWatcherClient>() and need the header, never this .cpp. The only caller
# of RegisterSingleton() is MapEditor, which is Windows-only anyway, so nothing that
# builds elsewhere is left with an undefined symbol.
if(MSVC)
    list(APPEND SOURCES DBWatcherClient.cpp)
endif()

add_library(libdb SHARED ${SOURCES})
add_stdafx_pch(libdb)
add_export_header(libdb)
target_include_directories(libdb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libdb PRIVATE
    MemoryLib
    Misc
    Parser
    System
    zlib
    fmt::fmt-header-only
    Boost::uuid
)
if(WIN32)
    target_link_libraries(libdb PRIVATE winapi)
endif()
install(TARGETS libdb DESTINATION bin)
add_version_info(libdb)
