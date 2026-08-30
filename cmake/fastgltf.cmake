include(FetchContent)

# fastgltf 0.9 is the final release with C++17 support, matching the engine's
# language level. The game owns its materials, textures and test infrastructure.
set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_DOCS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf.git
        GIT_TAG v0.9.0
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(fastgltf)
