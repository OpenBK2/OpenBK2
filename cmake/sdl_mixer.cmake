include(FetchContent)

FetchContent_Declare(
        SDL_mixer
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
        GIT_TAG cedfeef30e93db35eee6b25759117da63f8e5a4f
        #GIT_TAG 93685a9006952fdc49c58fa0f95306a9cff1ed83 (old version)
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(SDL_mixer)

FetchContent_GetProperties(SDL_mixer SOURCE_DIR SDL_MIXER_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(sdl_mixer_targets ${SDL_MIXER_SOURCE_DIR})
foreach(target IN LISTS sdl_mixer_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/SDL_mixer")
endforeach()
