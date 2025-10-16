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
add_library(Parser SHARED ${SOURCES})
add_stdafx_pch(Parser)
add_export_header(Parser)
target_link_libraries(Parser PRIVATE MemoryLib Misc System zlib winapi fmt::fmt-header-only)
install(TARGETS Parser DESTINATION bin)
add_version_info(Parser)

set_source_files_properties(lex.yy.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(lang.tab.c PROPERTIES LANGUAGE CXX)
