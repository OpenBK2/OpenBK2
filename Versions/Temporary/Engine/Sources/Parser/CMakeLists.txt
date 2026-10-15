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
bk2_add_library(Parser SHARED ${SOURCES})
target_link_libraries(Parser PRIVATE MemoryLib Misc System zlib fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(Parser PRIVATE winapi)
endif()

set_source_files_properties(lex.yy.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(lang.tab.c PROPERTIES LANGUAGE CXX)

add_subdirectory(test)
