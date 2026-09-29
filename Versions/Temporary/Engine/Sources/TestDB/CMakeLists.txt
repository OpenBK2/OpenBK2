# The database types libdb/test runs the editor database against, and the
# records in data/ it opens. TestType.h/.cpp and data/types.xml are dbcodegen's
# output for TestType.cll; the regenerate-db target rewrites them (see
# cmake/dbcodegen.cmake).
#
# A shared library like every module, because the generated registration
# expands to TESTDB_EXPORT, and built only when a test that links it is.
#
# TestType.cll is deliberately not in SOURCES: the build collects the .cll files
# listed there into the game's types.xml, and these types are not the game's.
project(TestDB)
set(SOURCES
    stdafx.cpp
    TestDB.cpp
    TestType.cpp
)
add_library(TestDB SHARED EXCLUDE_FROM_ALL ${SOURCES})
add_stdafx_pch(TestDB)
add_export_header(TestDB)
target_link_libraries(TestDB PRIVATE libdb MemoryLib Misc System fmt::fmt-header-only)
target_compile_definitions(TestDB PRIVATE TESTDB_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
