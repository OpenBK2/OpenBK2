include(FetchContent)

# MariaDB Connector/C, the client library the dedicated server talks to its
# account and ladder database through.
#
# MariaDB's connector rather than MySQL's, for two reasons. It is LGPL, where
# the MySQL C client is GPL with a FOSS exception, and it is wire and API
# compatible with what the server was written against in 2005: the same
# mysql.h spellings, so the call sites need no translation to reach either a
# MySQL or a MariaDB server. The vendored copy the sources include,
# vendor/MySQL/include/mysql.h, was never in this tree.
#
# This is not the only backend. See cmake/sqlite.cmake for the embedded one,
# which is what a self-hosted server can run without a database daemon; both
# sit behind IDatabase. This one comes first because an existing NivalNET dump
# loads straight into it, which is what makes it the reference to check the
# other against.
FetchContent_Declare(
        mariadb_connector
        GIT_REPOSITORY https://github.com/mariadb-corporation/mariadb-connector-c.git
        GIT_TAG 8153da40fe720b1d25db12aa3f993918da15bd3b #refs/tags/v3.4.10
        GIT_PROGRESS TRUE
)

# Only the client library is wanted. Its own default build also produces the
# plugins, the curl-backed ones among them, and installs a set of files this
# tree has no use for.
set(WITH_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(WITH_CURL OFF CACHE BOOL "" FORCE)
set(WITH_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_MYSQLCOMPAT OFF CACHE BOOL "" FORCE)
set(WITH_DOCS OFF CACHE BOOL "" FORCE)
set(WITH_MSI OFF CACHE BOOL "" FORCE)

# Every plugin linked into the library rather than built as a loadable module.
#
# The connector leaves INSTALL_PLUGINDIR empty when it is a subproject, which
# is what FetchContent makes it, on the reasonable view that a subproject has
# no business installing plugins into a layout of its own. Its dynamic plugins
# still generate an install rule, and one with an empty destination fails
# configure. Building them static is the shape that expectation implies, and it
# also means the server carries its authentication methods in the executable
# rather than looking for DLLs beside it at run time.
#
# They are named one by one because each plugin decides its own default, and
# because turning them off instead would decide which servers can be connected
# to: caching_sha2_password is how MySQL 8 authenticates by default, and
# client_ed25519 and parsec are MariaDB's. That choice does not belong in a
# build file. A future version adding another dynamic plugin fails configure
# the same way and gets added here.
#
# ZSTD is on this list because of a platform difference rather than a version
# one: the plugin is only registered when the connector finds libzstd, which it
# does not on the Windows machine here and does on Linux. So the same tree
# configured on Linux met a dynamic plugin Windows never saw. That is worth
# remembering for the rest of this list: what is registered depends on what is
# installed.
foreach(plugin
        DIALOG
        CLIENT_ED25519
        CACHING_SHA2_PASSWORD
        SHA256_PASSWORD
        PARSEC
        MYSQL_CLEAR_PASSWORD
        PVIO_SHMEM
        ZSTD)
    set(CLIENT_PLUGIN_${plugin} STATIC CACHE STRING "" FORCE)
endforeach()

# GSSAPI and Kerberos single sign on, which this server does not use and which
# would put a system library on the link line for it.
set(CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT OFF CACHE STRING "" FORCE)

# This tree's zlib, rather than the copy the connector would otherwise build or
# go looking for on the system.
#
# Its default is WITH_EXTERNAL_ZLIB OFF, which adds its own external/zlib
# subdirectory, so a build would carry two zlibs and the engine and the
# connector would each be calling a different one. Turning the option on makes
# it FIND_PACKAGE(ZLIB REQUIRED), which fails here: this tree has no system
# zlib either, it fetches zlib-ng and builds it as the target named zlib.
#
# The connector checks ZLIB_FOUND before searching, for exactly this case, so
# the answer is supplied rather than found. zlib-ng is configured with
# ZLIB_COMPAT, so it is zlib as far as anything here can tell, and it is
# already built by the time this file is included.
set(WITH_EXTERNAL_ZLIB ON CACHE BOOL "" FORCE)
if(NOT TARGET zlib)
    message(FATAL_ERROR "cmake/mariadb.cmake must be included after cmake/zlib-ng.cmake")
endif()
set(ZLIB_FOUND TRUE)
set(ZLIB_LIBRARY zlib)
set(ZLIB_LIBRARIES zlib)
get_target_property(ZLIB_INCLUDE_DIR zlib INTERFACE_INCLUDE_DIRECTORIES)

# Schannel on Windows, so the connector does not pull in OpenSSL there. Off
# Windows it takes the system OpenSSL, which every target platform has.
if(WIN32)
    set(WITH_SSL SCHANNEL CACHE STRING "" FORCE)
endif()

FetchContent_MakeAvailable(mariadb_connector)

# mariadbclient is the static library, which is what the server links: nothing
# else in the tree talks to a database, so there is no reason to ship a DLL
# beside the executable for it. libmariadb, the shared one, is built too and
# simply goes unused.
#
# Its own CMake predates usage requirements and tells consumers nothing about
# where its headers are, so that is attached here rather than repeated by every
# target that links it.
if(NOT TARGET mariadbclient)
    message(FATAL_ERROR "MariaDB Connector/C did not declare the mariadbclient target")
endif()
#
# Two directories, because mysql.h includes mariadb_version.h and that one is
# configured into the build tree rather than shipped in the source tree.
target_include_directories(mariadbclient INTERFACE
        ${mariadb_connector_SOURCE_DIR}/include
        ${mariadb_connector_BINARY_DIR}/include)

include(cmake/get_all_targets.cmake)

get_all_targets(mariadb_targets ${mariadb_connector_SOURCE_DIR})
foreach(target IN LISTS mariadb_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/MariaDB")

    # WIN32_LEAN_AND_MEAN for the connector's own sources, which otherwise do
    # not compile against a current Windows SDK.
    #
    # Several of them, secure/schannel_certs.c and the parsec plugin among
    # them, include <windows.h> before <ma_global.h>, and ma_global.h is what
    # includes <winsock2.h>. Without this macro that first windows.h pulls in
    # winsock.h, the 1.1 header, and winsock2.h then redefines every socket
    # type and function against it: a hundred or so C2011 and C2375 errors that
    # name only SDK headers and no connector file.
    #
    # This is a compile definition rather than a patch because the include
    # order is upstream's to fix, and because the macro is what the SDK
    # documents for exactly this: it excludes winsock.h so that whichever
    # header does want sockets gets to choose the version.
    if(WIN32)
        target_compile_definitions(${target} PRIVATE WIN32_LEAN_AND_MEAN=1)
    endif()
endforeach()
