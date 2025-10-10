# see https://github.com/boostorg/cmake

include(FetchContent)

set(BOOST_IOSTREAMS_ENABLE_ZLIB OFF)
set(BOOST_IOSTREAMS_ENABLE_BZIP2 OFF)
set(BOOST_IOSTREAMS_ENABLE_LZMA OFF)
set(BOOST_IOSTREAMS_ENABLE_ZSTD OFF)

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
