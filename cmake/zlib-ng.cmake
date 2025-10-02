include(FetchContent)

FetchContent_Declare(
        zlibng
        GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
        GIT_TAG 425439062b114a0f6cf625022c41d929c7e879f9 #refs/tags/2.2.5
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(zlibng)
