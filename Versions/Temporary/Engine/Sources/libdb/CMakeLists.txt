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
    DBWatcherClient.cpp
    EditorDatabase.cpp
    GameDatabase.cpp
    GameDBIterator.cpp
    Hungarian.cpp
    Logger.cpp
    Nodes2TypeDefs.cpp
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
include(GenerateExportHeader)
add_library(libdb SHARED ${SOURCES})
generate_export_header(libdb EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/libdb_export.h)
target_include_directories(libdb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libdb PRIVATE MemoryLib Misc Parser System zlib winapi)
install(TARGETS libdb DESTINATION bin)
