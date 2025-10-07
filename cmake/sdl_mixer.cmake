include(FetchContent)

FetchContent_Declare(
        SDL_mixer
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
        GIT_TAG 93685a9006952fdc49c58fa0f95306a9cff1ed83
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(SDL_mixer)
