# see https://github.com/boostorg/cmake

include(FetchContent)

set(BOOST_IOSTREAMS_ENABLE_ZLIB OFF)
set(BOOST_IOSTREAMS_ENABLE_BZIP2 OFF)
set(BOOST_IOSTREAMS_ENABLE_LZMA OFF)
set(BOOST_IOSTREAMS_ENABLE_ZSTD OFF)

# Only the Boost libraries the engine links. Left unset, BoostRoot adds all ~150
# of them, and adds them without EXCLUDE_FROM_ALL, so the default build compiled
# every one that has sources (url, log, serialization, test, locale, cobalt, ...)
# for nothing, several hundred objects. Configuring them was also the largest
# single cost of every reconfigure: 12.8 s of a 35 s no-change reconfigure of an
# x64 RelWithDebInfo tree, locale, cobalt and regex alone 7 s.
#
# List what a target_link_libraries() names, not every header the engine
# includes. BoostRoot adds each listed library's dependencies itself, and
# include paths come from the linked targets either way, so a header reached
# transitively (boost/functional/hash.hpp through Boost::math) keeps working.
# Linking a Boost:: target that is missing here fails at generate time, naming it.
#
# A plain variable rather than a cache entry: BoostRoot only declares its own
# cache entry when Boost is the top-level project, and reads this otherwise.
set(BOOST_INCLUDE_LIBRARIES
        filesystem      # System
        interprocess    # System
        math            # Misc
        predef          # MemoryLib
        sort            # 3Dmotor
        stacktrace      # Game: stacktrace_backtrace, stacktrace_from_exception
        uuid            # System, 3Dmotor, GameX, libdb, Main
)

# Boost.Regex arrives anyway, as a dependency of Boost.Range, and looks for ICU
# for its optional Unicode support. ICU is never found here (no headers), and a
# lookup that fails is repeated on every reconfigure: ~20 NOTFOUND cache entries
# searched again each time, 3.3 s. Nothing in the engine uses Boost.Regex, let
# alone Boost::regex_icu, so skipping the lookup changes no build output.
set(CMAKE_DISABLE_FIND_PACKAGE_ICU TRUE)

FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.89.0/boost-1.89.0-cmake.tar.xz
        URL_HASH SHA256=67acec02d0d118b5de9eb441f5fb707b3a1cdd884be00ca24b9a73c995511f74
        DOWNLOAD_EXTRACT_TIMESTAMP ON
)

FetchContent_MakeAvailable(Boost)

FetchContent_GetProperties(Boost SOURCE_DIR BOOST_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(boost_targets ${BOOST_SOURCE_DIR})
foreach(target IN LISTS boost_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/boost")
endforeach()
