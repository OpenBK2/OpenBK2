include(FetchContent)

FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG f3815ede24fa6e4b759c8e9beb02334003649642
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(SDL)

FetchContent_GetProperties(SDL SOURCE_DIR SDL_SOURCE_DIR)

include(cmake/get_all_targets.cmake)

get_all_targets(sdl_targets ${SDL_SOURCE_DIR})
foreach(target IN LISTS sdl_targets)
    set_target_properties(${target} PROPERTIES FOLDER "third_party/SDL")
endforeach()
