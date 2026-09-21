include(FetchContent)

# fastgltf 0.9 is the final release with C++17 support, matching the engine's
# language level. The game owns its materials, textures and test infrastructure.
set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_DOCS OFF CACHE BOOL "" FORCE)

# The pin, named once. fastgltf publishes no version macro in its headers, so
# this is the only place that knows which one the build uses, and the editor's
# build details report it from here (MapEditorLib/BuildDetailsWx.cpp). Change
# both the tag and this together.
set(FASTGLTF_VERSION "0.9.0")

FetchContent_Declare(
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf.git
        GIT_TAG v${FASTGLTF_VERSION}
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(fastgltf)
