include(FetchContent)

# SQLite (public domain), the embedded backend for the dedicated server's
# account, chat and ladder tables.
#
# The server was written against MySQL and keeps a MySQL backend, because an
# existing NivalNET database dump loads straight into one and that is what
# step 2a validates against. This is the second backend, behind the same
# IDatabase interface, and it is what makes a self-hosted server runnable
# without a database daemon: no service, no port, no credentials, just a file
# next to server.xml.
#
# Nothing in the server's use of a database argues for a server-class one. It
# opens no transactions, prepares no statements, and is single threaded apart
# from the terminal's reader, so the concurrency SQLite gives up costs nothing
# here.
#
# The amalgamation rather than the source repository: sqlite.org ships the
# whole library as one generated sqlite3.c, while github.com/sqlite/sqlite is
# the unamalgamated tree and needs tclsh to produce that file. The zip is the
# official release artifact, so it is pinned by hash rather than by tag.
FetchContent_Declare(
        sqlite3
        URL https://sqlite.org/2026/sqlite-amalgamation-3530400.zip
        URL_HASH SHA256=1e71ddf93849c6a6ecf58b827c0692073d2dd7ee40196158068f7b29f422e87d
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# The amalgamation carries no CMakeLists, so MakeAvailable populates it and
# stops there, and the target is declared here. shell.c is the sqlite3 command
# line tool and is deliberately left out: it has its own main().
FetchContent_MakeAvailable(sqlite3)

add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)

target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})

# Compile-time options, all of them recommended by sqlite.org for new
# applications. THREADSAFE=1 is the default and is kept: the server is single
# threaded today, but the terminal already runs a second thread and the
# serialized mode costs nothing measurable at this query volume.
target_compile_definitions(sqlite3 PRIVATE
        SQLITE_DQS=0                      # reject double-quoted string literals
        SQLITE_DEFAULT_FOREIGN_KEYS=1     # dbstruct.sql declares real foreign keys
        SQLITE_ENABLE_COLUMN_METADATA     # the IDatabase column introspection SHOW COLUMNS becomes
        SQLITE_OMIT_DEPRECATED
        SQLITE_OMIT_LOAD_EXTENSION        # nothing loads one, and it is an attack surface
        SQLITE_USE_URI=1)

# A static library linked into an executable that is built position independent
# like every other target here.
set_target_properties(sqlite3 PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(sqlite3 PROPERTIES FOLDER "third_party/SQLite")

# sqlite3.c is generated C and warns freely under this tree's warning level;
# none of it is ours to fix.
if(MSVC)
    target_compile_options(sqlite3 PRIVATE /w)
else()
    target_compile_options(sqlite3 PRIVATE -w)
endif()
