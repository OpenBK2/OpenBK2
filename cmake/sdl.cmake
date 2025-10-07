include(FetchContent)

FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG f3815ede24fa6e4b759c8e9beb02334003649642
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(SDL)
