include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG de1c60926218478959040ad79be8b2dbed22bd2b
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(googletest)
