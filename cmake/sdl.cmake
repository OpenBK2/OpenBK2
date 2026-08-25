# On Linux the system SDL3 is used rather than a build of this project's own.
# DXVK Native's SDL3 WSI links the distro's libSDL3, and two SDL3 instances in
# one process would each keep their own event queue and window registry, so
# input would arrive at one while rendering happened against the other, with
# nothing failing loudly enough to be noticed. Windows has no distro to ask, so
# it keeps the pinned build.
#
# SDL_mixer stays FetchContent on both platforms because no libsdl3-mixer
# package exists yet. With SDLMIXER_VENDORED off it resolves SDL3 through
# find_package, so it builds against whichever SDL3 was selected here.
if(WIN32)
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
else()
    # ships SDL3::SDL3-shared, which is the target this build already links
    find_package(SDL3 REQUIRED CONFIG)
endif()
