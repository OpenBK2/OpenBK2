project(Parser)
set(SOURCES
    ErrorsAndMessages.cpp
    FileNode.cpp
    FileRead.cpp
    lang.tab.c
    LangNode.cpp
    LangNodesDefinitions.cpp
    lex.yy.c
    ParseOperations.cpp
    stdafx.cpp
    StringNumbers.cpp
)
include(GenerateExportHeader)
add_library(Parser SHARED ${SOURCES})
generate_export_header(Parser EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/Parser_export.h)
target_link_libraries(Parser PRIVATE MemoryLib Misc System zlib)
install(TARGETS Parser DESTINATION bin)

set_source_files_properties(lex.yy.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(lang.tab.c PROPERTIES LANGUAGE CXX)
